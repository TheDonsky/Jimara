#pragma once
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> 
			/// A rather simple wrapper atop the regular VkSemaphore 
			/// (all it does is that it auto-constructs and auto-destructs underlying semaphore and makes sure the device does not go out of scope while this object is alive)
			/// </summary>
			class VulkanSemaphore {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Vulkan device to create semaphore through </param>
				inline VulkanSemaphore(VulkanDevice* device = nullptr) : m_device(device), m_semaphore(VK_NULL_HANDLE) {
					if (m_device == nullptr) return;
					VkSemaphoreCreateInfo semaphoreInfo = {};
					semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &m_semaphore) != VK_SUCCESS)
						m_device->Log()->Fatal("VulkanSemaphore - Failed to create semaphore!");
				}

				/// <summary> Destroys underlying VkSemaphore object </summary>
				inline ~VulkanSemaphore() {
					if (m_semaphore != VK_NULL_HANDLE) {
						vkDestroySemaphore(*m_device, m_semaphore, nullptr);
						m_semaphore = VK_NULL_HANDLE;
					}
				}

				/// <summary>
				/// Move-constructor
				/// </summary>
				/// <param name="other"> Semaphore that's about to go out of scope </param>
				inline VulkanSemaphore(VulkanSemaphore&& other) noexcept
					: m_device(other.m_device), m_semaphore(other.m_semaphore) {
					other.m_semaphore = VK_NULL_HANDLE;
				}

				/// <summary>
				/// Move-assignment
				/// </summary>
				/// <param name="other"> Semaphore that's about to go out of scope </param>
				/// <returns> self </returns>
				inline VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept {
					if (m_semaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(*m_device, m_semaphore, nullptr);
					m_device = other.m_device;
					m_semaphore = other.m_semaphore;
					other.m_semaphore = VK_NULL_HANDLE;
					return (*this);
				}

				/// <summary> Underlying Vulkan semaphore </summary>
				inline operator VkSemaphore()const { return m_semaphore; }

				/// <summary> Underlying Vulkan semaphore </summary>
				inline VkSemaphore Semaphore()const { return m_semaphore; }



			private:
				// Device reference (just to keep it alive)
				Reference<VulkanDevice> m_device;

				// Underlying Vulkan semaphore
				VkSemaphore m_semaphore;

				// Block copy-construction/assignment:
				inline VulkanSemaphore(const VulkanSemaphore&) = delete;
				inline VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;
			};
		}
	}
}
