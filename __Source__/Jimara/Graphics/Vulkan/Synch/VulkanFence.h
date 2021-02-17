#pragma once
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> 
			/// A rather simple wrapper atop the regular VkFence
			/// (all it does is that it auto-constructs and auto-destructs underlying fence and makes sure the device does not go out of scope while this object is alive)
			/// </summary>
			class VulkanFence : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Vulkan device to create fence through </param>
				/// <param name="signalled"> If true, fence will allready be 'signalled' when instantiated </param>
				inline VulkanFence(VulkanDevice* device = nullptr, bool signalled = false) : m_device(*device), m_fence(VK_NULL_HANDLE) {
					if (m_device == nullptr) return;
					VkFenceCreateInfo fenceInfo = {};
					fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					if (signalled) fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
					if (vkCreateFence(*device, &fenceInfo, nullptr, &m_fence) != VK_SUCCESS)
						device->Log()->Fatal("VulkanFence - Failed to create fence!");
				}

				/// <summary> Destroys underlying VkFence object </summary>
				inline virtual ~VulkanFence() {
					if (m_fence != VK_NULL_HANDLE) {
						vkDestroyFence(*m_device, m_fence, nullptr);
						m_fence = VK_NULL_HANDLE;
					}
				}

				/// <summary>
				/// Move-constructor
				/// </summary>
				/// <param name="other"> Fence that's about to go out of scope </param>
				inline VulkanFence(VulkanFence&& other) noexcept
					: m_device(other.m_device), m_fence(other.m_fence) {
					other.m_fence = VK_NULL_HANDLE;
				}

				/// <summary>
				/// Move-assignment
				/// </summary>
				/// <param name="other"> Fence that's about to go out of scope </param>
				/// <returns> self </returns>
				inline VulkanFence& operator=(VulkanFence&& other) noexcept {
					if (m_fence != VK_NULL_HANDLE)
						vkDestroyFence(*m_device, m_fence, nullptr);
					m_device = other.m_device;
					m_fence = other.m_fence;
					other.m_fence = VK_NULL_HANDLE;
					return (*this);
				}

				/// <summary> Underlying Vulkan fence </summary>
				inline operator VkFence()const { return m_fence; }

				/// <summary> Underlying Vulkan fence </summary>
				inline VkFence Fence()const { return m_fence; }

				/// <summary> Waits for the fence indefinately </summary>
				inline void Wait()const { vkWaitForFences(*m_device, 1, &m_fence, VK_TRUE, UINT64_MAX); }

				/// <summary> Resets the fence </summary>
				inline void Reset()const { vkResetFences(*m_device, 1, &m_fence); }

				/// <summary> Waits for the fence indefinately and resets it afterwards </summary>
				inline void WaitAndReset()const { Wait(); Reset(); }



			private:
				// Device reference (just to keep it alive)
				Reference<VkDeviceHandle> m_device;

				// Underlying Vulkan fence
				VkFence m_fence;

				// Block copy-construction/assignment:
				inline VulkanFence(const VulkanFence&) = delete;
				inline VulkanFence& operator=(const VulkanFence&) = delete;
			};
		}
	}
}
