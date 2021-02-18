#pragma once
namespace Jimara { namespace Graphics { namespace Vulkan { class VulkanDeviceQueue; } } }
#include "../Synch/VulkanTimelineSemaphore.h"
#include <vector>
#include <mutex>


namespace Jimara { 
	namespace Graphics { 
		namespace Vulkan { 
			/// <summary>
			/// Represents a Vulkan-backed device queue
			/// </summary>
			class VulkanDeviceQueue : public virtual DeviceQueue {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Device handle </param>
				/// <param name="queueFamilyId"> Queue family id </param>
				VulkanDeviceQueue(VkDeviceHandle* device, uint32_t queueFamilyId);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDeviceQueue();

				/// <summary> Type cast to API object </summary>
				operator VkQueue()const;

				/// <summary> Device handle </summary>
				VkDeviceHandle* Device()const;

				/// <summary> Queue family id </summary>
				uint32_t FamilyId()const;

				/// <summary> Features, supported by the queue </summary>
				virtual FeatureBits Features()const override;

				/// <summary> Creates a new instance of a command pool </summary>
				virtual Reference<CommandPool> CreateCommandPool() override;

				/// <summary> Executes command buffer on the queue </summary>
				/// <param name="buffer"> Command buffer </param>
				virtual void ExecuteCommandBuffer(PrimaryCommandBuffer* buffer) override;


			private:
				// Device handle
				const Reference<VkDeviceHandle> m_device;
				
				// Queue family id
				const uint32_t m_queueFamilyId;

				// API handle
				const VkQueue m_queue;

				// Features, available to the queue
				const FeatureBits m_features;

				// Submition buffer entry
				/*struct SubmitionInfo {
					// Semaphore to wait on for cache entry reuse
					Reference<VulkanTimelineSemaphore> semaphore;

					// Semaphore counter
					std::atomic<uint64_t> counter;

					// Command buffer, currently executing on the device
					Reference<PrimaryCommandBuffer> commandBuffer;

					// Constructor and copy functionality:
					inline SubmitionInfo() : counter(0) {}
					inline SubmitionInfo(const SubmitionInfo& other) 
						: semaphore(other.semaphore), counter(other.counter.load()), commandBuffer(other.commandBuffer) {}
					inline SubmitionInfo& operator=(const SubmitionInfo& other) {
						semaphore = other.semaphore;
						counter = other.counter.load();
						commandBuffer = other.commandBuffer;
						return (*this);
					}
				};*/

				// Submition buffer lock
				std::mutex m_submitionLock;

				// Ring buffer of latest submitions
				std::vector<Reference<PrimaryCommandBuffer>> m_submitionBuffer;

				// Submition pointer
				std::atomic<size_t> m_submitionPointer;
			};
		} 
	} 
}
