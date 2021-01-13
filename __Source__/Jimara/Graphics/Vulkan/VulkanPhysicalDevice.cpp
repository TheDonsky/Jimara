#include "VulkanPhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* instance, VkPhysicalDevice device, size_t index)
				: PhysicalDevice(instance), m_device(device), m_index(index) {
				m_features = 0;

				// Device Features:
				vkGetPhysicalDeviceFeatures(device, &m_deviceFeatures);
				{
					if (m_deviceFeatures.samplerAnisotropy)
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
							m_vramCapacity = max(m_vramCapacity, static_cast<uint64_t>(heap.size));
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
				}
			}

			VulkanPhysicalDevice::~VulkanPhysicalDevice() {}

			VulkanPhysicalDevice::operator VkPhysicalDevice()const { return m_device; }

			const VkPhysicalDeviceFeatures& VulkanPhysicalDevice::DeviceFeatures()const { return m_deviceFeatures; }

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



			PhysicalDevice::DeviceType VulkanPhysicalDevice::Type()const { return m_type; }

			uint64_t VulkanPhysicalDevice::Features()const { return m_features; }

			const char* VulkanPhysicalDevice::Name()const { return m_deviceProperties.deviceName; }

			size_t VulkanPhysicalDevice::VramCapacity()const { return m_vramCapacity; }

			Reference<LogicalDevice> VulkanPhysicalDevice::CreateLogicalDevice() { return nullptr; }
		}
	}
}
