#include "VulkanDevice.h"
#include "Memory/Buffers/VulkanConstantBuffer.h"
#include "Memory/Buffers/VulkanIndirectBuffers.h"
#include "Memory/Textures/VulkanImageTexture.h"
#include "Memory/AccelerationStructures/VulkanBottomLevelAccelerationStructure.h"
#include "Memory/AccelerationStructures/VulkanTopLevelAccelerationStructure.h"
#include "Pipeline/Bindings/VulkanBindlessSet.h"
#include "Pipeline/RenderPass/VulkanRenderPass.h"
#include "Pipeline/Commands/VulkanDeviceQueue.h"
#include "Rendering/VulkanSurfaceRenderEngine.h"
#include "Pipeline/Bindings/VulkanBindingPool.h"
#include "Pipeline/Pipelines/VulkanComputePipeline.h"
#include "Pipeline/Pipelines/VulkanRayTracingPipeline.h"
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
						(device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::SWAP_CHAIN) ? "YES" : "NO");

					stream << std::endl << "    SAMPLER_ANISOTROPY: " <<
						(device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::SAMPLER_ANISOTROPY) ? "YES" : "NO");

					stream << std::endl << "    FRAG_INTERLOCK: " <<
						(device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK) ? "YES" : "NO");

					stream << std::endl << "    RAY_TRACING: " <<
						(device->PhysicalDeviceInfo()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING) ? "YES" : "NO");

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
#define ENABLE_VULKAN_DEVICE_FEATURE(featureName) \
	if (!static_cast<bool>(m_physicalDevice->DeviceFeatures().featureName)) \
		m_physicalDevice->Log()->Fatal("VulkanDevice - Missing feature '", #featureName, "'!"); \
	deviceFeatures.featureName = VK_TRUE
#define ENABLE_VULKAN_DEVICE_FEATURE_IF_PRESENT(featureName) \
	deviceFeatures.featureName = m_physicalDevice->DeviceFeatures().featureName

					ENABLE_VULKAN_DEVICE_FEATURE(samplerAnisotropy);
					ENABLE_VULKAN_DEVICE_FEATURE(sampleRateShading);
					ENABLE_VULKAN_DEVICE_FEATURE(fragmentStoresAndAtomics);
					ENABLE_VULKAN_DEVICE_FEATURE(vertexPipelineStoresAndAtomics);
					ENABLE_VULKAN_DEVICE_FEATURE_IF_PRESENT(geometryShader);
					ENABLE_VULKAN_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat);
					ENABLE_VULKAN_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat);
					ENABLE_VULKAN_DEVICE_FEATURE(multiDrawIndirect);
					if (m_physicalDevice->MaxMultisapling() > Texture::Multisampling::SAMPLE_COUNT_1) {
						ENABLE_VULKAN_DEVICE_FEATURE(shaderStorageImageMultisample);
					}
					ENABLE_VULKAN_DEVICE_FEATURE_IF_PRESENT(depthBounds);
					ENABLE_VULKAN_DEVICE_FEATURE_IF_PRESENT(shaderInt64);
#undef ENABLE_VULKAN_DEVICE_FEATURE_IF_PRESENT
#undef ENABLE_VULKAN_DEVICE_FEATURE
				}

				VkPhysicalDeviceVulkan12Features device12Features = {};
				{
					device12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
#define ENABLE_VULKAN_DEVICE_FEATURE12(featureName) \
	if (!static_cast<bool>(m_physicalDevice->DeviceFeatures12().featureName)) \
		m_physicalDevice->Log()->Fatal("VulkanDevice - Missing 1.2 feature '", #featureName, "'!"); \
	device12Features.featureName = VK_TRUE
					ENABLE_VULKAN_DEVICE_FEATURE12(timelineSemaphore);
					ENABLE_VULKAN_DEVICE_FEATURE12(descriptorIndexing);
					ENABLE_VULKAN_DEVICE_FEATURE12(bufferDeviceAddress);
					ENABLE_VULKAN_DEVICE_FEATURE12(runtimeDescriptorArray);
					ENABLE_VULKAN_DEVICE_FEATURE12(descriptorBindingPartiallyBound);
					ENABLE_VULKAN_DEVICE_FEATURE12(descriptorBindingVariableDescriptorCount);
					ENABLE_VULKAN_DEVICE_FEATURE12(descriptorBindingStorageBufferUpdateAfterBind);
					ENABLE_VULKAN_DEVICE_FEATURE12(descriptorBindingSampledImageUpdateAfterBind);
					ENABLE_VULKAN_DEVICE_FEATURE12(shaderStorageBufferArrayNonUniformIndexing);
					ENABLE_VULKAN_DEVICE_FEATURE12(shaderStorageImageArrayNonUniformIndexing);
					ENABLE_VULKAN_DEVICE_FEATURE12(shaderSampledImageArrayNonUniformIndexing);
#undef ENABLE_VULKAN_DEVICE_FEATURE12
				}

				void** pNext = &device12Features.pNext;
				auto setPNext = [&](auto* ptr) {
					(*pNext) = ptr;
					pNext = &ptr->pNext;
				};

				VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT shaderInterlockFeature = {};
				if (m_physicalDevice->DeviceExtensionVerison(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME).has_value()) {
					shaderInterlockFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;
					setPNext(&shaderInterlockFeature);
					const bool shaderInterlockSupported = m_physicalDevice->HasFeatures(PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK);
#define ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE(featureName) \
	if (shaderInterlockSupported && (!static_cast<bool>(m_physicalDevice->InterlockFeatures().featureName))) \
		m_physicalDevice->Log()->Fatal("VulkanDevice - Missing interlock feature '", #featureName, "'!"); \
	shaderInterlockFeature.featureName = shaderInterlockSupported ? VK_TRUE : VK_FALSE
#define ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE_IF_PRESENT(featureName) \
	shaderInterlockFeature.featureName = shaderInterlockSupported ? m_physicalDevice->InterlockFeatures().featureName : VK_FALSE
					ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE(fragmentShaderSampleInterlock);
					ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE(fragmentShaderPixelInterlock);
					ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE_IF_PRESENT(fragmentShaderShadingRateInterlock);
#undef ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE_IF_PRESENT
#undef ENABLE_VULKAN_SGADER_INTERLOCK_FEATURE
				}

				VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures;
				if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME).has_value()) {
					accelerationStructureFeatures = m_physicalDevice->RTFeatures().accelerationStructure;
					setPNext(&accelerationStructureFeatures);
				}
				VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT pipelineLibraryFeatures;
				if (m_physicalDevice->DeviceExtensionVerison(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME).has_value()) {
					pipelineLibraryFeatures = m_physicalDevice->RTFeatures().pipelineLibrary;
					setPNext(&pipelineLibraryFeatures);
				}
				VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures;
				if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_RAY_QUERY_EXTENSION_NAME).has_value()) {
					rayQueryFeatures = m_physicalDevice->RTFeatures().rayQuery;
					setPNext(&rayQueryFeatures);
				}
				VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures;
				if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME).has_value()) {
					rayTracingPipelineFeatures = m_physicalDevice->RTFeatures().rayTracingPipeline;
					setPNext(&rayTracingPipelineFeatures);
				}
				VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenance1Features;
				if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME).has_value()) {
					rayTracingMaintenance1Features = m_physicalDevice->RTFeatures().maintenance1;
					setPNext(&rayTracingMaintenance1Features);
				}
				VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rayTracingPositionFetchFeatures;
				if (m_physicalDevice->DeviceExtensionVerison(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME).has_value()) {
					rayTracingPositionFetchFeatures = m_physicalDevice->RTFeatures().positionFetch;
					setPNext(&rayTracingPositionFetchFeatures);
				}

				// Creating the logical device:
				VkDeviceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pNext = &device12Features;
				createInfo.pQueueCreateInfos = (queueCreateInfos.size() > 0 ? queueCreateInfos.data() : nullptr);
				createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				{
					auto enableExtensionIfPresent = [&](const char* name) {
						if (!m_physicalDevice->DeviceExtensionVerison(name).has_value())
							return false;
						m_deviceExtensions.push_back(name);
						return true;
					};
					auto enableExtension = [&](const char* name) {
						if (!enableExtensionIfPresent(name))
							m_physicalDevice->Log()->Fatal("VulkanDevice - Missing extension '", name, "'!");
					};
					enableExtensionIfPresent(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#ifdef __APPLE__
					enableExtensionIfPresent("VK_KHR_portability_subset");
#endif
					enableExtensionIfPresent(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
					enableExtension(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
					enableExtension(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
					enableExtension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);

					enableExtensionIfPresent(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
					enableExtensionIfPresent(VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_RAY_QUERY_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
					enableExtensionIfPresent(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
					
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

				if (m_physicalDevice->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					m_rtAPI.FindAPIMethods(m_device);
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

			Reference<Buffer> VulkanDevice::CreateConstantBuffer(size_t size) {
				return Object::Instantiate<VulkanConstantBuffer>(size);
			}

			Reference<ArrayBuffer> VulkanDevice::CreateArrayBuffer(size_t objectSize, size_t objectCount, ArrayBuffer::CPUAccess cpuAccess) {
				if (cpuAccess == ArrayBuffer::CPUAccess::CPU_READ_WRITE)
					return Object::Instantiate<VulkanArrayBuffer>(this, objectSize, objectCount, false,
						VulkanCpuWriteOnlyBuffer::DefaultUsage(PhysicalDeviceInfo()),
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
				else return Object::Instantiate<VulkanCpuWriteOnlyBuffer>(this, objectSize, objectCount);
			}

			IndirectDrawBufferReference VulkanDevice::CreateIndirectDrawBuffer(size_t objectCount, ArrayBuffer::CPUAccess cpuAccess) {
				if (cpuAccess == ArrayBuffer::CPUAccess::CPU_READ_WRITE)
					return Object::Instantiate<VulkanCPUReadWriteIndirectDrawBuffer>(this, objectCount);
				else return Object::Instantiate<VulkanCPUWriteOnlyIndirectDrawBuffer>(this, objectCount);
			}

			Reference<ImageTexture> VulkanDevice::CreateTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps, ImageTexture::AccessFlags accessFlags) {
				return Object::Instantiate<VulkanImageTexture>(this,
					type, format, size, arraySize, generateMipmaps,
					VulkanTexture::DefaultUsage(format), accessFlags);
			}

			Reference<Texture> VulkanDevice::CreateMultisampledTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) {
				return Object::Instantiate<VulkanTexture>(
					this, type, format, size, arraySize, false, VulkanTexture::DefaultUsage(format), sampleCount, 
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_LAYOUT_GENERAL);
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

			Reference<BottomLevelAccelerationStructure> VulkanDevice::CreateBottomLevelAccelerationStructure(const BottomLevelAccelerationStructure::Properties& properties) {
				return VulkanBottomLevelAccelerationStructure::Create(this, properties);
			}

			Reference<TopLevelAccelerationStructure> VulkanDevice::CreateTopLevelAccelerationStructure(const TopLevelAccelerationStructure::Properties& properties) {
				return VulkanTopLevelAccelerationStructure::Create(this, properties);
			}

			Reference<BindlessSet<ArrayBuffer>> VulkanDevice::CreateArrayBufferBindlessSet() {
				return Object::Instantiate<VulkanBindlessSet<ArrayBuffer>>(this);
			}

			Reference<BindlessSet<TextureSampler>> VulkanDevice::CreateTextureSamplerBindlessSet() {
				return Object::Instantiate<VulkanBindlessSet<TextureSampler>>(this);
			}

			Reference<RenderPass> VulkanDevice::GetRenderPass(
				Texture::Multisampling sampleCount,
				size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats,
				Texture::PixelFormat depthFormat,
				RenderPass::Flags flags) {
				return VulkanRenderPass::Get(this, sampleCount, numColorAttachments, colorAttachmentFormats, depthFormat, flags);
			}

			Reference<Graphics::ComputePipeline> VulkanDevice::GetComputePipeline(const SPIRV_Binary* computeShader) {
				return VulkanComputePipeline::Get(this, computeShader);
			}

			Reference<RayTracingPipeline> VulkanDevice::CreateRayTracingPipeline(const RayTracingPipeline::Descriptor& pipelineDescriptor) {
				return VulkanRayTracingPipeline::Create(this, pipelineDescriptor);
			}

			Reference<BindingPool> VulkanDevice::CreateBindingPool(size_t inFlightCommandBufferCount) {
				return Object::Instantiate<VulkanBindingPool>(this, inFlightCommandBufferCount);
			}
		}
	}
}
#pragma warning(default: 26812)
