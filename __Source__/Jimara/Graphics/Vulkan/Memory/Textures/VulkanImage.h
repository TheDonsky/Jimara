#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanImage;
		}
	}
}
#include "../../Pipeline/Commands/VulkanCommandBuffer.h"
#include <mutex>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Basic VkImage wrapper interface </summary>
			class JIMARA_API VulkanImage : public virtual Texture {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="shaderAccessLayout"> Shader access layout </param>
				inline VulkanImage(VkImageLayout shaderAccessLayout) 
					: m_shaderAccessLayout(shaderAccessLayout) {}

				/// <summary> Type cast to underlying API object </summary>
				virtual operator VkImage()const = 0;

				/// <summary> Vulkan color format </summary>
				virtual VkFormat VulkanFormat()const = 0;

				/// <summary> "Owner" device </summary>
				virtual VulkanDevice* Device()const = 0;

				/// <summary> Automatic VkImageAspectFlags </summary>
				VkImageAspectFlags VulkanImageAspectFlags()const;

				/// <summary> Layout for in-shader usage </summary>
				inline VkImageLayout ShaderAccessLayout()const { return m_shaderAccessLayout; }

				/// <summary> Automatic VkAccessFlags and VkPipelineStageFlags based on old and new layouts (for memory barriers) </summary>
				static bool GetDefaultAccessMasksAndStages(VkImageLayout oldLayout, VkImageLayout newLayout
					, VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage
					, GraphicsDevice* device);

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
				void TransitionLayout(VulkanCommandBuffer* commandBuffer
					, VkImageLayout oldLayout, VkImageLayout newLayout
					, VkImageAspectFlags aspectFlags
					, uint32_t baseMipLevel, uint32_t mipLevelCount
					, uint32_t baseArrayLayer, uint32_t arrayLayerCount
					, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
					, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)const;

				/// <summary> Records memory barrier for image layout transition (automatically calculates missing fields when possible) </summary>
				void TransitionLayout(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
					, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount)const;


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
				/// Copies a region of another texture onto this one without rescaling, as long as formats are compatible
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record operation on </param>
				/// <param name="srcTexture"> Source texture </param>
				/// <param name="dstOffset"> Start of the region to copy to </param>
				/// <param name="srcOffset"> Start of the region to copy from </param>
				/// <param name="regionSize"> Copied region size </param>
				virtual void Copy(CommandBuffer* commandBuffer, Texture* srcTexture,
					const Size3& dstOffset = Size3(0u),
					const Size3& srcOffset = Size3(0u),
					const Size3& regionSize = Size3(~static_cast<uint32_t>(0u))) override;

				/// <summary>
				/// Copies a region of a buffer to a texture
				/// <para/> buffer element size does not matter, 
				/// but it's content should be exactly the same as you would expect the memory-mapped texture region to look like
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record operation on </param>
				/// <param name="srcBuffer"> Source buffer </param>
				/// <param name="bufferImageLayerSize"> Virtual texture layer size on the buffer (in texels) </param>
				/// <param name="dstOffset"> Start of the region to copy to </param>
				/// <param name="srcOffset"> Start of the region to copy from (in texels) </param>
				/// <param name="regionSize"> Copied region size (in texels) </param>
				virtual void Copy(CommandBuffer* commandBuffer, ArrayBuffer* srcBuffer,
					const Size3& bufferImageLayerSize,
					const Size3& dstOffset = Size3(0u),
					const Size3& srcOffset = Size3(0u),
					const Size3& regionSize = Size3(~static_cast<uint32_t>(0u))) override;

				/// <summary>
				/// Clears the image with a single color
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record operation on </param>
				/// <param name="color"> Clear color </param>
				/// <param name="baseMipLevel"> Base mip level (default 0) </param>
				/// <param name="mipLevelCount"> Number of mip levels (default is all) </param>
				/// <param name="baseArrayLayer"> Base array slice (default 0) </param>
				/// <param name="arrayLayerCount"> Number of array slices (default is all) </param>
				virtual void Clear(CommandBuffer* commandBuffer, const Vector4& color,
					uint32_t baseMipLevel = 0, uint32_t mipLevelCount = ~((uint32_t)0u),
					uint32_t baseArrayLayer = 0, uint32_t arrayLayerCount = ~((uint32_t)0u)) override;

				/// <summary>
				/// Generates all mip levels from the highest mip
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record operation on </param>
				virtual void GenerateMipmaps(CommandBuffer* commandBuffer) override;

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

			private:
				// Layout for in-shader usage
				const VkImageLayout m_shaderAccessLayout;
			};
		}
	}
}
