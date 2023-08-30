#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanTexture;
		}
	}
}
#include "VulkanImage.h"
#include "../VulkanOneTimeCommandBufferCache.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Wrapper on top of a VkImage object, responsible for it's full lifecycle </summary>
			class JIMARA_API VulkanTexture : public virtual VulkanImage {
			public:
				/// <summary>
				/// Default VkImageUsageFlags for given PixelFormat
				/// </summary>
				/// <param name="format"> Pixel format </param>
				/// <returns> Format </returns>
				inline static constexpr VkImageUsageFlags DefaultUsage(PixelFormat format) {
					return VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT
						| ((format >= Texture::PixelFormat::FIRST_DEPTH_FORMAT && format <= Texture::PixelFormat::LAST_DEPTH_FORMAT)
							? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
				}

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
				/// <param name="shaderAccessLayout"> Shader access layout </param>
				VulkanTexture(
					VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
					VkImageUsageFlags usage, Multisampling sampleCount, 
					VkMemoryPropertyFlags memoryFlags, VkImageLayout shaderAccessLayout);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanTexture();

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

				/// <summary> Command buffer cache for internal updates </summary>
				inline VulkanOneTimeCommandBufferCache* OneTimeCommandBufferCache() { return &m_updateCache; }

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

				// Command buffer cache for internal updates
				VulkanOneTimeCommandBufferCache m_updateCache;
			};
		}
	}
}
