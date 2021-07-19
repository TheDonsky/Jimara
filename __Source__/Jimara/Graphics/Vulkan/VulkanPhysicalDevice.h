#pragma once
#include "VulkanAPIIncludes.h"
#include "../PhysicalDevice.h"
#include "VulkanInstance.h"
#include <optional>
#include <unordered_map>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan physical device descriptor
			/// </summary>
			class VulkanPhysicalDevice : public PhysicalDevice {
			public:
				/// <summary> Type cast to API object </summary>
				operator VkPhysicalDevice()const;

				/// <summary> Full Vulkan device features structure </summary>
				const VkPhysicalDeviceFeatures& DeviceFeatures()const;

				/// <summary> Full Vulkan device properties structure </summary>
				const VkPhysicalDeviceProperties& DeviceProperties()const;

				/// <summary> Full Vulkan device memory properties struct </summary>
				const VkPhysicalDeviceMemoryProperties& MemoryProperties()const;

				/// <summary> Number of available queue families </summary>
				size_t QueueFamilyCount()const;

				/// <summary>
				/// Queue family properties by index
				/// </summary>
				/// <param name="index"> Queue family index </param>
				/// <returns> Family properties </returns>
				const VkQueueFamilyProperties& QueueFamilyProperties(size_t index)const;

				/// <summary> Graphics queue id (does not have value if there's no graphics capability) </summary>
				std::optional<uint32_t> GraphicsQueueId()const;

				/// <summary> Compute queue id (synchronous(same as GraphicsQueueId), if possible; does not have value if there's no compute capability) </summary>
				std::optional<uint32_t> ComputeQueueId()const;

				/// <summary> Number of available asynchronous compute queues </summary>
				size_t AsynchComputeQueueCount()const;

				/// <summary>
				/// Asynchronous compute queue index(global) from asynchronous index(ei 0 - AsynchComputeQueueCount)
				/// </summary>
				/// <param name="asynchQueueId"> Asynchronous queue index (0 - AsynchComputeQueueCount) </param>
				/// <returns> Asynchronous compute queue index </returns>
				uint32_t AsynchComputeQueueId(size_t asynchQueueId)const;

				/// <summary> Device extension version (will not have value if extension is not supported by the device) </summary>
				std::optional<uint32_t> DeviceExtensionVerison(const std::string& extensionName)const;

				/// <summary>
				/// Number of MSAA samples, based on hardware, configuration and the render engine's specificity.
				/// </summary>
				/// <param name="desired"> Desired (configured) number of samples </param>
				/// <returns> Sample flag bits (Vulkan API format) </returns>
				VkSampleCountFlagBits SampleCountFlags(Texture::Multisampling desired)const;



				/// <summary> Physical device type </summary>
				virtual DeviceType Type()const override;

				/// <summary> Physical device features </summary>
				virtual uint64_t Features()const override;

				/// <summary> Phisical device name/title </summary>
				virtual const char* Name()const override;

				/// <summary> Device VRAM(memory) capacity in bytes </summary>
				virtual size_t VramCapacity()const override;

				/// <summary> Maximal available Multisampling this device is capable of </summary>
				virtual Texture::Multisampling MaxMultisapling()const override;

				/// <summary> Instantiates a logical device </summary>
				virtual Reference<GraphicsDevice> CreateLogicalDevice() override;



			private:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="instance"> Owning instance </param>
				/// <param name="device"> Physical device </param>
				/// <param name="index"> Physical device index </param>
				VulkanPhysicalDevice(VulkanInstance* instance, VkPhysicalDevice device, size_t index);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPhysicalDevice();

				// Only vulkan instance can create or delete VulkanPhysicalDevice-es
				friend class VulkanInstance;


				// Underlying physical device
				const VkPhysicalDevice m_device;

				// Underlying physical device index
				const size_t m_index;

				// Queue indices
				struct {
					// Main graphics queue
					std::optional<uint32_t> graphics;

					// Main compute queue (synchronous, if possible)
					std::optional<uint32_t> compute;

					// Asynchronous compute queue indices
					std::vector<uint32_t> asynchronous_compute;
				} m_queueIds;

				// Device type
				PhysicalDevice::DeviceType m_type;

				// Device features
				uint64_t m_features;
				
				// Device VRAM capacity
				size_t m_vramCapacity;

				// Vulkan-reported device features
				VkPhysicalDeviceFeatures m_deviceFeatures;

				// Vulkan-reported device properties
				VkPhysicalDeviceProperties m_deviceProperties;

				// Vulkan-reported device memory properties
				VkPhysicalDeviceMemoryProperties m_memoryProps;

				// Vulkan-reported device queue family properties
				std::vector<VkQueueFamilyProperties> m_queueFamilies;

				// Vulkan-reported device extensions
				std::unordered_map<std::string, uint32_t> m_availableExtensions;
			};
		}
	}
}
