#pragma once
#include "VulkanAPIIncludes.h"
#include "../PhysicalDevice.h"
#include "VulkanInstance.h"
#include <optional>
#include <unordered_map>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanPhysicalDevice : public PhysicalDevice {
			public:
				VulkanPhysicalDevice(VulkanInstance* instance, VkPhysicalDevice device, size_t index);

				virtual ~VulkanPhysicalDevice();

				operator VkPhysicalDevice()const;

				const VkPhysicalDeviceFeatures& DeviceFeatures()const;

				const VkPhysicalDeviceProperties& DeviceProperties()const;

				const VkPhysicalDeviceMemoryProperties& MemoryProperties()const;

				size_t QueueFamilyCount()const;

				const VkQueueFamilyProperties& QueueFamilyProperties(size_t index)const;

				std::optional<uint32_t> GraphicsQueueId()const;

				std::optional<uint32_t> ComputeQueueId()const;

				size_t AsynchComputeQueueCount()const;

				uint32_t AsynchComputeQueueId(size_t asynchQueueId)const;

				std::optional<uint32_t> DeviceExtensionVerison(const std::string& extensionName)const;

				/// <summary> Physical device type </summary>
				virtual DeviceType Type()const override;

				/// <summary> Physical device features </summary>
				virtual uint64_t Features()const override;

				/// <summary> Phisical device name/title </summary>
				virtual const char* Name()const override;

				/// <summary> Device VRAM(memory) capacity in bytes </summary>
				virtual size_t VramCapacity()const override;

				/// <summary> Instantiates a logical device </summary>
				virtual Reference<LogicalDevice> CreateLogicalDevice() override;

			private:
				const VkPhysicalDevice m_device;
				const size_t m_index;

				struct {
					std::optional<uint32_t> graphics;
					std::optional<uint32_t> compute;
					std::vector<uint32_t> asynchronous_compute;
				} m_queueIds;

				PhysicalDevice::DeviceType m_type;
				uint64_t m_features;
				size_t m_vramCapacity;

				VkPhysicalDeviceFeatures m_deviceFeatures;
				VkPhysicalDeviceProperties m_deviceProperties;
				VkPhysicalDeviceMemoryProperties m_memoryProps;
				std::vector<VkQueueFamilyProperties> m_queueFamilies;
				std::unordered_map<std::string, uint32_t> m_availableExtensions;
			};
		}
	}
}
