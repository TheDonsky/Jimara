#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanImage;
			class VulkanStaticImage;
		}
	}
}
#include "../../Pipeline/VulkanCommandBuffer.h"
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Basic VkImage wrapper interface </summary>
			class VulkanImage : public virtual Texture {
			public:
				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const = 0;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const = 0;

				/// <summary>
				/// Access static resource
				/// </summary>
				/// <param name="commandBuffer"> Command buffer that may depend on the resource </param>
				/// <returns> Reference to the texture </returns>
				virtual Reference<VulkanStaticImage> GetStaticHandle(VulkanCommandBuffer* commandBuffer) = 0;

				/// <summary> Automatic VkImageAspectFlags </summary>
				VkImageAspectFlags VulkanImageAspectFlags()const;

				/// <summary> Automatic VkAccessFlags and VkPipelineStageFlags based on old and new layouts (for memory barriers) </summary>
				static bool GetDefaultAccessMasksAndStages(VkImageLayout oldLayout, VkImageLayout newLayout
					, VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage);

				/// <summary> Fills in VkImageMemoryBarrier for image layout transition </summary>
				VkImageMemoryBarrier LayoutTransitionBarrier(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
					, VkImageAspectFlags aspectFlags
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask);

				/// <summary> Fills in VkImageMemoryBarrier for image layout transition (automatically calculates missing fields when possible) </summary>
				VkImageMemoryBarrier LayoutTransitionBarrier(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
					, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount);

				/// <summary> Records memory barrier for image layout transition </summary>
				void TransitionLayout(VulkanCommandBuffer* commandBuffer
					, VkImageLayout oldLayout, VkImageLayout newLayout
					, VkImageAspectFlags aspectFlags
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
					, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

				/// <summary> Records memory barrier for image layout transition (automatically calculates missing fields when possible) </summary>
				void TransitionLayout(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
					, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount);


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

				/// <summary>
				/// Records commands needed for mipmap generation
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record to </param>
				/// <param name="lastKnownLayout"> Last known image layout </param>
				/// <param name="targetLayout"> Target image layout </param>
				void GenerateMipmaps(VulkanCommandBuffer* commandBuffer, VkImageLayout lastKnownLayout, VkImageLayout targetLayout);

				/// <summary>
				/// "Blits"/Copies data from a region of some other buffer to a part of this one
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record operation on </param>
				/// <param name="srcTexture"> Source texture </param>
				/// <param name="dstRegion"> Region to copy to </param>
				/// <param name="srcRegion"> Source region to copy from </param>
				virtual void Blit(CommandBuffer* commandBuffer, Texture* srcTexture,
					const SizeAABB& dstRegion = SizeAABB(Size3(0u), Size3(~static_cast<uint32_t>(0u))),
					const SizeAABB& srcRegion = SizeAABB(Size3(0u), Size3(~static_cast<uint32_t>(0u)))) override;

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


			/// <summary> Interface for a VulkanImage, that has an immutable VkImage handle </summary>
			class VulkanStaticImage : public virtual VulkanImage {
			public:
				/// <summary> Type cast to underlying API object </summary>
				virtual operator VkImage()const = 0;

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


				/// <summary>
				/// Access self
				/// </summary>
				/// <param name="commandRecorder"> Command buffer that may depend on the resource </param>
				/// <returns> Reference to the texture </returns>
				virtual Reference<VulkanStaticImage> GetStaticHandle(VulkanCommandBuffer* commandBuffer) override;
			};
		}
	}
}
