#pragma once
#include "../VulkanDevice.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Wrapper on top of a Vulkan Timeline Semaphore
			/// </summary>
			class VulkanTimelineSemaphore : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Device handle </param>
				/// <param name="initialValue"> Initial value </param>
				VulkanTimelineSemaphore(VkDeviceHandle* device, uint64_t initialValue = 0)
					: m_device(device), m_semaphore([&]() -> VkSemaphore {
					VkSemaphoreTypeCreateInfo timelineCreateInfo = {};
					{
						timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
						timelineCreateInfo.pNext = nullptr;
						timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
						timelineCreateInfo.initialValue = initialValue;
					}
					VkSemaphoreCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
						createInfo.pNext = &timelineCreateInfo;
						createInfo.flags = 0;
					}
					VkSemaphore semaphore;
					if (vkCreateSemaphore(*device, &createInfo, nullptr, &semaphore) != VK_SUCCESS) {
						semaphore = VK_NULL_HANDLE;
						device->Log()->Fatal("VulkanTimelineSemaphore - Failed to create semaphore!");
					}
					return semaphore;
						}()) {
				}

				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanTimelineSemaphore() {
					if (m_semaphore != VK_NULL_HANDLE)
						vkDestroySemaphore(*m_device, m_semaphore, nullptr);
				}

				/// <summary> Type cast to API object </summary>
				inline operator VkSemaphore()const { return m_semaphore; }

				/// <summary>
				/// Waits for the semaphore
				/// </summary>
				/// <param name="count"> Counter value to wait for </param>
				/// <param name="timeoutNanoseconds"> Timeout in nanoseconds </param>
				/// <returns> True, if counter reached count </returns>
				inline bool Wait(uint64_t count, uint64_t timeoutNanoseconds)const {
					VkSemaphoreWaitInfo info = {};
					{
						info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
						info.pNext = nullptr;
						info.flags = 0;
						info.semaphoreCount = 1;
						info.pSemaphores = &m_semaphore;
						info.pValues = &count;
					}
					return dynamic_cast<const VulkanInstance*>(m_device->PhysicalDevice()->GraphicsInstance())
						->ProcedureAddresses()->waitSemaphores(*m_device, &info, timeoutNanoseconds) == VK_SUCCESS;
				}

				/// <summary>
				/// Waits for the semaphore indefinately (till the counter reaches given number)
				/// </summary>
				/// <param name="count"> Counter value to wait for </param>
				inline void Wait(uint64_t count) {
					while (!Wait(count, UINT64_MAX));
				}

				/// <summary>
				/// Signals semaphore from CPU
				/// </summary>
				/// <param name="count"> Counter to set </param>
				inline void Signal(uint64_t count)const {
					VkSemaphoreSignalInfo info = {};
					{
						info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
						info.pNext = nullptr;
						info.semaphore = m_semaphore;
						info.value = count;
					}
					dynamic_cast<const VulkanInstance*>(m_device->PhysicalDevice()->GraphicsInstance())
						->ProcedureAddresses()->signalSemaphore(*m_device, &info);
				}

				/// <summary> Current counter value </summary>
				inline uint64_t Count()const {
					uint64_t count;
					dynamic_cast<const VulkanInstance*>(m_device->PhysicalDevice()->GraphicsInstance())
						->ProcedureAddresses()->getSemaphoreCounterValue(*m_device, m_semaphore, &count);
					return count;
				}


			private:
				// "Owner" device handle
				const Reference<VkDeviceHandle> m_device;

				// Underlying API object
				const VkSemaphore m_semaphore;
			};
		}
	}
}
