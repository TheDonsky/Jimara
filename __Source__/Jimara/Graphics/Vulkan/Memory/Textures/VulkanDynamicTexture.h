#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanDynamicTexture;
		}
	}
}
#include "VulkanStaticTexture.h"
#include "../Buffers/VulkanStaticBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// CPU-writable vulkan texture
			/// </summary>
			class VulkanDynamicTexture : public virtual ImageTexture, public virtual VulkanImage {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="type"> Texture type </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Array slice count </param>
				/// <param name="generateMipmaps"> If true, underlaying images will be auto-generated </param>
				VulkanDynamicTexture(VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDynamicTexture();

				/// <summary> Type of the image </summary>
				virtual TextureType Type()const override;

				/// <summary> Pixel format of the image </summary>
				virtual PixelFormat ImageFormat()const override;

				/// <summary> Image size (or array slice size) </summary>
				virtual Size3 Size()const override;

				/// <summary> Image array slice count </summary>
				virtual uint32_t ArraySize()const override;

				/// <summary> Mipmap level count </summary>
				virtual uint32_t MipLevels()const override;

				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const override;

				/// <summary> Sample count per texel </summary>
				virtual VkSampleCountFlagBits SampleCount()const override;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const override;

				/// <summary>
				/// Creates an image view
				/// </summary>
				/// <param name="type"> View type </param>
				/// <param name="baseMipLevel"> Base mip level (default 0) </param>
				/// <param name="mipLevelCount"> Number of mip levels (default is all) </param>
				/// <param name="baseArrayLayer"> Base array slice (default 0) </param>
				/// <param name="arrayLayerCount"> Number of array slices (default is all) </param>
				/// <returns> A new instance of an image view </returns>
				virtual Reference<TextureView> CreateView(TextureView::ViewType type
					, uint32_t baseMipLevel = 0, uint32_t mipLevelCount = ~((uint32_t)0u)
					, uint32_t baseArrayLayer = 0, uint32_t arrayLayerCount = ~((uint32_t)0u)) override;


				/// <summary> CPU access info </summary>
				virtual CPUAccess HostAccess()const override;

				/// <summary>
				/// Maps texture memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during texture creation(or texture type when CPUAccess does not apply), 
				///		 the actual content of the texture will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				virtual void* Map() override;

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				virtual void Unmap(bool write) override;

				/// <summary>
				/// Access immutable texture
				/// </summary>
				/// <param name="commandRecorder"> Command recorder for flushing any modifications if necessary </param>
				/// <returns> Reference to the texture </returns>
				virtual Reference<VulkanStaticImage> GetStaticHandle(VulkanCommandRecorder* commandRecorder) override;


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Texture type
				const TextureType m_textureType;

				// Testure format
				const PixelFormat m_pixelFormat;

				// Texture size
				const Size3 m_textureSize;

				// Texture slice count
				const uint32_t m_arraySize;

				// Mipmap count
				const uint32_t m_mipLevels;

				// Lock for m_texture and m_stagingBuffer
				std::mutex m_bufferLock;

				// Texture, holding the data
				Reference<VulkanStaticTexture> m_texture;

				// Staging buffer for temporarily holding CPU-mapped data
				Reference<VulkanStaticBuffer> m_stagingBuffer;

				// CPU mapping
				void* m_cpuMappedData;
			};
		}
	}
}
