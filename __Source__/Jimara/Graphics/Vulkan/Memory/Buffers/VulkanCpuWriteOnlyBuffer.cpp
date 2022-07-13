#include "VulkanCpuWriteOnlyBuffer.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
#pragma warning(disable: 26117)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCpuWriteOnlyBuffer::VulkanCpuWriteOnlyBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount)
				: VulkanArrayBuffer(device, objectSize, objectCount, true, 
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				, m_cpuMappedData(nullptr), m_updater(*device) {}

			VulkanCpuWriteOnlyBuffer::~VulkanCpuWriteOnlyBuffer() {}

			Buffer::CPUAccess VulkanCpuWriteOnlyBuffer::HostAccess()const {
				return CPUAccess::CPU_WRITE_ONLY;
			}

			void* VulkanCpuWriteOnlyBuffer::Map() {
				if (m_cpuMappedData != nullptr) return m_cpuMappedData;

				m_bufferLock.lock();

				if (m_stagingBuffer == nullptr)
					m_stagingBuffer = Object::Instantiate<VulkanArrayBuffer>(Device(), ObjectSize(), ObjectCount(), true
						, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				m_cpuMappedData = m_stagingBuffer->Map();

				return m_cpuMappedData;
			}

			void VulkanCpuWriteOnlyBuffer::Unmap(bool write) {
				if (m_cpuMappedData == nullptr) return;
				m_stagingBuffer->Unmap(write);
				m_cpuMappedData = nullptr;
				if (!write) m_stagingBuffer = nullptr;
				m_bufferLock.unlock();
			}

			VkBuffer VulkanCpuWriteOnlyBuffer::GetVulkanHandle(VulkanCommandBuffer* commandBuffer) {
				VkBuffer dataBuffer = VulkanArrayBuffer::GetVulkanHandle(commandBuffer);

				std::unique_lock<std::mutex> lock(m_bufferLock);
				if (m_stagingBuffer == nullptr || m_cpuMappedData != nullptr)
					m_updater.WaitForTimeline(commandBuffer);
				else {
					auto updateData = [&](VulkanCommandBuffer* updateBuffer) {
						VkBufferCopy copy = {};
						{
							copy.srcOffset = 0;
							copy.dstOffset = 0;
							copy.size = static_cast<VkDeviceSize>(ObjectSize() * ObjectCount());
						}
						updateBuffer->RecordBufferDependency(m_stagingBuffer);
						VkBuffer stagingBuffer = m_stagingBuffer->GetVulkanHandle(updateBuffer);
						vkCmdCopyBuffer(*updateBuffer, stagingBuffer, dataBuffer, 1, &copy);
						m_stagingBuffer = nullptr;
					};
					m_updater.Update(commandBuffer, Callback<VulkanCommandBuffer*>::FromCall(&updateData));
				}

				return dataBuffer;
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
#pragma warning(default: 26117)
