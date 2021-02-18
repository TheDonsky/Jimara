#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanCommandBuffer;
			class VulkanPrimaryCommandBuffer;
			class VulkanSecondaryCommandBuffer;
		}
	}
}
#include "VulkanCommandPool.h"
#include "../Synch/VulkanSemaphore.h"
#include "../Synch/VulkanTimelineSemaphore.h"
#include <unordered_map>
#include <vector>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanCommandBuffer : public virtual CommandBuffer {
			public:
				VulkanCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				operator VkCommandBuffer()const;

				VulkanCommandPool* CommandPool()const;

				void WaitForSemaphore(VulkanSemaphore* semaphore, VkPipelineStageFlags waitStages);

				void WaitForSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count, VkPipelineStageFlags waitStages);

				void SignalSemaphore(VulkanSemaphore* semaphore);

				void SignalSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count);

				void RecordBufferDependency(Object* dependency);

				void GetSemaphoreDependencies(
					std::vector<VkSemaphore>& waitSemaphores, std::vector<uint64_t>& waitCounts, std::vector<VkPipelineStageFlags>& waitStages,
					std::vector<VkSemaphore>& signalSemaphores, std::vector<uint64_t>& signalCounts)const;

			private:
				const Reference<VulkanCommandPool> m_commandPool;
				
				const VkCommandBuffer m_commandBuffer;

				struct SemaphoreInfo {
					Reference<Object> semaphore;
					uint64_t count;

					inline SemaphoreInfo(Object* sem = nullptr, uint64_t cnt = 0) : semaphore(sem), count(cnt) {}
				};

				struct WaitInfo : public SemaphoreInfo {
					VkPipelineStageFlags stageFlags;

					inline WaitInfo(Object* sem = nullptr, uint64_t cnt = 0, VkPipelineStageFlags flags = 0) 
						: SemaphoreInfo(sem, cnt), stageFlags(0) {}
					
					inline WaitInfo(const WaitInfo&) = default;
					inline WaitInfo& operator=(const WaitInfo&) = default;

					inline WaitInfo(const SemaphoreInfo& info) : SemaphoreInfo(info), stageFlags() {}
					inline WaitInfo& operator=(const SemaphoreInfo& info) { *((SemaphoreInfo*)this) = info; return (*this); }
				};

				std::unordered_map<VkSemaphore, WaitInfo> m_semaphoresToWait;

				std::unordered_map<VkSemaphore, SemaphoreInfo> m_semaphoresToSignal;

				std::vector<Reference<Object>> m_bufferDependencies;
			};

			class VulkanPrimaryCommandBuffer : public virtual PrimaryCommandBuffer, public VulkanCommandBuffer {
			public:
				VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

			private:

			};

			class VulkanSecondaryCommandBuffer : public virtual SecondaryCommandBuffer, public VulkanCommandBuffer {
			public:
				VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

			private:

			};
		}
	}
}
