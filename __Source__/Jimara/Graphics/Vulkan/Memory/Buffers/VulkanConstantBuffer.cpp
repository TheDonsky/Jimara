#include "VulkanConstantBuffer.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanConstantBuffer::VulkanConstantBuffer(size_t size) 
				: m_size(size)
				, m_data(size > 0 ? new uint8_t[size << 1] : nullptr)
				, m_mappedData(nullptr)
				, m_revision(~((uint64_t)0)) {
				m_mappedData = m_data + m_size;
			}

			VulkanConstantBuffer::~VulkanConstantBuffer() {
				if (m_data != nullptr) {
					delete[] m_data;
					m_data = nullptr;
				}
			}

			size_t VulkanConstantBuffer::ObjectSize()const {
				return m_size;
			}

			Buffer::CPUAccess VulkanConstantBuffer::HostAccess()const {
				return Buffer::CPUAccess::CPU_READ_WRITE;
			}

			void* VulkanConstantBuffer::Map() {
				m_lock.lock();
				return static_cast<void*>(m_mappedData);
			}

			void VulkanConstantBuffer::Unmap(bool write) {
				if (write) {
					if (memcmp(m_data, m_mappedData, m_size) != 0) {
						memcpy(m_data, m_mappedData, m_size);
						m_revision++;
						m_onRevisionChanged(this);
					}
				}
				else memcpy(m_mappedData, m_data, m_size);
				m_lock.unlock();
			}


			VulkanPipelineConstantBuffer::VulkanPipelineConstantBuffer(VulkanDevice* device, VulkanConstantBuffer* buffer, size_t commandBufferCount) 
				: m_device(device), m_constantBuffer(buffer), m_buffer(VK_NULL_HANDLE) {
				if (commandBufferCount <= 0 || m_constantBuffer == nullptr) return;

				VkDeviceSize offsetAlignment = device->PhysicalDeviceInfo()->DeviceProperties().limits.minUniformBufferOffsetAlignment;
				VkDeviceSize bufferStep = (offsetAlignment * ((static_cast<VkDeviceSize>(m_constantBuffer->ObjectSize()) + offsetAlignment - 1) / offsetAlignment));
				VkBufferCreateInfo bufferInfo = {};
				{
					bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferInfo.flags = 0;
					bufferInfo.size = static_cast<VkDeviceSize>(bufferStep * commandBufferCount);
					bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
					bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // We may want to change this down the line...
				}
				if (vkCreateBuffer(*m_device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanPipelineConstantBuffer - Failed to create a buffer!");
				
				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(*m_device, m_buffer, &memRequirements);
				m_memory = m_device->MemoryPool()->Allocate(memRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				vkBindBufferMemory(*m_device, m_buffer, m_memory->Memory(), m_memory->Offset());

				for (VkDeviceSize i = 0; i < commandBufferCount; i++)
					m_buffers.push_back(Attachment(bufferStep * i));				
			}

			VulkanPipelineConstantBuffer::~VulkanPipelineConstantBuffer() {
				if (m_buffer != VK_NULL_HANDLE) {
					vkDestroyBuffer(*m_device, m_buffer, nullptr);
					m_buffer = VK_NULL_HANDLE;
				}
				m_buffers.clear();
			}

			VulkanConstantBuffer* VulkanPipelineConstantBuffer::TargetBuffer()const {
				return m_constantBuffer;
			}

			std::pair<VkBuffer, VkDeviceSize> VulkanPipelineConstantBuffer::GetBuffer(size_t commandBufferIndex) {
				Attachment& attachment = m_buffers[commandBufferIndex];
				if ((!attachment.resvison.has_value()) || attachment.resvison.value() != m_constantBuffer->Revision()) {
					const void* bufferData = m_constantBuffer->Map();
					if ((!attachment.resvison.has_value()) || attachment.resvison.value() != m_constantBuffer->Revision()) {
						uint8_t* data = static_cast<uint8_t*>(m_memory->Map(false)) + attachment.memoryOffset;
						memcpy(data, bufferData, m_constantBuffer->ObjectSize());
						m_memory->Unmap(true);
						attachment.resvison = m_constantBuffer->Revision();
					}
					m_constantBuffer->Unmap(false);
				}
				return std::make_pair(m_buffer, attachment.memoryOffset);
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
