#include "VulkanDynamicBuffer.h"


#pragma warning(disable: 26812)
#pragma warning(disable: 26110)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanDynamicBuffer::VulkanDynamicBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount)
				: m_device(device), m_objectSize(objectSize), m_objectCount(objectCount), m_cpuMappedData(nullptr), m_updater(*device) {}

			VulkanDynamicBuffer::~VulkanDynamicBuffer() {}

			size_t VulkanDynamicBuffer::ObjectSize()const {
				return m_objectSize;
			}

			size_t VulkanDynamicBuffer::ObjectCount()const {
				return m_objectCount;
			}

			Buffer::CPUAccess VulkanDynamicBuffer::HostAccess()const {
				return CPUAccess::CPU_WRITE_ONLY;
			}

			void* VulkanDynamicBuffer::Map() {
				if (m_cpuMappedData != nullptr) return m_cpuMappedData;

				m_bufferLock.lock();

				if (m_stagingBuffer == nullptr)
					m_stagingBuffer = Object::Instantiate<VulkanStaticBuffer>(m_device, m_objectSize, m_objectCount, true
						, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				m_cpuMappedData = m_stagingBuffer->Map();

				return m_cpuMappedData;
			}

			void VulkanDynamicBuffer::Unmap(bool write) {
				if (m_cpuMappedData == nullptr) return;
				m_stagingBuffer->Unmap(write);
				m_cpuMappedData = nullptr;
				if (write) m_dataBuffer = nullptr;
				else m_stagingBuffer = nullptr;
				m_bufferLock.unlock();
			}

			Reference<VulkanStaticBuffer> VulkanDynamicBuffer::GetStaticHandle(VulkanCommandBuffer* commandBuffer) {
				Reference<VulkanStaticBuffer> dataBuffer = m_dataBuffer;
				if (dataBuffer != nullptr) {
					m_updater.WaitForTimeline(commandBuffer);
					commandBuffer->RecordBufferDependency(dataBuffer);
					return dataBuffer;
				}

				std::unique_lock<std::mutex> lock(m_bufferLock);
				if (m_dataBuffer == nullptr)
					m_dataBuffer = Object::Instantiate<VulkanStaticBuffer>(m_device, m_objectSize, m_objectCount, true
						, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
						, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				commandBuffer->RecordBufferDependency(m_dataBuffer);

				if (m_stagingBuffer == nullptr || m_cpuMappedData != nullptr) {
					m_updater.WaitForTimeline(commandBuffer);
					return m_dataBuffer;
				}

				m_updater.Update(commandBuffer, Callback<VulkanCommandBuffer*>(&VulkanDynamicBuffer::UpdateData, this));

				return m_dataBuffer;
			}

			void VulkanDynamicBuffer::UpdateData(VulkanCommandBuffer* commandBuffer) {
				VkBufferCopy copy = {};
				{
					copy.srcOffset = 0;
					copy.dstOffset = 0;
					copy.size = static_cast<VkDeviceSize>(m_objectSize * m_objectCount);
				}
				commandBuffer->RecordBufferDependency(m_stagingBuffer);
				commandBuffer->RecordBufferDependency(m_dataBuffer);
				vkCmdCopyBuffer(*commandBuffer, *m_stagingBuffer, *m_dataBuffer, 1, &copy);
				m_stagingBuffer = nullptr;
			}
		}
	}
}
#pragma warning(default: 26812)
#pragma warning(default: 26110)
