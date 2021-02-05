#include "VulkanConstantBuffer.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanConstantBuffer::VulkanConstantBuffer(size_t size) 
				: m_size(size)
				, m_data(size > 0 ? new uint8_t[size] : nullptr)
				, m_mappedData(size > 0 ? new uint8_t[size] : nullptr)
				, m_revision(~((uint64_t)0)) {}

			VulkanConstantBuffer::~VulkanConstantBuffer() {
				if (m_data != nullptr) {
					delete[] m_data;
					m_data = nullptr;
				}
				if (m_mappedData != nullptr) {
					delete[] m_mappedData;
					m_mappedData = nullptr;
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
					memcpy(m_data, m_mappedData, m_size);
					m_revision++;
				}
				else memcpy(m_mappedData, m_data, m_size);
				m_lock.unlock();
			}


			VulkanPipelineConstantBuffer::VulkanPipelineConstantBuffer(VulkanDevice* device, VulkanConstantBuffer* buffer, size_t commandBufferCount) 
				: m_device(device), m_constantBuffer(buffer) {
				if (commandBufferCount <= 0 || m_constantBuffer == nullptr) return;
				for (size_t i = 0; i < commandBufferCount; i++) {
					VkBufferCreateInfo bufferInfo = {};
					{
						bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
						bufferInfo.flags = 0;
						bufferInfo.size = m_constantBuffer->m_size;
						bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
						bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // We may want to change this down the line...
					}
					Attachment attachment;

					if (vkCreateBuffer(*m_device, &bufferInfo, nullptr, &attachment.buffer) != VK_SUCCESS)
						m_device->Log()->Fatal("VulkanPipelineConstantBuffer - Failed to create a buffer!");
					else m_buffers.push_back(attachment);
				}

				assert(m_buffers.size() > 0);
				VkMemoryRequirements memRequirementsPerBuffer;
				vkGetBufferMemoryRequirements(*m_device, m_buffers[0].buffer, &memRequirementsPerBuffer);
				
				VkMemoryRequirements memRequirements = memRequirementsPerBuffer;
				VkDeviceSize sizePerBuffer =
					((memRequirementsPerBuffer.size + memRequirementsPerBuffer.alignment - 1) / memRequirementsPerBuffer.alignment) * memRequirementsPerBuffer.alignment;
				memRequirements.size = sizePerBuffer * m_buffers.size();

				m_memory = m_device->MemoryPool()->Allocate(memRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				for (size_t i = 0; i < m_buffers.size(); i++) {
					Attachment& attachment = m_buffers[i];
					attachment.memoryOffset = (sizePerBuffer * i);
					vkBindBufferMemory(*m_device, attachment.buffer, m_memory->Memory(), attachment.memoryOffset + m_memory->Offset());
				}
			}

			VulkanPipelineConstantBuffer::~VulkanPipelineConstantBuffer() {
				for (size_t i = 0; i < m_buffers.size(); i++)
					vkDestroyBuffer(*m_device, m_buffers[i].buffer, nullptr);
				m_buffers.clear();
			}

			VulkanConstantBuffer* VulkanPipelineConstantBuffer::TargetBuffer()const {
				return m_constantBuffer;
			}

			VkBuffer VulkanPipelineConstantBuffer::GetBuffer(size_t commandBufferIndex) {
				Attachment& attachment = m_buffers[commandBufferIndex];
				if ((!attachment.resvison.has_value()) || attachment.resvison.value() != m_constantBuffer->m_revision) {
					std::unique_lock<std::mutex> lock(m_constantBuffer->m_lock);
					if ((!attachment.resvison.has_value()) || attachment.resvison.value() != m_constantBuffer->m_revision) {
						uint8_t* data = static_cast<uint8_t*>(m_memory->Map(false)) + attachment.memoryOffset;
						memcpy(data, m_constantBuffer->m_data, m_constantBuffer->m_size);
						m_memory->Unmap(true);
						attachment.resvison = m_constantBuffer->m_revision;
					}
				}
				return attachment.buffer;
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
