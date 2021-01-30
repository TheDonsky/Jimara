#pragma once
#include "../../Memory/Texture.h"
#include "VulkanBuffer.h"
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Basic VkImage wrapper interface </summary>
			class VulkanImage : public virtual Texture {
			public:
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

				/// <summary> Type cast to API object </summary>
				virtual operator VkImage()const = 0;

				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const = 0;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const = 0;

				/// <summary> Sample count per texel </summary>
				virtual VkSampleCountFlagBits SampleCount()const = 0;

				/// <summary> Automatic VkImageAspectFlags based on target layout </summary>
				VkImageAspectFlags LayoutTransitionAspectFlags(VkImageLayout targetLayout)const;

				/// <summary> Automatic VkAccessFlags and VkPipelineStageFlags based on old and new layouts (for memory barriers) </summary>
				static bool GetDefaultAccessMasksAndStages(VkImageLayout oldLayout, VkImageLayout newLayout
					, VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage);

				/// <summary> Fills in VkImageMemoryBarrier for image layout transition </summary>
				VkImageMemoryBarrier LayoutTransitionBarrier(VkImageLayout oldLayout, VkImageLayout newLayout
					, VkImageAspectFlags aspectFlags
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)const;

				/// <summary> Fills in VkImageMemoryBarrier for image layout transition (automatically calculates missing fields when possible) </summary>
				VkImageMemoryBarrier LayoutTransitionBarrier(VkImageLayout oldLayout, VkImageLayout newLayout
					, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount)const;

				/// <summary> Records memory barrier for image layout transition </summary>
				void TransitionLayout(VkCommandBuffer commandBuffer
					, VkImageLayout oldLayout, VkImageLayout newLayout
					, VkImageAspectFlags aspectFlags
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
					, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)const;

				/// <summary> Records memory barrier for image layout transition (automatically calculates missing fields when possible) </summary>
				void TransitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
					, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount)const;

				/// <summary>
				/// Records commands needed for mipmap generation
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record to </param>
				/// <param name="lastKnownLayout"> Last known image layout </param>
				/// <param name="targetLayout"> Target image layout </param>
				void GenerateMipmaps(VkCommandBuffer commandBuffer, VkImageLayout lastKnownLayout, VkImageLayout targetLayout)const;

				/// <summary>
				/// Translates VkFormat to PixelFormat
				/// </summary>
				/// <param name="format"> Vulkan API format </param>
				/// <returns> Engine format </returns>
				static PixelFormat PixelFormatFromNativeFormat(VkFormat format);

				/// <summary>
				/// Translates PixelFormat to VkFormat
				/// </summary>
				/// <param name="format"> Engine format </param>
				/// <returns> Vulkan API format </returns>
				static VkFormat NativeFormatFromPixelFormat(PixelFormat format);

				/// <summary>
				/// Calculates a size of a single pixel, given the format
				/// </summary>
				/// <param name="format"> Pixel format </param>
				/// <returns> Pixel size </returns>
				static size_t BytesPerPixel(PixelFormat format);

				/// <summary>
				/// Translates TextureType to VkImageType
				/// </summary>
				/// <param name="type"> Engine type </param>
				/// <returns> Vulkan API type </returns>
				static VkImageType NativeTypeFromTextureType(TextureType type);
			};


			/// <summary> Wrapper on top of a VkImage object, responsible for it's full lifecycle </summary>
			class VulkanTexture : public virtual VulkanImage {
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
				VulkanTexture(
					VulkanDevice* device, TextureType type, PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps,
					VkImageUsageFlags usage, VkSampleCountFlagBits sampleCount);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanTexture();

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

				/// <summary> Type cast to API object </summary>
				virtual operator VkImage()const override;

				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const override;

				/// <summary> Sample count per texel </summary>
				virtual VkSampleCountFlagBits SampleCount()const override;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const override;

				/// <summary>
				/// Calculates mip level count
				/// </summary>
				/// <param name="size"> Image size </param>
				/// <returns> Mip level count </returns>
				static uint32_t CalculateMipLevels(const Size3& size);

				/// <summary>
				/// Calculates supported mip level count based on device and format
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="format"> Pixel format </param>
				/// <param name="size"> Image size </param>
				/// <returns> Mip level count </returns>
				static uint32_t CalculateSupportedMipLevels(VulkanDevice* device, Texture::PixelFormat format, const Size3& size);


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
				const VkSampleCountFlagBits m_sampleCount;

				// Underlying api object
				VkImage m_image;

				// Texture memory allocation
				Reference<VulkanMemoryAllocation> m_memory;
			};
		}
	}
}
