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
#include "VulkanDeviceQueue.h"
#include "../Synch/VulkanFence.h"
#include "../Synch/VulkanSemaphore.h"
#include "../Synch/VulkanTimelineSemaphore.h"
#include <unordered_map>
#include <vector>


#pragma warning(disable: 4250)
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

				virtual void Reset() override;

				virtual void BeginRecording() override;

				virtual void EndRecording() override;

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
						: SemaphoreInfo(sem, cnt), stageFlags(flags) {}
					
					inline WaitInfo(const WaitInfo&) = default;
					inline WaitInfo& operator=(const WaitInfo&) = default;

					inline WaitInfo(const SemaphoreInfo& info) : SemaphoreInfo(info), stageFlags() {}
					inline WaitInfo& operator=(const SemaphoreInfo& info) { *((SemaphoreInfo*)this) = info; return (*this); }
				};

				std::unordered_map<VkSemaphore, WaitInfo> m_semaphoresToWait;

				std::unordered_map<VkSemaphore, SemaphoreInfo> m_semaphoresToSignal;

				std::vector<Reference<Object>> m_bufferDependencies;
			};

			class VulkanPrimaryCommandBuffer : public VulkanCommandBuffer, public virtual PrimaryCommandBuffer {
			public:
				VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				virtual ~VulkanPrimaryCommandBuffer();

				virtual void Reset() override;

				virtual void Wait() override;

				void SumbitOnQueue(VkQueue queue);

			private:
				VulkanFence m_fence;

				std::atomic<bool> m_running;
			};

			class VulkanSecondaryCommandBuffer : public VulkanCommandBuffer, public virtual SecondaryCommandBuffer {
			public:
				VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

			private:

			};
		}
	}
}
#pragma warning(default: 4250)
