#pragma once
#include "../GraphicsDevice.h"
#include "VulkanAPIIncludes.h"
#include "VulkanPhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan logical device(VkDevice) wrapper
			/// </summary>
			class VulkanDevice : public GraphicsDevice {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="physicalDevice"> Physical device </param>
				VulkanDevice(VulkanPhysicalDevice* physicalDevice);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDevice();

				/// <summary> Vulkan instance </summary>
				VulkanInstance* VulkanAPIInstance()const;

				/// <summary> Physical device </summary>
				VulkanPhysicalDevice* PhysicalDeviceInfo()const;

				/// <summary> Type cast to API object </summary>
				operator VkDevice()const;

				/// <summary> Graphics queue (VK_NULL_HANDLE if no graphics capabilities are present) </summary>
				VkQueue GraphicsQueue()const;

				/// <summary> Primary compute queue (same as GraphicsQueue if possible; VK_NULL_HANDLE if compute capabilities are missing) </summary>
				VkQueue ComputeQueue()const;

				/// <summary> Graphics-synchronous compute queue (same as GraphicsQueue if possible, VK_NULL_HANDLE otherwise) </summary>
				VkQueue SynchComputeQueue()const;

				/// <summary> Number of asynchronous compute queues </summary>
				size_t AsynchComputeQueueCount()const;

				/// <summary>
				/// Asynchronous compute queue by index
				/// </summary>
				/// <param name="index"> Queue index </param>
				/// <returns> ASynchronous compute queue </returns>
				VkQueue AsynchComputeQueue(size_t index)const;


			private:
				// Underlying API object
				VkDevice m_device;

				// Enabled device extensions
				std::vector<const char*> m_deviceExtensions;

				// Primary graphics queue (doubles as transfer/compute queue if possible)
				VkQueue m_graphicsQueue;

				// Primary compute queue (m_graphicsQueue if possible, otherwise m_asynchComputeQueues[0] if there is a valid compute queue)
				VkQueue m_primaryComputeQueue;

				// Synchronized compute queue (m_graphicsQueue or VK_NULL_HANDLE)
				VkQueue m_synchComputeQueue;

				// Asynchronous compute queues
				std::vector<VkQueue> m_asynchComputeQueues;
			};
		}
	}
}
