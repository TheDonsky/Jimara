#include "VulkanCommandBuffer.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer) 
				: m_commandPool(commandPool), m_commandBuffer(buffer) {

			}

			VulkanCommandBuffer::operator VkCommandBuffer()const { return m_commandBuffer; }

			VulkanCommandPool* VulkanCommandBuffer::CommandPool()const { return m_commandPool; }

			namespace {
				template<typename InfoType, typename WaitInfo>
				inline static void IncludeSemaphore(VkSemaphore semaphore, const WaitInfo& info, std::unordered_map<VkSemaphore, InfoType>& collection) {
					typename std::unordered_map<VkSemaphore, InfoType>::iterator it = collection.find(semaphore);
					if (it != collection.end()) {
						WaitInfo waitInfo = it->second;
						if (waitInfo.count < info.count)
							waitInfo.count = info.count;
						waitInfo.stageFlags |= info.stageFlags;
					}
					else collection[semaphore] = info;
				}
			}

			void VulkanCommandBuffer::WaitForSemaphore(VulkanSemaphore* semaphore, VkPipelineStageFlags waitStages) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, 0, waitStages), m_semaphoresToWait);
			}

			void VulkanCommandBuffer::WaitForSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count, VkPipelineStageFlags waitStages) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, count, waitStages), m_semaphoresToWait);
			}

			void VulkanCommandBuffer::SignalSemaphore(VulkanSemaphore* semaphore) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, 0, 0), m_semaphoresToSignal);
			}

			void VulkanCommandBuffer::SignalSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count) {
				IncludeSemaphore(*semaphore, WaitInfo(semaphore, count, 0), m_semaphoresToSignal);
			}

			void VulkanCommandBuffer::RecordBufferDependency(Object* dependency) { 
				m_bufferDependencies.push_back(dependency); 
			}

			void VulkanCommandBuffer::GetSemaphoreDependencies(
				std::vector<VkSemaphore>& waitSemaphores, std::vector<uint64_t>& waitCounts, std::vector<VkPipelineStageFlags>& waitStages,
				std::vector<VkSemaphore>& signalSemaphores, std::vector<uint64_t>& signalCounts)const {
				for (std::unordered_map<VkSemaphore, WaitInfo>::const_iterator it = m_semaphoresToWait.begin(); it != m_semaphoresToWait.end(); ++it) {
					waitSemaphores.push_back(it->first);
					waitCounts.push_back(it->second.count);
					waitStages.push_back(it->second.stageFlags);
				}
				for (std::unordered_map<VkSemaphore, SemaphoreInfo>::const_iterator it = m_semaphoresToSignal.begin(); it != m_semaphoresToSignal.end(); ++it) {
					signalSemaphores.push_back(it->first);
					signalCounts.push_back(it->second.count);
				}
			}

			VulkanPrimaryCommandBuffer::VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer)
				: VulkanCommandBuffer(commandPool, buffer) {}

			VulkanSecondaryCommandBuffer::VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer)
				: VulkanCommandBuffer(commandPool, buffer) {}
		}
	}
}
