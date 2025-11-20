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
#include "../../Synch/VulkanFence.h"
#include "../../Synch/VulkanSemaphore.h"
#include "../../Synch/VulkanTimelineSemaphore.h"
#include "../../Memory/Textures/VulkanTextureView.h"
#include <unordered_map>
#include <vector>


#pragma warning(disable: 4250)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed command buffer
			/// </summary>
			class JIMARA_API VulkanCommandBuffer : public virtual CommandBuffer {
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
				/// Records a list of objects as dependencies for execution.
				/// <para/> When we rely on device-addresses, 
				/// there is little to no way for the system to 'automagically' know which resources to keep alive.
				/// To address this issue, we can call AddDependencies() to make sure the command buffer keeps track of the resources.
				/// <para/> For performance reasons, the underlying implementations may or may not choose not to verify the resource types before recording them.
				/// If the command buffer is not reset and there is a self-reference hidden somewhere within the object-list, that might cause a memory leak.
				/// Additionally, if the implementation chooses to verify types to stay on the safe side, arbitrary class objects will not be recorded,
				/// meaning that relying on an 'uber-object' with some resource references within will be unsafe to use.
				/// Because of these factors, the only valid usage is to only include Buffers, Textures and acceleration structures from the same device as arguments. 
				/// There should and mostly will be a validation code for that, but only in Debug-mode.
				/// </summary>
				/// <param name="resources"> Resources list </param>
				/// <param name="count"> Number of entries within the list </param>
				virtual void AddDependencies(Object* const* resources, size_t count) override;

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


			protected:
				/// <summary>
				/// Makes sure, the destination command buffer waits for every semaphore the source does and signals every semaphore, destination does
				/// </summary>
				/// <param name="src"> 'Source' command buffer </param>
				/// <param name="dst"> 'Destination' command buffer </param>
				static void AddSemaphoreDependencies(const VulkanCommandBuffer* src, VulkanCommandBuffer* dst);
			};


			/// <summary>
			/// Vulkan-backed primary command buffer
			/// </summary>
			class JIMARA_API VulkanPrimaryCommandBuffer : public VulkanCommandBuffer, public virtual PrimaryCommandBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="commandPool"> Command pool the buffer belongs to </param>
				/// <param name="buffer"> Command buffer (keep in mind, that creator is responsible for the buffer's lifetime unless it was created internally by the pool) </param>
				VulkanPrimaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPrimaryCommandBuffer();

				/// <summary> Starts recording the command buffer (does NOT auto-invoke Reset()) </summary>
				virtual void BeginRecording() override;

				/// <summary> Resets command buffer and all of it's internal state previously recorded </summary>
				virtual void Reset() override;

				/// <summary> If the command buffer has been previously submitted, this call will wait on execution wo finish </summary>
				virtual void Wait() override;

				/// <summary> Ends recording the command buffer </summary>
				virtual void EndRecording() override;

				/// <summary>
				/// Executes commands from a secondary command buffer
				/// </summary>
				/// <param name="commands"> Command buffer to execute </param>
				virtual void ExecuteCommands(SecondaryCommandBuffer* commands) override;

				/// <summary>
				/// Submits command buffer to queue
				/// </summary>
				/// <param name="queue"> Vulkan queue </param>
				void SumbitOnQueue(VulkanDeviceQueue* queue);

			private:
				// Fence for submition
				VulkanFence m_fence;

				// True, if submitted
				std::atomic<bool> m_running;
			};


			/// <summary>
			/// Vulkan-backed secondary command buffer
			/// </summary>
			class JIMARA_API VulkanSecondaryCommandBuffer : public VulkanCommandBuffer, public virtual SecondaryCommandBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="commandPool"> Command pool the buffer belongs to </param>
				/// <param name="buffer"> Command buffer (keep in mind, that creator is responsible for the buffer's lifetime unless it was created internally by the pool) </param>
				VulkanSecondaryCommandBuffer(VulkanCommandPool* commandPool, VkCommandBuffer buffer);

				/// <summary>
				/// Begins command buffer recording
				/// </summary>
				/// <param name="activeRenderPass"> Render pass, that will be active during the command buffer execution (can be nullptr, if there's no active pass) </param>
				/// <param name="targetFrameBuffer"> If the command buffer is meant to be used as a part of a render pass, this will be our target frame buffer </param>
				virtual void BeginRecording(RenderPass* activeRenderPass, FrameBuffer* targetFrameBuffer) override;
			};
		}
	}
}
#pragma warning(default: 4250)
