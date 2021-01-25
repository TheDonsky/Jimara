#include "VulkanDeviceResidentBuffer.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanDeviceResidentBuffer::VulkanDeviceResidentBuffer(VulkanDevice* device, size_t objectSize, size_t objectCount, CPUAccess cpuAccess)
				: m_device(device), m_objectSize(objectSize), m_objectCount(objectCount), m_cpuAccess(cpuAccess), m_cpuMappedData(nullptr) {}

			VulkanDeviceResidentBuffer::~VulkanDeviceResidentBuffer() {}

			size_t VulkanDeviceResidentBuffer::ObjectSize()const {
				return m_objectSize;
			}

			size_t VulkanDeviceResidentBuffer::ObjectCount()const {
				return m_objectCount;
			}

			Buffer::CPUAccess VulkanDeviceResidentBuffer::HostAccess()const {
				return m_cpuAccess;
			}

			void* VulkanDeviceResidentBuffer::Map() {
				std::unique_lock<std::mutex> lock(m_bufferLock);
				if (m_cpuMappedData != nullptr) return m_cpuMappedData;

				Reference<VulkanBuffer>& mainBuffer = (m_cpuAccess == CPUAccess::CPU_READ_WRITE) ? m_dataBuffer : m_stagingBuffer;

				if (mainBuffer == nullptr)
					mainBuffer = Object::Instantiate<VulkanBuffer>(m_device, m_objectSize, m_objectCount, true
						, (m_cpuAccess == CPUAccess::CPU_READ_WRITE) 
						? (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
						: VK_BUFFER_USAGE_TRANSFER_SRC_BIT
						, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				m_cpuMappedData = mainBuffer->Map();

				return m_cpuMappedData;
			}

			void VulkanDeviceResidentBuffer::Unmap(bool write) {
				std::unique_lock<std::mutex> lock(m_bufferLock);
				if (m_cpuMappedData == nullptr) return;
				m_stagingBuffer->Unmap(write);
				m_cpuMappedData = nullptr;
				if (write) m_dataBuffer = nullptr;
				else m_stagingBuffer = nullptr;
			}

			Reference<VulkanBuffer> VulkanDeviceResidentBuffer::GetDataBuffer(VulkanRenderEngine::CommandRecorder* commandRecorder) {
				Reference<VulkanBuffer> dataBuffer = m_dataBuffer;
				if (dataBuffer != nullptr) {
					commandRecorder->RecordBufferDependency(dataBuffer);
					return dataBuffer;
				}

				std::unique_lock<std::mutex> lock(m_bufferLock);
				if (m_dataBuffer == nullptr)
					m_dataBuffer = Object::Instantiate<VulkanBuffer>(m_device, m_objectSize, m_objectCount, true
						, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
						, (m_cpuAccess == CPUAccess::CPU_READ_WRITE) 
						? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				commandRecorder->RecordBufferDependency(m_dataBuffer);

				if (m_stagingBuffer == nullptr || m_cpuMappedData != nullptr) return m_dataBuffer;

				VkBufferCopy copy = {};
				{
					copy.srcOffset = 0;
					copy.dstOffset = 0;
					copy.size = static_cast<VkDeviceSize>(m_objectSize * m_objectCount);
				}
				commandRecorder->RecordBufferDependency(m_stagingBuffer);
				vkCmdCopyBuffer(commandRecorder->CommandBuffer(), *m_stagingBuffer, *m_dataBuffer, 1, &copy);
				m_stagingBuffer = nullptr;
				
				return m_dataBuffer;
			}
		}
	}
}
#pragma warning(default: 26812)
