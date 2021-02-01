#include "VulkanDevice.h"
#include "Memory/Buffers/VulkanConstantBuffer.h"
#include "Memory/Buffers/VulkanDynamicBuffer.h"
#include "Memory/Textures/VulkanDynamicTexture.h"
#include "Pipeline/VulkanShader.h"
#include "Rendering/VulkanSurfaceRenderEngine.h"
#include <sstream>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
#ifndef NDEBUG
				inline static void LogDeviceInstantiateInfo(const VulkanDevice* device) {
					std::stringstream stream;
					stream << "Vulkan::VulkanDevice - Device Instantiated: " << std::endl
						<< "    NAME:           " << device->PhysicalDeviceInfo()->Name() << std::endl
						<< "    TYPE:           " << (
							device->PhysicalDeviceInfo()->Type() == PhysicalDevice::DeviceType::CPU ? "CPU" :
							device->PhysicalDeviceInfo()->Type() == PhysicalDevice::DeviceType::INTEGRATED ? "INTEGRATED" :
							device->PhysicalDeviceInfo()->Type() == PhysicalDevice::DeviceType::DESCRETE ? "DESCRETE" :
							device->PhysicalDeviceInfo()->Type() == PhysicalDevice::DeviceType::VIRTUAL ? "VIRTUAL" : "OTHER");

					stream << std::endl << "    GRAPHICS:       ";
					if (device->GraphicsQueue() != nullptr)
						stream << "YES <" << device->GraphicsQueue() << ">";
					else stream << "NO";

					stream << std::endl << "    COMPUTE:        ";
					if (device->ComputeQueue() != nullptr)
						stream << "YES <" << device->ComputeQueue() << ">";
					else stream << "NO";

					stream << std::endl << "    SYNCH_COMPUTE:  ";
					if (device->SynchComputeQueue() != nullptr)
						stream << "YES <" << device->SynchComputeQueue() << ">";
					else stream << "NO";

					stream << std::endl << "    ASYNCH_COMPUTE: x" << device->AsynchComputeQueueCount();
					if (device->AsynchComputeQueueCount() > 0) {
						stream << " [";
						for (size_t i = 0; i < device->AsynchComputeQueueCount(); i++)
							stream << device->AsynchComputeQueue(i) << ((i >= (device->AsynchComputeQueueCount() - 1)) ? "]" : "; ");
					}

					stream << std::endl << "    SWAP_CHAIN:     " <<
						(device->PhysicalDeviceInfo()->HasFeature(PhysicalDevice::DeviceFeature::SWAP_CHAIN) ? "YES" : "NO");

					stream << std::endl << "    VRAM:           " << device->PhysicalDeviceInfo()->VramCapacity() << " bytes" << std::endl;
					device->Log()->Info(stream.str());
				}
#endif
			}

			VulkanDevice::VulkanDevice(VulkanPhysicalDevice* physicalDevice) 
				: GraphicsDevice(physicalDevice)
				, m_device(VK_NULL_HANDLE)
				, m_graphicsQueue(VK_NULL_HANDLE)
				, m_primaryComputeQueue(VK_NULL_HANDLE), m_synchComputeQueue(VK_NULL_HANDLE)
				, m_memoryPool(nullptr) {
				
				// Specifying the queues to be created:
				float queuePriority = 1.0f;
				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				for (size_t i = 0; i < PhysicalDeviceInfo()->QueueFamilyCount(); i++) {
					VkDeviceQueueCreateInfo queueCreateInfo = {};
					queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(i);
					queueCreateInfo.queueCount = 1;
					queueCreateInfo.pQueuePriorities = &queuePriority;
					queueCreateInfos.push_back(queueCreateInfo);
				}

				// Specifying used device features:
				VkPhysicalDeviceFeatures deviceFeatures = {};
				{
					deviceFeatures.samplerAnisotropy = VK_TRUE;
					deviceFeatures.sampleRateShading = VK_TRUE;
				}

				// Creating the logical device:
				VkDeviceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pQueueCreateInfos = (queueCreateInfos.size() > 0 ? queueCreateInfos.data() : nullptr);
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				{
					if (PhysicalDeviceInfo()->DeviceExtensionVerison(VK_KHR_SWAPCHAIN_EXTENSION_NAME).has_value())
						m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
					createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
					createInfo.ppEnabledExtensionNames = (m_deviceExtensions.size() > 0 ? m_deviceExtensions.data() : nullptr);
				}
				{
					createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanAPIInstance()->ActiveValidationLayers().size());
					createInfo.ppEnabledLayerNames = VulkanAPIInstance()->ActiveValidationLayers().size() > 0 ? VulkanAPIInstance()->ActiveValidationLayers().data() : nullptr;
				}
				if (vkCreateDevice(*PhysicalDeviceInfo(), &createInfo, nullptr, &m_device) != VK_SUCCESS) {
					m_device = VK_NULL_HANDLE;
					throw std::runtime_error("VulkanDevice - Failed to create logical device");
				}

				// Retrieve queues:
				{
					if (PhysicalDeviceInfo()->GraphicsQueueId().has_value())
						vkGetDeviceQueue(m_device, PhysicalDeviceInfo()->GraphicsQueueId().value(), 0, &m_graphicsQueue);
					if (PhysicalDeviceInfo()->ComputeQueueId().has_value())
						vkGetDeviceQueue(m_device, PhysicalDeviceInfo()->ComputeQueueId().value(), 0, &m_primaryComputeQueue);
					for (size_t i = 0; i < PhysicalDeviceInfo()->AsynchComputeQueueCount(); i++) {
						VkQueue queue = VK_NULL_HANDLE;
						vkGetDeviceQueue(m_device, PhysicalDeviceInfo()->AsynchComputeQueueId(i), 0, &queue);
						if (queue != VK_NULL_HANDLE)
							m_asynchComputeQueues.push_back(queue);
					}
					m_synchComputeQueue = (m_graphicsQueue == m_primaryComputeQueue ? m_primaryComputeQueue : nullptr);
				}

				m_memoryPool = new VulkanMemoryPool(this);

#ifndef NDEBUG
				// Log creation status:
				LogDeviceInstantiateInfo(this);
#endif
			}

			VulkanDevice::~VulkanDevice() {
				if (m_device != VK_NULL_HANDLE)
					vkDeviceWaitIdle(m_device);
				if (m_memoryPool != nullptr) {
					delete m_memoryPool;
					m_memoryPool = nullptr;
				}
				if (m_device != VK_NULL_HANDLE) {
					vkDestroyDevice(m_device, nullptr);
					m_device = VK_NULL_HANDLE;
				}
			}

			VulkanInstance* VulkanDevice::VulkanAPIInstance()const { return dynamic_cast<VulkanInstance*>(GraphicsInstance()); }

			VulkanPhysicalDevice* VulkanDevice::PhysicalDeviceInfo()const { return dynamic_cast<VulkanPhysicalDevice*>(PhysicalDevice()); }

			VulkanDevice::operator VkDevice()const { return m_device; }

			VkQueue VulkanDevice::GraphicsQueue()const { return m_graphicsQueue; }

			VkQueue VulkanDevice::ComputeQueue()const { return m_primaryComputeQueue; }

			VkQueue VulkanDevice::SynchComputeQueue()const { return m_synchComputeQueue; }

			size_t VulkanDevice::AsynchComputeQueueCount()const { return m_asynchComputeQueues.size(); }

			VkQueue VulkanDevice::AsynchComputeQueue(size_t index)const { return m_asynchComputeQueues[index]; }

			VulkanMemoryPool* VulkanDevice::MemoryPool()const { return m_memoryPool; }

			Reference<RenderEngine> VulkanDevice::CreateRenderEngine(RenderSurface* targetSurface) {
				VulkanWindowSurface* windowSurface = dynamic_cast<VulkanWindowSurface*>(targetSurface);
				if (windowSurface != nullptr) return Object::Instantiate<VulkanSurfaceRenderEngine>(this, windowSurface);

				Log()->Warning("VulkanDevice - Target surface not of a known type");
				return nullptr;
			}

			Reference<ShaderCache> VulkanDevice::CreateShaderCache() {
				return Object::Instantiate<VulkanShaderCache>(this);
			}

			Reference<Buffer> VulkanDevice::CreateConstantBuffer(size_t size) {
				return Object::Instantiate<VulkanConstantBuffer>(size);
			}

			Reference<ArrayBuffer> VulkanDevice::CreateArrayBuffer(size_t objectSize, size_t objectCount, ArrayBuffer::CPUAccess cpuAccess) {
				if (cpuAccess == ArrayBuffer::CPUAccess::CPU_READ_WRITE) {
					Log()->Fatal("VulkanDevice - CPU_READ_WRITE capable buffers not yet implemented!");
					return nullptr;
				}
				else return Object::Instantiate<VulkanDynamicBuffer>(this, objectSize, objectCount);
			}

			Reference<ImageTexture> VulkanDevice::CreateTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps, ImageTexture::CPUAccess cpuAccess) {
				if (cpuAccess == ImageTexture::CPUAccess::CPU_READ_WRITE) {
					Log()->Fatal("VulkanDevice - CPU_READ_WRITE capable textures not yet implemented!");
					return nullptr;
				}
				else return Object::Instantiate<VulkanDynamicTexture>(this, type, format, size, arraySize, generateMipmaps);
			}
		}
	}
}
