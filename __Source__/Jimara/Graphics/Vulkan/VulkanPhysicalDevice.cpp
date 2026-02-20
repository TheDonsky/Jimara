#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* instance, VkPhysicalDevice device, size_t index)
				: PhysicalDevice(instance), m_device(device), m_index(index) {
				m_features = PhysicalDevice::DeviceFeatures::NONE;

				// Extension support:
				{
					uint32_t extensionCount;
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
					std::vector<VkExtensionProperties> availableExtensions(extensionCount);
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
					for (size_t i = 0; i < availableExtensions.size(); i++) {
						const VkExtensionProperties& extension = availableExtensions[i];
						m_availableExtensions[extension.extensionName] = extension.specVersion;
					}
					if (DeviceExtensionVerison(VK_KHR_SWAPCHAIN_EXTENSION_NAME).has_value())
						m_features |= PhysicalDevice::DeviceFeatures::SWAP_CHAIN;
					if (DeviceExtensionVerison(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME).has_value())
						m_features |= PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK;
				}

				// Device Features:
				{
					m_deviceFeatures = {};
					m_deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
					
					void** pNext = &m_deviceFeatures.pNext;
					auto setNext = [&](auto* next) {
						(*pNext) = next;
						pNext = &next->pNext;
					};

					m_deviceFeatures12 = {};
					m_deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
					setNext(&m_deviceFeatures12);

					m_interlockFeatures = {};
					m_interlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
					setNext(&m_interlockFeatures);

					m_rtFeatures.accelerationStructure.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
					setNext(&m_rtFeatures.accelerationStructure);

					m_rtFeatures.pipelineLibrary.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT;
					setNext(&m_rtFeatures.pipelineLibrary);

					m_rtFeatures.rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
					setNext(&m_rtFeatures.rayQuery);

					m_rtFeatures.rayTracingPipeline.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
					setNext(&m_rtFeatures.rayTracingPipeline);

					m_rtFeatures.maintenance1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR;
					setNext(&m_rtFeatures.maintenance1);

					m_rtFeatures.positionFetch.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
					setNext(&m_rtFeatures.positionFetch);

					vkGetPhysicalDeviceFeatures2(device, &m_deviceFeatures);

					if (m_deviceFeatures.features.samplerAnisotropy)
						m_features |= PhysicalDevice::DeviceFeatures::SAMPLER_ANISOTROPY;

					if (DeviceExtensionVerison(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME).has_value() &&
						m_rtFeatures.accelerationStructure.accelerationStructure &&
						DeviceExtensionVerison(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME).has_value() &&
						m_rtFeatures.pipelineLibrary.graphicsPipelineLibrary &&
						DeviceExtensionVerison(VK_KHR_RAY_QUERY_EXTENSION_NAME).has_value() &&
						m_rtFeatures.rayQuery.rayQuery &&
						DeviceExtensionVerison(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME).has_value() &&
						m_rtFeatures.rayTracingPipeline.rayTracingPipeline &&
						m_rtFeatures.rayTracingPipeline.rayTraversalPrimitiveCulling &&
						DeviceExtensionVerison(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME).has_value() &&
						m_rtFeatures.positionFetch.rayTracingPositionFetch)
						m_features |= PhysicalDevice::DeviceFeatures::RAY_TRACING;
				}

				// Device properties:
				{
					m_deviceProperties = {};
					m_deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

					m_rtFeatures.rayTracingPipelineProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
					m_deviceProperties.pNext = &m_rtFeatures.rayTracingPipelineProps;

					vkGetPhysicalDeviceProperties2(device, &m_deviceProperties);
					m_type =
						(m_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) ? PhysicalDevice::DeviceType::CPU :
						(m_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) ? PhysicalDevice::DeviceType::INTEGRATED :
						(m_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? PhysicalDevice::DeviceType::DESCRETE :
						(m_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) ? PhysicalDevice::DeviceType::VIRTUAL : PhysicalDevice::DeviceType::OTHER;
				}

				// Device memory properties:
				{
					vkGetPhysicalDeviceMemoryProperties(device, &m_memoryProps);
					m_vramCapacity = 0;
					for (uint32_t heapId = 0; heapId < m_memoryProps.memoryHeapCount; heapId++) {
						const VkMemoryHeap& heap = m_memoryProps.memoryHeaps[heapId];
						if (heap.flags & VkMemoryHeapFlagBits::VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
							m_vramCapacity = std::max(m_vramCapacity, static_cast<size_t>(heap.size));
					}
				}

				// Queue families:
				{
					uint32_t queueFamilyCount = 0;
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
					m_queueFamilies.resize(queueFamilyCount);
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, m_queueFamilies.data());

					for (uint32_t queueFamilyId = 0; queueFamilyId < queueFamilyCount; queueFamilyId++) {
						const VkQueueFamilyProperties& family = m_queueFamilies[queueFamilyId];

						bool hasGraphics = ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0);
						bool hasCompute = ((family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0);

						bool foundGraphicsBit = (hasGraphics && ((!m_queueIds.graphics.has_value()) || ((!m_queueIds.compute.has_value()) && hasCompute)));
						if (foundGraphicsBit) m_queueIds.graphics = queueFamilyId;
						if (hasCompute) {
							if (foundGraphicsBit) m_queueIds.compute = queueFamilyId;
							else m_queueIds.asynchronous_compute.push_back(queueFamilyId);
						}
					}
					if ((!m_queueIds.compute.has_value()) && (m_queueIds.asynchronous_compute.size() > 0))
						m_queueIds.compute = m_queueIds.asynchronous_compute[0];

					if (m_queueIds.graphics.has_value())
						m_features |= PhysicalDevice::DeviceFeatures::GRAPHICS;
					if (m_queueIds.compute.has_value()) {
						m_features |= PhysicalDevice::DeviceFeatures::COMPUTE;
						if (m_queueIds.graphics.has_value() && (m_queueIds.graphics == m_queueIds.compute))
							m_features |= PhysicalDevice::DeviceFeatures::SYNCHRONOUS_COMPUTE;
						if (m_queueIds.asynchronous_compute.size() > 0)
							m_features |= PhysicalDevice::DeviceFeatures::ASYNCHRONOUS_COMPUTE;
					}
				}
			}

			VulkanPhysicalDevice::~VulkanPhysicalDevice() {}

			std::optional<uint32_t> VulkanPhysicalDevice::DeviceExtensionVerison(const std::string& extensionName)const {
				std::unordered_map<std::string, uint32_t>::const_iterator it = m_availableExtensions.find(extensionName);
				return (it == m_availableExtensions.end() ? std::optional<uint32_t>() : it->second);
			}

			VkSampleCountFlagBits VulkanPhysicalDevice::SampleCountFlags(Texture::Multisampling desired)const {
				if (!static_cast<bool>(DeviceFeatures().shaderStorageImageMultisample)) 
					return Texture::Multisampling::SAMPLE_COUNT_1;
				VkSampleCountFlags counts = m_deviceProperties.properties.limits.framebufferColorSampleCounts & m_deviceProperties.properties.limits.framebufferDepthSampleCounts;
				if (desired >= Texture::Multisampling::SAMPLE_COUNT_64 && ((counts & VK_SAMPLE_COUNT_64_BIT) != 0)) return VK_SAMPLE_COUNT_64_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_32 && ((counts & VK_SAMPLE_COUNT_32_BIT) != 0)) return VK_SAMPLE_COUNT_32_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_16 && ((counts & VK_SAMPLE_COUNT_16_BIT) != 0)) return VK_SAMPLE_COUNT_16_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_8 && ((counts & VK_SAMPLE_COUNT_8_BIT) != 0)) return VK_SAMPLE_COUNT_8_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_4 && ((counts & VK_SAMPLE_COUNT_4_BIT) != 0)) return VK_SAMPLE_COUNT_4_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_2 && ((counts & VK_SAMPLE_COUNT_2_BIT) != 0)) return VK_SAMPLE_COUNT_2_BIT;
				else return VK_SAMPLE_COUNT_1_BIT;
			}



			PhysicalDevice::DeviceType VulkanPhysicalDevice::Type()const { return m_type; }

			PhysicalDevice::DeviceFeatures VulkanPhysicalDevice::Features()const { return m_features; }

			const char* VulkanPhysicalDevice::Name()const { return m_deviceProperties.properties.deviceName; }

			size_t VulkanPhysicalDevice::VramCapacity()const { return m_vramCapacity; }

			Texture::Multisampling VulkanPhysicalDevice::MaxMultisapling()const {
				if (!static_cast<bool>(DeviceFeatures().shaderStorageImageMultisample)) 
					return Texture::Multisampling::SAMPLE_COUNT_1;
				VkSampleCountFlags counts = m_deviceProperties.properties.limits.framebufferColorSampleCounts & m_deviceProperties.properties.limits.framebufferDepthSampleCounts;
				if (((counts & VK_SAMPLE_COUNT_64_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_64;
				else if (((counts & VK_SAMPLE_COUNT_32_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_32;
				else if (((counts & VK_SAMPLE_COUNT_16_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_16;
				else if (((counts & VK_SAMPLE_COUNT_8_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_8;
				else if (((counts & VK_SAMPLE_COUNT_4_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_4;
				if (((counts & VK_SAMPLE_COUNT_2_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_2;
				if (((counts & VK_SAMPLE_COUNT_1_BIT) != 0)) return Texture::Multisampling::SAMPLE_COUNT_1;
				else {
						Log()->Fatal("VulkanPhysicalDevice::MaxMultisapling - Internal Error! SAMPLE_COUNT_1 not supported!");
						return Texture::Multisampling::SAMPLE_COUNT_1;
					}
			}

			Reference<GraphicsDevice> VulkanPhysicalDevice::CreateLogicalDevice() { return Object::Instantiate<VulkanDevice>(this); }
		}
	}
}
#pragma warning(default: 26812)
