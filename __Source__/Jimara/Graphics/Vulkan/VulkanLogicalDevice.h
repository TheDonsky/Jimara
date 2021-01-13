#pragma once
#include "VulkanAPIIncludes.h"
#include "VulkanPhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanLogicalDevice : public LogicalDevice {
			public:
				VulkanLogicalDevice(VulkanPhysicalDevice* physicalDevice);

				virtual ~VulkanLogicalDevice();

				VulkanInstance* VulkanAPIInstance()const;

				VulkanPhysicalDevice* PhysicalDeviceInfo()const;

				operator VkDevice()const;

				VkQueue GraphicsQueue()const;

				VkQueue ComputeQueue()const;

				VkQueue SynchComputeQueue()const;

				size_t AsynchComputeQueueCount()const;

				VkQueue AsynchComputeQueue(size_t index)const;


			private:
				VkDevice m_device;

				std::vector<const char*> m_deviceExtensions;

				VkQueue m_graphicsQueue;
				VkQueue m_primaryComputeQueue;
				VkQueue m_synchComputeQueue;
				std::vector<VkQueue> m_asynchComputeQueues;
			};
		}
	}
}
