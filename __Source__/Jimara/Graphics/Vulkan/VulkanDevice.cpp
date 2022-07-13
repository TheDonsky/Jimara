#include "VulkanDevice.h"
#include "Memory/Buffers/VulkanConstantBuffer.h"
#include "Memory/Buffers/VulkanCpuWriteOnlyBuffer.h"
#include "Memory/Textures/VulkanCpuWriteOnlyTexture.h"
#include "Pipeline/VulkanShader.h"
#include "Pipeline/VulkanPipeline.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanDeviceQueue.h"
#include "Pipeline/VulkanComputePipeline.h"
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
						stream << "YES <" << device->GraphicsQueue() << "; familyId=" << device->PhysicalDeviceInfo()->GraphicsQueueId().value() << ">";
					else stream << "NO";

					stream << std::endl << "    COMPUTE:        ";
					if (device->PhysicalDeviceInfo()->ComputeQueueId().has_value())
						stream << "YES <familyId=" << device->PhysicalDeviceInfo()->ComputeQueueId().value() << ">";
					else stream << "NO";

					stream << std::endl << "    SYNCH_COMPUTE:  ";
					if (device->PhysicalDeviceInfo()->ComputeQueueId().has_value() && (device->GraphicsQueue() != nullptr) 
						&& (device->PhysicalDeviceInfo()->ComputeQueueId().value() == device->PhysicalDeviceInfo()->GraphicsQueueId().value()))
						stream << "YES <familyId=" << device->PhysicalDeviceInfo()->ComputeQueueId().value() << ">";
					else stream << "NO";

					stream << std::endl << "    ASYNCH_COMPUTE: x" << device->PhysicalDeviceInfo()->AsynchComputeQueueCount();
					if (device->PhysicalDeviceInfo()->AsynchComputeQueueCount() > 0) {
						stream << " [";
						for (size_t i = 0; i < device->PhysicalDeviceInfo()->AsynchComputeQueueCount(); i++)
							stream << device->PhysicalDeviceInfo()->AsynchComputeQueueId(i) << ((i >= (device->PhysicalDeviceInfo()->AsynchComputeQueueCount() - 1)) ? "]" : "; ");
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
					deviceFeatures.geometryShader = VK_TRUE;
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
				, m_memoryPool(nullptr) {
				// Retrieve queues:
				{
					for (uint32_t i = 0; i < m_device->PhysicalDevice()->QueueFamilyCount(); i++)
						m_deviceQueues.push_back(Object::Instantiate<VulkanDeviceQueue>(m_device, i));
					const std::optional<uint32_t> MAIN_GRAPHICS_QUEUE = m_device->PhysicalDevice()->GraphicsQueueId();
					if (MAIN_GRAPHICS_QUEUE.has_value())
						m_graphicsQueue = m_deviceQueues[MAIN_GRAPHICS_QUEUE.value()];
				}

				m_memoryPool = new VulkanMemoryPool(this);

#ifndef NDEBUG
				// Log creation status:
				LogDeviceInstantiateInfo(this);
#endif
			}

			VulkanDevice::~VulkanDevice() {
				WaitIdle();
				if (m_oneTimeCommandBuffers != nullptr) {
					std::unique_lock<std::recursive_mutex> lock(m_oneTimeCommandBufferLock);
					m_oneTimeCommandBuffers = nullptr;
				}
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

			DeviceQueue* VulkanDevice::GetQueue(size_t queueFamilyId)const { return m_deviceQueues[queueFamilyId]; }
			
			void VulkanDevice::WaitIdle()const {
				for (size_t i = 0; i < m_deviceQueues.size(); i++)
					dynamic_cast<VulkanDeviceQueue*>(m_deviceQueues[i].operator->())->WaitIdle();
			}

			VulkanMemoryPool* VulkanDevice::MemoryPool()const { return m_memoryPool; }

			std::mutex& VulkanDevice::PipelineCreationLock() { return m_pipelineCreationLock; }

			namespace {
				struct VulkanDevice_OneTimeCommandBuffers : public virtual Object {
					const Reference<VulkanCommandPool> commandPool;
					Reference<VulkanTimelineSemaphore> semaphore;
					std::atomic<uint64_t> lastSubmittedRevision = 0;

					inline VulkanDevice_OneTimeCommandBuffers(VkDeviceHandle* device, DeviceQueue* queue)
						: semaphore(Object::Instantiate<VulkanTimelineSemaphore>(device))
						, commandPool(queue->CreateCommandPool()) {}

					inline virtual ~VulkanDevice_OneTimeCommandBuffers() {}
				};
			}

			VulkanDevice::OneTimeCommandBufferInfo VulkanDevice::SubmitOneTimeCommandBuffer(Callback<VulkanPrimaryCommandBuffer*> recordCommands) {
				std::unique_lock<std::recursive_mutex> lock(m_oneTimeCommandBufferLock);
				Reference<VulkanDevice_OneTimeCommandBuffers> buffers = m_oneTimeCommandBuffers;

				// Create VulkanDevice_OneTimeCommandBuffers if it's not there...
				if (buffers == nullptr) {
					buffers = Object::Instantiate<VulkanDevice_OneTimeCommandBuffers>(m_device, GraphicsQueue());
					m_oneTimeCommandBuffers = buffers;
				}

				// When there's a risc of an overflow, discard the timeline semaphore, synchonize all submitted buffers and effectively reset the state
				if (buffers->lastSubmittedRevision == (~((uint64_t)0))) {
					buffers->semaphore = Object::Instantiate<VulkanTimelineSemaphore>(m_device, 0);
					buffers->lastSubmittedRevision = 0;
				}

				// Create and record command buffer:
				const uint64_t waitValue = buffers->lastSubmittedRevision.fetch_add(1);
				const uint64_t curValue = buffers->lastSubmittedRevision;
				Reference<VulkanPrimaryCommandBuffer> commandBuffer = buffers->commandPool->CreatePrimaryCommandBuffer();
				commandBuffer->BeginRecording();
				commandBuffer->WaitForSemaphore(buffers->semaphore, waitValue, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				commandBuffer->SignalSemaphore(buffers->semaphore, curValue);
				recordCommands(commandBuffer);
				commandBuffer->EndRecording();

				// Submit and return the info:
				buffers->commandPool->Queue()->ExecuteCommandBuffer(commandBuffer);
				return OneTimeCommandBufferInfo{ commandBuffer, buffers->semaphore, curValue };
			}

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
				if (cpuAccess == ArrayBuffer::CPUAccess::CPU_READ_WRITE)
					return Object::Instantiate<VulkanArrayBuffer>(this, objectSize, objectCount, false,
						VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
						VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				else return Object::Instantiate<VulkanCpuWriteOnlyBuffer>(this, objectSize, objectCount);
			}

			Reference<ImageTexture> VulkanDevice::CreateTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps) {
				return Object::Instantiate<VulkanCpuWriteOnlyTexture>(this, type, format, size, arraySize, generateMipmaps);
			}

			namespace {
				template<typename TextureType>
				inline static Reference<TextureType> CreateVulkanTexture(
					VulkanDevice* device,
					Texture::TextureType type, Texture::PixelFormat format,
					Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) {
					return Object::Instantiate<TextureType>(device, type, format, size, arraySize, false
						, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
						| ((sampleCount <= Texture::Multisampling::SAMPLE_COUNT_1) ? VK_IMAGE_USAGE_STORAGE_BIT : 0)
						| ((format >= Texture::PixelFormat::FIRST_DEPTH_FORMAT && format <= Texture::PixelFormat::LAST_DEPTH_FORMAT)
							? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
						, sampleCount);
				}
			}

			Reference<Texture> VulkanDevice::CreateMultisampledTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) {
				return CreateVulkanTexture<VulkanTexture>(this, type, format, size, arraySize, sampleCount);
			}

			Reference<ImageTexture> VulkanDevice::CreateCpuReadableTexture(Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize) {
				return CreateVulkanTexture<VulkanTextureCPU>(this, type, format, size, arraySize, Texture::Multisampling::SAMPLE_COUNT_1);
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

			Reference<RenderPass> VulkanDevice::CreateRenderPass(
				Texture::Multisampling sampleCount,
				size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats,
				Texture::PixelFormat depthFormat,
				RenderPass::Flags flags) {
				return Object::Instantiate<VulkanRenderPass>(this, sampleCount, numColorAttachments, colorAttachmentFormats, depthFormat, flags);
			}

			Reference<Pipeline> VulkanDevice::CreateEnvironmentPipeline(PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) {
				static const VkPipelineBindPoint BIND_POINTS[] = { VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE };
				return Object::Instantiate<VulkanEnvironmentPipeline>(this, descriptor, maxInFlightCommandBuffers, sizeof(BIND_POINTS) / sizeof(VkPipelineBindPoint), BIND_POINTS);
			}

			Reference<ComputePipeline> VulkanDevice::CreateComputePipeline(ComputePipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) {
				return Object::Instantiate<VulkanComputePipeline>(this, descriptor, maxInFlightCommandBuffers);
			}
		}
	}
}
#pragma warning(default: 26812)
