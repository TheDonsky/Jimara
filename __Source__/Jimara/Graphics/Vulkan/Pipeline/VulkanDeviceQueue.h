#pragma once
namespace Jimara { namespace Graphics { namespace Vulkan { class VulkanDeviceQueue; } } }
#include "../Synch/VulkanTimelineSemaphore.h"
#include "../Synch/VulkanFence.h"
#include <vector>
#include <mutex>


namespace Jimara { 
	namespace Graphics { 
		namespace Vulkan { 
			/// <summary>
			/// Represents a Vulkan-backed device queue
			/// </summary>
			class JIMARA_API VulkanDeviceQueue : public virtual DeviceQueue {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Device handle </param>
				/// <param name="queueFamilyId"> Queue family id </param>
				VulkanDeviceQueue(VkDeviceHandle* device, uint32_t queueFamilyId);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDeviceQueue();

				/// <summary> Device handle </summary>
				VkDeviceHandle* Device()const;

				/// <summary> Queue family id </summary>
				uint32_t FamilyId()const;

				/// <summary> Features, supported by the queue </summary>
				virtual FeatureBits Features()const override;

				/// <summary>
				/// Invokes vkQueueSubmit() safetly
				/// </summary>
				/// <param name="submitCount"> Number of elements within info </param>
				/// <param name="info"> Submit info </param>
				/// <param name="fence"> Fence to wait for </param>
				/// <returns> result of vkQueueSubmit </returns>
				VkResult Submit(size_t submitCount, const VkSubmitInfo* info, VulkanFence* fence = nullptr);

				/// <summary>
				/// Invokes vkQueueSubmit() safetly
				/// </summary>
				/// <param name="info"> Submit info </param>
				/// <param name="fence"> Fence to wait for </param>
				/// <returns> result of vkQueueSubmit </returns>
				inline VkResult Submit(const VkSubmitInfo& info, VulkanFence* fence = nullptr) { return Submit(1, &info, fence); }

				/// <summary>
				/// Invokes vkQueuePresentKHR() safetly
				/// </summary>
				/// <param name="presentInfo"> VkPresentInfoKHR argument for the call </param>
				/// <returns> result of vkQueuePresentKHR </returns>
				VkResult PresentKHR(const VkPresentInfoKHR& presentInfo);

				/// <summary> Invokes vkQueueWaitIdle() safetly </summary>
				VkResult WaitIdle()const;

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

				// Submition buffer lock
				mutable std::mutex m_lock;
			};
		} 
	} 
}
