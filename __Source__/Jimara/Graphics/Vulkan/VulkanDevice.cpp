#include "VulkanDevice.h"
#include "Memory/Buffers/VulkanConstantBuffer.h"
#include "Memory/Buffers/VulkanDynamicBuffer.h"
#include "Memory/Textures/VulkanDynamicTexture.h"
#include "Pipeline/VulkanShader.h"
#include "Pipeline/VulkanPipeline.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanDeviceQueue.h"
#include "Rendering/VulkanSurfaceRenderEngine.h"
#include <sstream>

#pragma warning(disable: 26812)
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

			VkDeviceHandle::VkDeviceHandle(VulkanPhysicalDevice* physicalDevice) 
				: m_physicalDevice(physicalDevice), m_device(VK_NULL_HANDLE) {
				// Specifying the queues to be created:
				float queuePriority = 1.0f;
				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				for (size_t i = 0; i < m_physicalDevice->QueueFamilyCount(); i++) {
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
					deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;
				}
				VkPhysicalDeviceVulkan12Features device12Features = {};
				{
					device12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
					device12Features.timelineSemaphore = VK_TRUE;
				}

				// Creating the logical device:
				VkDeviceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pNext = &device12Features;
				createInfo.pQueueCreateInfos = (queueCreateInfos.size() > 0 ? queueCreateInfos.data() : nullptr);
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				{
					if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_SWAPCHAIN_EXTENSION_NAME).has_value())
						m_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
					m_deviceExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
					m_deviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
					createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
					createInfo.ppEnabledExtensionNames = (m_deviceExtensions.size() > 0 ? m_deviceExtensions.data() : nullptr);
				}
				{
					createInfo.enabledLayerCount = static_cast<uint32_t>(dynamic_cast<VulkanInstance*>(m_physicalDevice->GraphicsInstance())->ActiveValidationLayers().size());
					createInfo.ppEnabledLayerNames = dynamic_cast<VulkanInstance*>(m_physicalDevice->GraphicsInstance())->ActiveValidationLayers().size() > 0
						? dynamic_cast<VulkanInstance*>(m_physicalDevice->GraphicsInstance())->ActiveValidationLayers().data() : nullptr;
				}
				if (vkCreateDevice(*m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
					m_device = VK_NULL_HANDLE;
					m_physicalDevice->Log()->Fatal("VulkanDevice - Failed to create logical device");
				}
			}

			VkDeviceHandle::~VkDeviceHandle() {
				if (m_device != VK_NULL_HANDLE)
					vkDeviceWaitIdle(m_device);
				if (m_device != VK_NULL_HANDLE) {
					vkDestroyDevice(m_device, nullptr);
					m_device = VK_NULL_HANDLE;
				}
			}

			VulkanDevice::VulkanDevice(VulkanPhysicalDevice* physicalDevice) 
				: GraphicsDevice(physicalDevice)
				, m_device(Object::Instantiate<VkDeviceHandle>(physicalDevice))
				, m_graphicsQueue(VK_NULL_HANDLE)
				, m_primaryComputeQueue(VK_NULL_HANDLE), m_synchComputeQueue(VK_NULL_HANDLE)
				, m_memoryPool(nullptr) {
				// Retrieve queues:
				{
					const std::optional<uint32_t> MAIN_GRAPHICS_QUEUE = m_device->PhysicalDevice()->GraphicsQueueId();
					if (MAIN_GRAPHICS_QUEUE.has_value())
						m_graphicsQueue = Object::Instantiate<VulkanDeviceQueue>(m_device, MAIN_GRAPHICS_QUEUE.value());
					const std::optional<uint32_t> MAIN_COMPUTE_QUEUE = m_device->PhysicalDevice()->ComputeQueueId();
					if (MAIN_COMPUTE_QUEUE.has_value()) {
						if (MAIN_GRAPHICS_QUEUE.has_value() && MAIN_GRAPHICS_QUEUE.value() == MAIN_COMPUTE_QUEUE.value())
							m_synchComputeQueue = m_primaryComputeQueue = m_graphicsQueue;
						else m_primaryComputeQueue = Object::Instantiate<VulkanDeviceQueue>(m_device, MAIN_COMPUTE_QUEUE.value());
					}
					for (size_t i = 0; i < m_device->PhysicalDevice()->AsynchComputeQueueCount(); i++)
						m_asynchComputeQueues.push_back(Object::Instantiate<VulkanDeviceQueue>(m_device, m_device->PhysicalDevice()->AsynchComputeQueueId(i)));
				}

				m_memoryPool = new VulkanMemoryPool(this);

#ifndef NDEBUG
				// Log creation status:
				LogDeviceInstantiateInfo(this);
#endif
			}

			VulkanDevice::~VulkanDevice() {
				if (m_device != VK_NULL_HANDLE)
					vkDeviceWaitIdle(*m_device);
				if (m_memoryPool != nullptr) {
					delete m_memoryPool;
					m_memoryPool = nullptr;
				}
			}

			VulkanInstance* VulkanDevice::VulkanAPIInstance()const { return dynamic_cast<VulkanInstance*>(GraphicsInstance()); }

			VulkanPhysicalDevice* VulkanDevice::PhysicalDeviceInfo()const { return m_device->PhysicalDevice(); }

			VulkanDevice::operator VkDevice()const { return *m_device; }
			
			VulkanDevice::operator VkDeviceHandle* ()const { return m_device; }

			DeviceQueue* VulkanDevice::GraphicsQueue()const { return m_graphicsQueue; }

			VkQueue VulkanDevice::MainGraphicsQueue()const { return *dynamic_cast<VulkanDeviceQueue*>(m_graphicsQueue.operator->()); }

			VkQueue VulkanDevice::ComputeQueue()const { return *dynamic_cast<VulkanDeviceQueue*>(m_primaryComputeQueue.operator->()); }

			VkQueue VulkanDevice::SynchComputeQueue()const { return *dynamic_cast<VulkanDeviceQueue*>(m_synchComputeQueue.operator->()); }

			size_t VulkanDevice::AsynchComputeQueueCount()const { return m_asynchComputeQueues.size(); }

			VkQueue VulkanDevice::AsynchComputeQueue(size_t index)const { return *dynamic_cast<VulkanDeviceQueue*>(m_asynchComputeQueues[index].operator->()); }

			VulkanMemoryPool* VulkanDevice::MemoryPool()const { return m_memoryPool; }

			Reference<RenderEngine> VulkanDevice::CreateRenderEngine(RenderSurface* targetSurface) {
				VulkanWindowSurface* windowSurface = dynamic_cast<VulkanWindowSurface*>(targetSurface);
				if (windowSurface != nullptr) return Object::Instantiate<VulkanSurfaceRenderEngine>(this, windowSurface);

				Log()->Warning("VulkanDevice - Target surface not of a known type");
				return nullptr;
			}

			Reference<Shader> VulkanDevice::CreateShader(const SPIRV_Binary* bytecode) {
				return Object::Instantiate<VulkanShader>(this, bytecode);
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

			Reference<Texture> VulkanDevice::CreateMultisampledTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) {
				return Object::Instantiate<VulkanStaticTexture>(this, type, format, size, arraySize, false
					, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
					| ((sampleCount <= Texture::Multisampling::SAMPLE_COUNT_1) ? VK_IMAGE_USAGE_STORAGE_BIT : 0)
					| ((format >= Texture::PixelFormat::FIRST_DEPTH_FORMAT && format <= Texture::PixelFormat::LAST_DEPTH_FORMAT)
						? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
					, sampleCount);
			}

			Texture::PixelFormat VulkanDevice::GetDepthFormat() {
				auto depthFormatViable = [&](Texture::PixelFormat format) {
					VkFormat vulkanFormat = VulkanImage::NativeFormatFromPixelFormat(format);
					VkFormatProperties props;
					vkGetPhysicalDeviceFormatProperties(*m_device->PhysicalDevice(), vulkanFormat, &props);
					return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
				};
				static const Texture::PixelFormat DEPTH_FORMATS[] = {
					Texture::PixelFormat::D32_SFLOAT,
					Texture::PixelFormat::D24_UNORM_S8_UINT,
					Texture::PixelFormat::D32_SFLOAT_S8_UINT
				};
				for (size_t i = 0; i < (sizeof(DEPTH_FORMATS) / sizeof(Texture::PixelFormat)); i++) {
					Texture::PixelFormat format = DEPTH_FORMATS[i];
					if (depthFormatViable(format)) return format;
				}
				return Texture::PixelFormat::OTHER;
			}

			Reference<RenderPass> VulkanDevice::CreateRenderPass(Texture::Multisampling sampleCount
				, size_t numColorAttachments, Texture::PixelFormat* colorAttachmentFormats
				, Texture::PixelFormat depthFormat, bool includeResolveAttachments) {
				return Object::Instantiate<VulkanRenderPass>(this, sampleCount, numColorAttachments, colorAttachmentFormats, depthFormat, includeResolveAttachments);
			}

			Reference<Pipeline> VulkanDevice::CreateEnvironmentPipeline(PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) {
				static const VkPipelineBindPoint BIND_POINTS[] = { VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE };
				return Object::Instantiate<VulkanEnvironmentPipeline>(this, descriptor, maxInFlightCommandBuffers, sizeof(BIND_POINTS) / sizeof(VkPipelineBindPoint), BIND_POINTS);
			}
		}
	}
}
#pragma warning(default: 26812)
