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
			/// <summary>
			/// Vulkan-backed command buffer
			/// </summary>
			class VulkanCommandBuffer : public virtual CommandBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="commandPool"> Command pool the buffer belongs to </param>
				/// <param name="buffer"> Command buffer (keep in mind, that creator is responsible for the buffer's lifetime unless it was created internally by the pool) </param>
				VulkanCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				/// <summary> Type cast to API object </summary>
				operator VkCommandBuffer()const;

				/// <summary> Owner command pool </summary>
				VulkanCommandPool* CommandPool()const;

				/// <summary>
				/// Records semaphore dependency to wait on
				/// </summary>
				/// <param name="semaphore"> Semaphore to wait on </param>
				/// <param name="waitStages"> Stages to wait for </param>
				void WaitForSemaphore(VulkanSemaphore* semaphore, VkPipelineStageFlags waitStages);

				/// <summary>
				/// Records timeline semaphore dependency to wait on
				/// </summary>
				/// <param name="semaphore"> Semaphore to wait on </param>
				/// <param name="count"> Counter to wait for </param>
				/// <param name="waitStages"> Stages to wait for </param>
				void WaitForSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count, VkPipelineStageFlags waitStages);

				/// <summary>
				/// Records a semaphore that should be signalled when the command buffer is executed
				/// </summary>
				/// <param name="semaphore"> Semaphore to signal </param>
				void SignalSemaphore(VulkanSemaphore* semaphore);

				/// <summary>
				/// Records a timeline semaphore to signal when the command buffer is executed
				/// </summary>
				/// <param name="semaphore"> Semaphore to signal </param>
				/// <param name="count"></param>
				void SignalSemaphore(VulkanTimelineSemaphore* semaphore, uint64_t count);

				/// <summary>
				/// Records an object that has to stay alive for the command buffer to execute without issues
				/// </summary>
				/// <param name="dependency"> Object, the command buffer depends on </param>
				void RecordBufferDependency(Object* dependency);

				/// <summary>
				/// Retrieves currently set semaphore dependencies and signals
				/// </summary>
				/// <param name="waitSemaphores"> Semaphores to wait for </param>
				/// <param name="waitCounts"> Counters for each semaphore (arbitrary filler values for non-timeline entries) </param>
				/// <param name="waitStages"> Stages to wait for per semaphore </param>
				/// <param name="signalSemaphores"> Semaphores to signal </param>
				/// <param name="signalCounts"> Semaphore signal values (arbitrary filler values for non-timeline entries) </param>
				void GetSemaphoreDependencies(
					std::vector<VkSemaphore>& waitSemaphores, std::vector<uint64_t>& waitCounts, std::vector<VkPipelineStageFlags>& waitStages,
					std::vector<VkSemaphore>& signalSemaphores, std::vector<uint64_t>& signalCounts)const;

				/// <summary> Resets command buffer and all of it's internal state previously recorded </summary>
				virtual void Reset() override;

				/// <summary> Starts recording the command buffer (does NOT auto-invoke Reset()) </summary>
				virtual void BeginRecording() override;

				/// <summary> Ends recording the command buffer </summary>
				virtual void EndRecording() override;


			private:
				// "Owner" command pool
				const Reference<VulkanCommandPool> m_commandPool;
				
				// Terget command buffer
				const VkCommandBuffer m_commandBuffer;

				// Information about semaphore
				struct SemaphoreInfo {
					// Semaphore handle
					Reference<Object> semaphore;

					// Counter value
					uint64_t count;

					// Constructor
					inline SemaphoreInfo(Object* sem = nullptr, uint64_t cnt = 0) : semaphore(sem), count(cnt) {}
				};

				// Information about wait dependency
				struct WaitInfo : public SemaphoreInfo {
					// Stages to wait for
					VkPipelineStageFlags stageFlags;

					// Constructor
					inline WaitInfo(Object* sem = nullptr, uint64_t cnt = 0, VkPipelineStageFlags flags = 0)
						: SemaphoreInfo(sem, cnt), stageFlags(flags) {}
					
					// Default copy
					inline WaitInfo(const WaitInfo&) = default;
					inline WaitInfo& operator=(const WaitInfo&) = default;

					// Copy from SemaphoreInfo
					inline WaitInfo(const SemaphoreInfo& info) : SemaphoreInfo(info), stageFlags() {}
					inline WaitInfo& operator=(const SemaphoreInfo& info) { *((SemaphoreInfo*)this) = info; return (*this); }
				};

				// Semaphores to wait for
				std::unordered_map<VkSemaphore, WaitInfo> m_semaphoresToWait;

				// Semaphores to signal
				std::unordered_map<VkSemaphore, SemaphoreInfo> m_semaphoresToSignal;

				// Object dependencies
				std::vector<Reference<Object>> m_bufferDependencies;
			};


			/// <summary>
			/// Vulkan-backed primary command buffer
			/// </summary>
			class VulkanPrimaryCommandBuffer : public VulkanCommandBuffer, public virtual PrimaryCommandBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="commandPool"> Command pool the buffer belongs to </param>
				/// <param name="buffer"> Command buffer (keep in mind, that creator is responsible for the buffer's lifetime unless it was created internally by the pool) </param>
				VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPrimaryCommandBuffer();

				/// <summary> Resets command buffer and all of it's internal state previously recorded </summary>
				virtual void Reset() override;

				/// <summary> If the command buffer has been previously submitted, this call will wait on execution wo finish </summary>
				virtual void Wait() override;

				/// <summary>
				/// Submits command buffer to queue
				/// </summary>
				/// <param name="queue"> Vulkan queue </param>
				void SumbitOnQueue(VkQueue queue);

			private:
				// Fence for submition
				VulkanFence m_fence;

				// True, if submitted
				std::atomic<bool> m_running;
			};


			/// <summary>
			/// Vulkan-backed secondary command buffer
			/// </summary>
			class VulkanSecondaryCommandBuffer : public VulkanCommandBuffer, public virtual SecondaryCommandBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="commandPool"> Command pool the buffer belongs to </param>
				/// <param name="buffer"> Command buffer (keep in mind, that creator is responsible for the buffer's lifetime unless it was created internally by the pool) </param>
				VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);


			private:

			};
		}
	}
}
#pragma warning(default: 4250)
