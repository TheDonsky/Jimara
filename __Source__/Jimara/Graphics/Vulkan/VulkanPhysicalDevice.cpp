#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* instance, VkPhysicalDevice device, size_t index)
				: PhysicalDevice(instance), m_device(device), m_index(index) {
				m_features = 0;

				// Device Features:
				{
					m_deviceFeatures = {};
					m_deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
					
					m_deviceFeatures12 = {};
					m_deviceFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
					m_deviceFeatures.pNext = &m_deviceFeatures12;

					m_interlockFeatures = {};
					m_interlockFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
					m_deviceFeatures12.pNext = &m_interlockFeatures;

					vkGetPhysicalDeviceFeatures2(device, &m_deviceFeatures);
					if (m_deviceFeatures.features.samplerAnisotropy)
						m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::SAMPLER_ANISOTROPY);
				}

				// Device properties:
				{
					vkGetPhysicalDeviceProperties(device, &m_deviceProperties);
					m_type =
						(m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) ? PhysicalDevice::DeviceType::CPU :
						(m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) ? PhysicalDevice::DeviceType::INTEGRATED :
						(m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? PhysicalDevice::DeviceType::DESCRETE :
						(m_deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) ? PhysicalDevice::DeviceType::VIRTUAL : PhysicalDevice::DeviceType::OTHER;
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
						m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::GRAPHICS);
					if (m_queueIds.compute.has_value()) {
						m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::COMPUTE);
						if (m_queueIds.graphics.has_value() && (m_queueIds.graphics == m_queueIds.compute))
							m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::SYNCHRONOUS_COMPUTE);
						if (m_queueIds.asynchronous_compute.size() > 0)
							m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE);
					}
				}

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
						m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::SWAP_CHAIN);
					if (DeviceExtensionVerison(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME).has_value())
						m_features |= static_cast<uint64_t>(PhysicalDevice::DeviceFeature::FRAGMENT_SHADER_INTERLOCK);
				}
			}

			VulkanPhysicalDevice::~VulkanPhysicalDevice() {}

			VulkanPhysicalDevice::operator VkPhysicalDevice()const { return m_device; }

			const VkPhysicalDeviceFeatures& VulkanPhysicalDevice::DeviceFeatures()const { return m_deviceFeatures.features; }

			const VkPhysicalDeviceVulkan12Features& VulkanPhysicalDevice::DeviceFeatures12()const { return m_deviceFeatures12; }

			const VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT& VulkanPhysicalDevice::InterlockFeatures()const { return m_interlockFeatures; }

			const VkPhysicalDeviceProperties& VulkanPhysicalDevice::DeviceProperties()const { return m_deviceProperties; }

			const VkPhysicalDeviceMemoryProperties& VulkanPhysicalDevice::MemoryProperties()const { return m_memoryProps; }

			size_t VulkanPhysicalDevice::QueueFamilyCount()const { return m_queueFamilies.size(); }

			const VkQueueFamilyProperties& VulkanPhysicalDevice::QueueFamilyProperties(size_t index)const { return m_queueFamilies[index]; }

			std::optional<uint32_t> VulkanPhysicalDevice::GraphicsQueueId()const { return m_queueIds.graphics; }

			std::optional<uint32_t> VulkanPhysicalDevice::ComputeQueueId()const { return m_queueIds.compute; }

			size_t VulkanPhysicalDevice::AsynchComputeQueueCount()const { return m_queueIds.asynchronous_compute.size(); }

			uint32_t VulkanPhysicalDevice::AsynchComputeQueueId(size_t asynchQueueId)const { return m_queueIds.asynchronous_compute[asynchQueueId]; }

			std::optional<uint32_t> VulkanPhysicalDevice::DeviceExtensionVerison(const std::string& extensionName)const {
				std::unordered_map<std::string, uint32_t>::const_iterator it = m_availableExtensions.find(extensionName);
				return (it == m_availableExtensions.end() ? std::optional<uint32_t>() : it->second);
			}

			VkSampleCountFlagBits VulkanPhysicalDevice::SampleCountFlags(Texture::Multisampling desired)const {
				VkSampleCountFlags counts = m_deviceProperties.limits.framebufferColorSampleCounts & m_deviceProperties.limits.framebufferDepthSampleCounts;
				if (desired >= Texture::Multisampling::SAMPLE_COUNT_64 && ((counts & VK_SAMPLE_COUNT_64_BIT) != 0)) return VK_SAMPLE_COUNT_64_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_32 && ((counts & VK_SAMPLE_COUNT_32_BIT) != 0)) return VK_SAMPLE_COUNT_32_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_16 && ((counts & VK_SAMPLE_COUNT_16_BIT) != 0)) return VK_SAMPLE_COUNT_16_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_8 && ((counts & VK_SAMPLE_COUNT_8_BIT) != 0)) return VK_SAMPLE_COUNT_8_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_4 && ((counts & VK_SAMPLE_COUNT_4_BIT) != 0)) return VK_SAMPLE_COUNT_4_BIT;
				else if (desired >= Texture::Multisampling::SAMPLE_COUNT_2 && ((counts & VK_SAMPLE_COUNT_2_BIT) != 0)) return VK_SAMPLE_COUNT_2_BIT;
				else return VK_SAMPLE_COUNT_1_BIT;
			}



			PhysicalDevice::DeviceType VulkanPhysicalDevice::Type()const { return m_type; }

			uint64_t VulkanPhysicalDevice::Features()const { return m_features; }

			const char* VulkanPhysicalDevice::Name()const { return m_deviceProperties.deviceName; }

			size_t VulkanPhysicalDevice::VramCapacity()const { return m_vramCapacity; }

			Texture::Multisampling VulkanPhysicalDevice::MaxMultisapling()const {
				VkSampleCountFlags counts = m_deviceProperties.limits.framebufferColorSampleCounts & m_deviceProperties.limits.framebufferDepthSampleCounts;
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
