#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanStaticTexture;
		}
	}
}
#include "VulkanTexture.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Wrapper on top of a VkImage object, responsible for it's full lifecycle </summary>
			class VulkanStaticTexture : public virtual VulkanStaticImage {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="type"> Texture dimensionality </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <param name="generateMipmaps"> If true, mipmaps will be generated </param>
				/// <param name="usage"> Usage flags </param>
				/// <param name="sampleCount"> Vulkan sample count </param>
				/// <param name="memoryFlags"> Buffer memory flags </param>
				VulkanStaticTexture(
					VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
					VkImageUsageFlags usage, Multisampling sampleCount, VkMemoryPropertyFlags memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanStaticTexture();

				/// <summary> Type of the image </summary>
				virtual TextureType Type()const override;

				/// <summary> Pixel format of the image </summary>
				virtual PixelFormat ImageFormat()const override;

				/// <summary> Sample count per texel </summary>
				virtual Multisampling SampleCount()const override;

				/// <summary> Image size (or array slice size) </summary>
				virtual Size3 Size()const override;

				/// <summary> Image array slice count </summary>
				virtual uint32_t ArraySize()const override;

				/// <summary> Mipmap level count </summary>
				virtual uint32_t MipLevels()const override;

				/// <summary> Type cast to API object </summary>
				virtual operator VkImage()const override;

				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const override;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const override;

			protected:
				/// <summary> Underlying memory allocation </summary>
				inline VulkanMemoryAllocation* Memory()const { return m_memory; }


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

				// Vulkan sample count
				const Multisampling m_sampleCount;

				// Underlying api object
				VkImage m_image;

				// Texture memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};

#pragma warning(disable: 4250)
			/// <summary> VulkanStaticTexture, that can be memory-mapped </summary>
			class VulkanStaticTextureCPU : public virtual VulkanStaticTexture, public virtual ImageTexture {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="type"> Texture dimensionality </param>
				/// <param name="format"> Texture format </param>
				/// <param name="size"> Texture size </param>
				/// <param name="arraySize"> Texture array slice count </param>
				/// <param name="generateMipmaps"> If true, mipmaps will be generated </param>
				/// <param name="usage"> Usage flags </param>
				/// <param name="sampleCount"> Vulkan sample count </param>
				inline VulkanStaticTextureCPU(
					VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
					VkImageUsageFlags usage, Multisampling sampleCount)
					: VulkanStaticTexture(device, type, format, size, arraySize, generateMipmaps, usage, sampleCount,
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
					, m_pitch([&]()->Size3 {
					VkImageSubresource subresource = {};
					{
						subresource.arrayLayer = 0;
						subresource.aspectMask = VulkanImageAspectFlags();
						subresource.mipLevel = 0;
					}
					VkSubresourceLayout layout = {};
					vkGetImageSubresourceLayout(*device, *this, &subresource, &layout);
					const uint32_t bytesPerPixel = static_cast<uint32_t>(BytesPerPixel(ImageFormat()));
					if ((layout.rowPitch % bytesPerPixel) != 0) 
						device->Log()->Error("VulkanStaticTextureCPU - rowPitch not a multiple of bytesPerPixel! [File: ", __FILE__, "; Line: ", __LINE__, "]"); 
					if ((layout.depthPitch % layout.rowPitch) != 0)
						device->Log()->Error("VulkanStaticTextureCPU - depthPitch not a multiple of rowPitch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					if ((layout.arrayPitch % layout.depthPitch) != 0)
						device->Log()->Error("VulkanStaticTextureCPU - arrayPitch not a multiple of depthPitch! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return Size3(
						layout.rowPitch / bytesPerPixel,
						layout.depthPitch / layout.rowPitch,
						layout.arrayPitch / layout.depthPitch);
						}()) {}

				/// <summary> CPU access info </summary>
				inline virtual CPUAccess HostAccess()const override { return CPUAccess::CPU_READ_WRITE; }

				/// <summary> 
				/// Size + padding (in texels) for data index to pixel index translation.
				/// <para/> Tex(x, y, z)[layer] = Tex->data[x + y * pitch.x + z * (pitch.x * pitch.y) + layer * (pitch.x * pitch.y * pitch.z)].
				/// </summary>
				inline virtual Size3 Pitch()const override { return m_pitch; }

				/// <summary>
				/// Maps texture memory to CPU
				/// Notes:
				///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
				///		1. Depending on the CPUAccess flag used during texture creation(or texture type when CPUAccess does not apply), 
				///		 the actual content of the texture will or will not be present in mapped memory.
				/// </summary>
				/// <returns> Mapped memory </returns>
				inline virtual void* Map() override { return Memory()->Map(true); }

				/// <summary>
				/// Unmaps memory previously mapped via Map() call
				/// </summary>
				/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
				inline virtual void Unmap(bool write) { Memory()->Unmap(write); }

			private:
				// Size with padding
				const Size3 m_pitch;
			};
#pragma warning(default: 4250)
		}
	}
}
