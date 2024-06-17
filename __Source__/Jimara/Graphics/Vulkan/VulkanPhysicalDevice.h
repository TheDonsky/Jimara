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
			class JIMARA_API VulkanPhysicalDevice : public PhysicalDevice {
			public:
				/// <summary> Type cast to API object </summary>
				inline operator VkPhysicalDevice()const { return m_device; }

				/// <summary> Full Vulkan device features structure </summary>
				inline const VkPhysicalDeviceFeatures& DeviceFeatures()const { return m_deviceFeatures.features; }

				/// <summary> Full Vulkan device properties structure </summary>
				inline const VkPhysicalDeviceProperties& DeviceProperties()const { return m_deviceProperties.properties; }

				/// <summary> Full Vulkan 1.2 device features structure </summary>
				inline const VkPhysicalDeviceVulkan12Features& DeviceFeatures12()const { return m_deviceFeatures12; }

				/// <summary> Fragment shader interlock features </summary>
				inline const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT& InterlockFeatures()const { return m_interlockFeatures; }

				/// <summary> Full Vulkan device memory properties struct </summary>
				inline const VkPhysicalDeviceMemoryProperties& MemoryProperties()const { return m_memoryProps; }

				/// <summary> Ray-Tracing features </summary>
				struct RayTracingFeatures {
					VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure = {};
					VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT pipelineLibrary = {};
					VkPhysicalDeviceRayQueryFeaturesKHR rayQuery = {};
					VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipeline = {};
					VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProps = {};
					VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR maintenance1 = {};
					VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR positionFetch = {};
				};

				/// <summary> Ray-Tracing features </summary>
				inline const RayTracingFeatures& RTFeatures()const { return m_rtFeatures; }

				/// <summary> Number of available queue families </summary>
				inline size_t QueueFamilyCount()const { return m_queueFamilies.size(); }

				/// <summary>
				/// Queue family properties by index
				/// </summary>
				/// <param name="index"> Queue family index </param>
				/// <returns> Family properties </returns>
				inline const VkQueueFamilyProperties& QueueFamilyProperties(size_t index)const { return m_queueFamilies[index]; }

				/// <summary> Graphics queue id (does not have value if there's no graphics capability) </summary>
				inline std::optional<uint32_t> GraphicsQueueId()const { return m_queueIds.graphics; }

				/// <summary> Compute queue id (synchronous(same as GraphicsQueueId), if possible; does not have value if there's no compute capability) </summary>
				inline std::optional<uint32_t> ComputeQueueId()const { return m_queueIds.compute; }

				/// <summary> Number of available asynchronous compute queues </summary>
				inline size_t AsynchComputeQueueCount()const { return m_queueIds.asynchronous_compute.size(); }

				/// <summary>
				/// Asynchronous compute queue index(global) from asynchronous index(ei 0 - AsynchComputeQueueCount)
				/// </summary>
				/// <param name="asynchQueueId"> Asynchronous queue index (0 - AsynchComputeQueueCount) </param>
				/// <returns> Asynchronous compute queue index </returns>
				inline uint32_t AsynchComputeQueueId(size_t asynchQueueId)const { return m_queueIds.asynchronous_compute[asynchQueueId]; }

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
				virtual PhysicalDevice::DeviceFeatures Features()const override;

				/// <summary> Phisical device name/title </summary>
				virtual const char* Name()const override;

				/// <summary> Device VRAM(memory) capacity in bytes </summary>
				virtual size_t VramCapacity()const override;

				/// <summary> Maximal available Multisampling this device is capable of </summary>
				virtual Texture::Multisampling MaxMultisapling()const override;

				/// <summary> Maximal allowed RT-pipeline recursion depth (undefined if RT is not supported) </summary>
				virtual uint32_t MaxRTPipelineRecursionDepth()const override { return m_rtFeatures.rayTracingPipelineProps.maxRayRecursionDepth; }

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
				PhysicalDevice::DeviceFeatures m_features;
				
				// Device VRAM capacity
				size_t m_vramCapacity;

				// Vulkan-reported device features
				VkPhysicalDeviceFeatures2 m_deviceFeatures;
				VkPhysicalDeviceVulkan12Features m_deviceFeatures12;
				VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT m_interlockFeatures;
				RayTracingFeatures m_rtFeatures;

				// Vulkan-reported device properties
				VkPhysicalDeviceProperties2 m_deviceProperties;

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
