#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDevice;
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
			/// Vulkan logical device(VkDevice) wrapper
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

				/// <summary> Graphics queue (VK_NULL_HANDLE if no graphics capabilities are present) </summary>
				VkQueue GraphicsQueue()const;

				/// <summary> Primary compute queue (same as GraphicsQueue if possible; VK_NULL_HANDLE if compute capabilities are missing) </summary>
				VkQueue ComputeQueue()const;

				/// <summary> Graphics-synchronous compute queue (same as GraphicsQueue if possible, VK_NULL_HANDLE otherwise) </summary>
				VkQueue SynchComputeQueue()const;

				/// <summary> Number of asynchronous compute queues </summary>
				size_t AsynchComputeQueueCount()const;

				/// <summary>
				/// Asynchronous compute queue by index
				/// </summary>
				/// <param name="index"> Queue index </param>
				/// <returns> ASynchronous compute queue </returns>
				VkQueue AsynchComputeQueue(size_t index)const;

				/// <summary> Memory pool </summary>
				VulkanMemoryPool* MemoryPool()const;

				/// <summary>
				/// Instantiates a render engine (Depending on the context/os etc only one per surface may be allowed)
				/// </summary>
				/// <param name="targetSurface"> Surface to render to </param>
				/// <returns> New instance of a render engine </returns>
				virtual Reference<RenderEngine> CreateRenderEngine(RenderSurface* targetSurface) override;

				/// <summary>
				/// Instantiates a shader cache object.
				/// Note: It's recomended to have a single shader cache per GraphicsDevice, but nobody's really judging you if you have a cache per shader...
				/// </summary>
				/// <returns> New shader cache instance </returns>
				virtual Reference<ShaderCache> CreateShaderCache() override;

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
				/// <param name="type"> Texture type </param>
				/// <param name="cpuAccess"> CPU access flags </param>
				/// <returns> New instance of an ImageTexture object </returns>
				virtual Reference<ImageTexture> CreateTexture(
					Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps, ImageTexture::CPUAccess cpuAccess) override;


			private:
				// Underlying API object
				VkDevice m_device;

				// Enabled device extensions
				std::vector<const char*> m_deviceExtensions;

				// Primary graphics queue (doubles as transfer/compute queue if possible)
				VkQueue m_graphicsQueue;

				// Primary compute queue (m_graphicsQueue if possible, otherwise m_asynchComputeQueues[0] if there is a valid compute queue)
				VkQueue m_primaryComputeQueue;

				// Synchronized compute queue (m_graphicsQueue or VK_NULL_HANDLE)
				VkQueue m_synchComputeQueue;

				// Asynchronous compute queues
				std::vector<VkQueue> m_asynchComputeQueues;

				// Memory pool
				VulkanMemoryPool* m_memoryPool;
			};
		}
	}
}
