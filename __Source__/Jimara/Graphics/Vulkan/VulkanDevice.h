#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDevice;
			class VkDeviceHandle;
		}
	}
}
#include "../GraphicsDevice.h"
#include "VulkanAPIIncludes.h"
#include "VulkanPhysicalDevice.h"
#include "Memory/VulkanMemory.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Just a wrapper on top of VkDevice
			/// </summary>
			class VkDeviceHandle : public virtual Object {
			private:
				/// <summary> Physical device handle </summary>
				const Reference<VulkanPhysicalDevice> m_physicalDevice;

				// Vulkan Device
				VkDevice m_device;

				// Enabled device extensions
				std::vector<const char*> m_deviceExtensions;

			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="physicalDevice"> Physical device </param>
				VkDeviceHandle(VulkanPhysicalDevice* physicalDevice);

				/// <summary> Virtual destructor </summary>
				virtual ~VkDeviceHandle();

				/// <summary> Type cast to API object </summary>
				inline operator VkDevice()const { return m_device; }

				/// <summary> Physical device </summary>
				inline VulkanPhysicalDevice* PhysicalDevice()const { return m_physicalDevice; }

				/// <summary> Logger </summary>
				inline OS::Logger* Log()const { return m_physicalDevice->Log(); }
			};

			/// <summary>
			/// Vulkan backed logical device
			/// </summary>
			class VulkanDevice : public GraphicsDevice {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="physicalDevice"> Physical device </param>
				VulkanDevice(VulkanPhysicalDevice* physicalDevice);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDevice();

				/// <summary> Vulkan instance </summary>
				VulkanInstance* VulkanAPIInstance()const;

				/// <summary> Physical device </summary>
				VulkanPhysicalDevice* PhysicalDeviceInfo()const;

				/// <summary> Type cast to API object </summary>
				operator VkDevice()const;

				/// <summary> Type cast to VkDeviceHandle </summary>
				operator VkDeviceHandle* ()const;

				/// <summary> Access to main graphics queue </summary>
				virtual DeviceQueue* GraphicsQueue()const override;

				/// <summary>
				/// Gets device queue based on it's family id
				/// </summary>
				/// <param name="queueFamilyId"> Queue family id </param>
				/// <returns> Queue by family id </returns>
				DeviceQueue* GetQueue(size_t queueFamilyId)const;

				/// <summary> Safe-ish way to invoke vkDeviceWaitIdle() </summary>
				void WaitIdle()const;

				/// <summary> Memory pool </summary>
				VulkanMemoryPool* MemoryPool()const;

				/// <summary> Pipeline creation lock </summary>
				std::mutex& PipelineCreationLock();

				/// <summary>
				/// Instantiates a render engine (Depending on the context/os etc only one per surface may be allowed)
				/// </summary>
				/// <param name="targetSurface"> Surface to render to </param>
				/// <returns> New instance of a render engine </returns>
				virtual Reference<RenderEngine> CreateRenderEngine(RenderSurface* targetSurface) override;

				/// <summary>
				/// Instantiates a shader module
				/// Note: Generally speaking, it's recommended to use shader cache for effective shader reuse.
				/// </summary>
				/// <param name="bytecode"> SPIR-V bytecode </param>
				/// <returns> New instance of a shader module </returns>
				virtual Reference<Shader> CreateShader(const SPIRV_Binary* bytecode) override;

				/// <summary>
				/// Creates an instance of a buffer that can be used as a constant buffer
				/// </summary>
				/// <param name="size"> Buffer size </param>
				/// <returns> New constant buffer </returns>
				virtual Reference<Buffer> CreateConstantBuffer(size_t size) override;

				/// <summary>
				/// Creates an array-type buffer of given size
				/// </summary>
				/// <param name="objectSize"> Individual element size </param>
				/// <param name="objectCount"> Element count within the buffer </param>
				/// <param name="cpuAccess"> CPU access flags </param>
				/// <returns> New instance of a buffer </returns>
				virtual Reference<ArrayBuffer> CreateArrayBuffer(size_t objectSize, size_t objectCount, ArrayBuffer::CPUAccess cpuAccess) override;

				/// <summary>
				/// Creates an image texture
				/// </summary>
				/// <param name="type"> Texture type </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <param name="generateMipmaps"> If true, image will generate mipmaps </param>
				/// <returns> New instance of an ImageTexture object </returns>
				virtual Reference<ImageTexture> CreateTexture(
					Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps) override;

				/// <summary>
				/// Creates a multisampled texture for color/depth attachments
				/// </summary>
				/// <param name="type"> Texture type </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <param name="sampleCount"> Desired multisampling (if the device does not support this amount, some lower number may be chosen) </param>
				/// <returns> New instance of a multisampled texture </returns>
				virtual Reference<Texture> CreateMultisampledTexture(
					Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) override;

				/// <summary>
				/// Creates a texture, that can be read from CPU
				/// </summary>
				/// <param name="type"> Texture type </param>
				/// <param name="format"> Pixel format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <returns> New instance of a texture with ArrayBuffer::CPUAccess::CPU_READ_WRITE flags </returns>
				virtual Reference<Texture> CreateCpuReadableTexture(
					Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize) override;

				/// <summary> Selects a depth format supported by the device (there may be more than one in actuality, but this picks one of them by prefference) </summary>
				virtual Texture::PixelFormat GetDepthFormat() override;

				/// <summary>
				/// Creates a render pass
				/// </summary>
				/// <param name="sampleCount"> "MSAA" </param>
				/// <param name="numColorAttachments"> Color attachment count </param>
				/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
				/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
				/// <param name="flags"> Clear and resolve flags </param>
				/// <returns> New instance of a render pass </returns>
				virtual Reference<RenderPass> CreateRenderPass(
					Texture::Multisampling sampleCount,
					size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats,
					Texture::PixelFormat depthFormat,
					RenderPass::Flags flags) override;

				/// <summary>
				/// Creates an environment pipeline
				/// </summary>
				/// <param name="descriptor"> Environment pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
				/// <returns> New instance of an environment pipeline object </returns>
				virtual Reference<Pipeline> CreateEnvironmentPipeline(PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) override;

				/// <summary>
				/// Creates a compute pipeline
				/// </summary>
				/// <param name="descriptor"> Compute pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
				/// <returns> New instance of a compute pipeline object </returns>
				virtual Reference<ComputePipeline> CreateComputePipeline(ComputePipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) override;


			private:
				// Underlying API object
				Reference<VkDeviceHandle> m_device;

				// Primary graphics queue (doubles as transfer/compute queue if possible)
				Reference<DeviceQueue> m_graphicsQueue;

				// All device queues
				std::vector<Reference<DeviceQueue>> m_deviceQueues;

				// Memory pool
				VulkanMemoryPool* m_memoryPool;

				// Pipeline creation lock
				std::mutex m_pipelineCreationLock;
			};
		}
	}
}
