#include "VulkanArrayBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanArrayBuffer::VulkanArrayBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, bool writeOnly, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags)
				: m_device(device), m_elemSize(objectSize), m_elemCount(objectCount), m_writeOnly(writeOnly), m_usage(usage), m_memoryFlags(memoryFlags), m_buffer(VK_NULL_HANDLE) {
				size_t allocation = m_elemSize * m_elemCount;
				if (allocation <= 0) allocation = 1;
				VkBufferCreateInfo bufferInfo = {};
				{
					bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferInfo.flags = 0;
					bufferInfo.size = static_cast<VkDeviceSize>(allocation);
					bufferInfo.usage = m_usage;
					bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // We may want to change this down the line...
				}
				if (vkCreateBuffer(*m_device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanArrayBuffer - Failed to create a buffer!");

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(*m_device, m_buffer, &memRequirements);
				m_memory = m_device->MemoryPool()->Allocate(memRequirements, m_memoryFlags);

				if(vkBindBufferMemory(*m_device, m_buffer, m_memory->Memory(), m_memory->Offset()) != VK_SUCCESS)
					m_device->Log()->Fatal("VulkanArrayBuffer - Failed to bind vulkan memory!");
			}

			VulkanArrayBuffer::~VulkanArrayBuffer() {
				if (m_buffer != VK_NULL_HANDLE) {
					vkDestroyBuffer(*m_device, m_buffer, nullptr);
					m_buffer = VK_NULL_HANDLE;
				}
			}

			size_t VulkanArrayBuffer::ObjectSize()const {
				return m_elemSize;
			}

			size_t VulkanArrayBuffer::ObjectCount()const {
				return m_elemCount;
			}

			Buffer::CPUAccess VulkanArrayBuffer::HostAccess()const {
				return ((m_memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) 
					? (m_writeOnly ? CPUAccess::CPU_WRITE_ONLY : CPUAccess::CPU_READ_WRITE) : CPUAccess::OTHER;
			}

			void* VulkanArrayBuffer::Map() {
				return m_memory->Map(!m_writeOnly);
			}

			void VulkanArrayBuffer::Unmap(bool write) {
				m_memory->Unmap(write);
			}

			void VulkanArrayBuffer::Copy(CommandBuffer* commandBuffer, ArrayBuffer* srcBuffer, size_t numBytes, size_t dstOffset, size_t srcOffset) {
				VulkanCommandBuffer* vulkanCommandBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanCommandBuffer == nullptr) {
					m_device->Log()->Error("VulkanArrayBuffer::Copy - commandBuffer NULL or Unsupported!");
					return;
				}

				VulkanArrayBuffer* vulkanSourceBuffer = dynamic_cast<VulkanArrayBuffer*>(srcBuffer);
				if (vulkanSourceBuffer == nullptr) {
					m_device->Log()->Error("VulkanArrayBuffer::Copy - srcBuffer NULL or Unsupported!");
					return;
				}

				const size_t dstBufferSize = m_elemSize * m_elemCount;
				if (dstBufferSize <= dstOffset) return;

				const size_t srcBufferSize = vulkanSourceBuffer->m_elemSize * vulkanSourceBuffer->m_elemCount;
				if (srcBufferSize <= srcOffset) return;

				numBytes = Math::Min(Math::Min(dstBufferSize, srcBufferSize), numBytes);

				const size_t dstRegionEnd = Math::Min(dstOffset + numBytes, dstBufferSize);
				const size_t srcRegionEnd = Math::Min(srcOffset + numBytes, srcBufferSize);

				const size_t dstCopySize = (dstRegionEnd - dstOffset);
				const size_t srcCopySize = (srcRegionEnd - srcOffset);

				const size_t copySize = Math::Min(dstCopySize, srcCopySize); 
				if (copySize <= 0u) return;

				VkMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
					barrier.pNext = nullptr;
					barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				}
				vkCmdPipelineBarrier(*vulkanCommandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
					| VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					0, 1, &barrier, 0, nullptr, 0, nullptr);

				VkBufferCopy copy = {};
				{
					copy.srcOffset = static_cast<VkDeviceSize>(srcOffset);
					copy.dstOffset = static_cast<VkDeviceSize>(dstOffset);
					copy.size = static_cast<VkDeviceSize>(copySize);
				}
				vkCmdCopyBuffer(*vulkanCommandBuffer, *vulkanSourceBuffer, *this, 1, &copy);

				vulkanCommandBuffer->RecordBufferDependency(this);
				vulkanCommandBuffer->RecordBufferDependency(vulkanSourceBuffer);
			}

			VulkanDevice* VulkanArrayBuffer::Device()const {
				return m_device;
			}

			VkBufferUsageFlags VulkanArrayBuffer::Usage()const {
				return m_usage;
			}

			VkMemoryPropertyFlags VulkanArrayBuffer::MemoryFlags()const {
				return m_memoryFlags;
			}

			VkDeviceSize VulkanArrayBuffer::AllocationSize()const {
				return m_memory->Size();
			}

			VulkanArrayBuffer::operator VkBuffer()const {
				return m_buffer;
			}
		}
	}
}
