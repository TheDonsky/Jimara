#include "VulkanTexture.h"
#include "../TextureViews/VulkanStaticTextureView.h"
#include <unordered_map>

#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VkImageAspectFlags VulkanImage::VulkanImageAspectFlags()const {
				PixelFormat format = ImageFormat();
				return (format >= PixelFormat::FIRST_DEPTH_FORMAT && format <= PixelFormat::LAST_DEPTH_FORMAT)
					? (VK_IMAGE_ASPECT_DEPTH_BIT | 
						((format >= PixelFormat::FIRST_DEPTH_AND_STENCIL_FORMAT && format <= PixelFormat::LAST_DEPTH_AND_STENCIL_FORMAT) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0))
					: VK_IMAGE_ASPECT_COLOR_BIT;
			}

			bool VulkanImage::GetDefaultAccessMasksAndStages(VkImageLayout oldLayout, VkImageLayout newLayout
				, VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage) {
				// Code below needs further examination...I do not understand what's going on...
				if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = 0;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_TRANSFER_WRITE_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
					if (srcAccessMask != nullptr) {
						if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
							(*srcAccessMask) = VK_ACCESS_TRANSFER_WRITE_BIT;
						else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
							(*srcAccessMask) = VK_ACCESS_TRANSFER_READ_BIT;
						else (*srcAccessMask) = 0;
					}
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_SHADER_READ_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TRANSFER_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = 0;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = 0;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else return false;
				return true;
			}

			VkImageMemoryBarrier VulkanImage::LayoutTransitionBarrier(VulkanCommandBuffer* commandBuffer
				, VkImageLayout oldLayout, VkImageLayout newLayout
				, VkImageAspectFlags aspectFlags
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount
				, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
				VkImageMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.oldLayout = oldLayout;
					barrier.newLayout = newLayout;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.image = *GetStaticHandle(commandBuffer);
					barrier.subresourceRange.aspectMask = aspectFlags;
					barrier.subresourceRange.baseMipLevel = baseMipLevel;
					barrier.subresourceRange.levelCount = mipLevelCount;
					barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
					barrier.subresourceRange.layerCount = arrayLayerCount;
					barrier.srcAccessMask = srcAccessMask;
					barrier.dstAccessMask = dstAccessMask;
				}
				return barrier;
			}

			VkImageMemoryBarrier VulkanImage::LayoutTransitionBarrier(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
				, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount) {

				VkAccessFlags srcAccessMask, dstAccessMask;
				if (!GetDefaultAccessMasksAndStages(oldLayout, newLayout, &srcAccessMask, &dstAccessMask, nullptr, nullptr))
					Device()->Log()->Error("VulkanImage::LayoutTransitionBarrier - Can not automatically deduce srcAccessMask and dstAccessMask");

				return LayoutTransitionBarrier(commandBuffer
					, oldLayout, newLayout
					, VulkanImageAspectFlags()
					, baseMipLevel, mipLevelCount
					, baseArrayLayer, arrayLayerCount
					, srcAccessMask, dstAccessMask);
			}

			void VulkanImage::TransitionLayout(VulkanCommandBuffer* commandBuffer
				, VkImageLayout oldLayout, VkImageLayout newLayout
				, VkImageAspectFlags aspectFlags
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount
				, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask
				, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
				
				if (oldLayout == newLayout) return;

				VkImageMemoryBarrier barrier = LayoutTransitionBarrier(commandBuffer
					, oldLayout, newLayout
					, aspectFlags
					, baseMipLevel, mipLevelCount
					, baseArrayLayer, arrayLayerCount
					, srcAccessMask, dstAccessMask);

				vkCmdPipelineBarrier(
					*commandBuffer,
					srcStage, dstStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			}

			void VulkanImage::TransitionLayout(VulkanCommandBuffer* commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout
				, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount) {
				
				VkAccessFlags srcAccessMask, dstAccessMask;
				VkPipelineStageFlags srcStage, dstStage;
				if (!GetDefaultAccessMasksAndStages(oldLayout, newLayout, &srcAccessMask, &dstAccessMask, &srcStage, &dstStage))
					Device()->Log()->Error("VulkanImage::TransitionLayout - Can not automatically deduce srcAccessMask, dstAccessMask, srcStage and dstStage");
				
				TransitionLayout(commandBuffer
					, oldLayout, newLayout
					, VulkanImageAspectFlags()
					, baseMipLevel, mipLevelCount
					, baseArrayLayer, arrayLayerCount
					, srcAccessMask, dstAccessMask
					, srcStage, dstStage);
			}

			uint32_t VulkanImage::CalculateMipLevels(const Size3& size) {
				return static_cast<uint32_t>(std::floor(std::log2(max(max(size.x, size.y), size.z)))) + 1u;
			}

			uint32_t VulkanImage::CalculateSupportedMipLevels(VulkanDevice* device, Texture::PixelFormat format, const Size3& size) {
				VkFormatProperties formatProperties;

				VkFormat nativeFormat = VulkanImage::NativeFormatFromPixelFormat(format);
				if (nativeFormat == VK_FORMAT_UNDEFINED) return 1u;

				vkGetPhysicalDeviceFormatProperties(*device->PhysicalDeviceInfo(), nativeFormat, &formatProperties);
				return ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0) ? CalculateMipLevels(size) : 1u;
			}

			void VulkanImage::GenerateMipmaps(VulkanCommandBuffer* commandBuffer, VkImageLayout lastKnownLayout, VkImageLayout targetLayout) {
				uint32_t mipLevels = MipLevels();
				uint32_t arraySize = ArraySize();
				if (mipLevels <= 1) {
					TransitionLayout(commandBuffer, lastKnownLayout, targetLayout, 0, mipLevels, 0, arraySize);
					return;
				}
				TransitionLayout(commandBuffer, lastKnownLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, mipLevels, 0, arraySize);
				VkImageMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.image = *GetStaticHandle(commandBuffer);
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier.subresourceRange.baseArrayLayer = 0;
					barrier.subresourceRange.layerCount = arraySize;
					barrier.subresourceRange.levelCount = 1;
				}
				Size3 mipSize = Size();
				uint32_t lastMip = mipLevels - 1;
				for (uint32_t i = 0; true; i++) {
					{
						barrier.subresourceRange.baseMipLevel = i;
						barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

						vkCmdPipelineBarrier(*commandBuffer,
							VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
							0, nullptr,
							0, nullptr,
							1, &barrier);
					}
					if (i >= lastMip) break;
					Size3 nextMipSize(
						mipSize.x > 1 ? (mipSize.x >> 1) : 1,
						mipSize.y > 1 ? (mipSize.y >> 1) : 1,
						mipSize.z > 1 ? (mipSize.z >> 1) : 1);
					{
						VkImageBlit blit{};
						blit.srcOffsets[0] = { 0, 0, 0 };
						blit.srcOffsets[1] = { static_cast<int32_t>(mipSize.x), static_cast<int32_t>(mipSize.y), static_cast<int32_t>(mipSize.z) };
						blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.srcSubresource.mipLevel = i;
						blit.srcSubresource.baseArrayLayer = 0;
						blit.srcSubresource.layerCount = arraySize;
						blit.dstOffsets[0] = { 0, 0, 0 };
						blit.dstOffsets[1] = { static_cast<int32_t>(nextMipSize.x), static_cast<int32_t>(nextMipSize.y), static_cast<int32_t>(nextMipSize.z) };
						blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.dstSubresource.mipLevel = i + 1;
						blit.dstSubresource.baseArrayLayer = 0;
						blit.dstSubresource.layerCount = arraySize;
						vkCmdBlitImage(*commandBuffer,
							barrier.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							barrier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1, &blit,
							VK_FILTER_LINEAR);
					}
					mipSize = nextMipSize;
				}
				TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetLayout, 0, mipLevels, 0, arraySize);
			}

			namespace {
				struct VulkanFormatInfo {
					VkFormat format;
					size_t bytesPerPixel;
				};

				static const VulkanFormatInfo* PIXEL_TO_NATIVE_FORMATS = []() -> VulkanFormatInfo* {
					static const uint8_t formatCount = static_cast<uint8_t>(Texture::PixelFormat::FORMAT_COUNT);
					static VulkanFormatInfo formats[formatCount];
					for (uint8_t i = 0; i < formatCount; i++) formats[i] = { VK_FORMAT_UNDEFINED, 0 };

					formats[static_cast<uint8_t>(Texture::PixelFormat::R8_SRGB)] = { VK_FORMAT_R8_SRGB, 1 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8_UNORM)] = { VK_FORMAT_R8_UNORM, 1 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8_SRGB)] = { VK_FORMAT_R8G8_SRGB, 2 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8_UNORM)] = { VK_FORMAT_R8G8_UNORM, 2 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8B8_SRGB)] = { VK_FORMAT_R8G8B8_SRGB, 3 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8B8_UNORM)] = { VK_FORMAT_R8G8B8_UNORM, 3 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::B8G8R8_SRGB)] = { VK_FORMAT_B8G8R8_SRGB, 3 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::B8G8R8_UNORM)] = { VK_FORMAT_B8G8R8_UNORM, 3 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8B8A8_SRGB)] = { VK_FORMAT_R8G8B8A8_SRGB, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R8G8B8A8_UNORM)] = { VK_FORMAT_R8G8B8A8_UNORM, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::B8G8R8A8_SRGB)] = { VK_FORMAT_B8G8R8A8_SRGB, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::B8G8R8A8_UNORM)] = { VK_FORMAT_B8G8R8A8_UNORM, 4 };

					formats[static_cast<uint8_t>(Texture::PixelFormat::R16_UINT)] = { VK_FORMAT_R16_UINT, 2 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16_SINT)] = { VK_FORMAT_R16_SINT, 2 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16_UNORM)] = { VK_FORMAT_R16_UNORM, 2 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16_SFLOAT)] = { VK_FORMAT_R16_SFLOAT, 2};
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16_UINT)] = { VK_FORMAT_R16G16_UINT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16_SINT)] = { VK_FORMAT_R16G16_SINT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16_UNORM)] = { VK_FORMAT_R16G16_UNORM, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16_SFLOAT)] = { VK_FORMAT_R16G16_SFLOAT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16_UINT)] = { VK_FORMAT_R16G16B16_UINT, 6 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16_SINT)] = { VK_FORMAT_R16G16B16_SINT, 6 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16_UNORM)] = { VK_FORMAT_R16G16B16_UNORM, 6 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16_SFLOAT)] = { VK_FORMAT_R16G16B16_SFLOAT, 6 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16A16_UINT)] = { VK_FORMAT_R16G16B16A16_UINT, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16A16_SINT)] = { VK_FORMAT_R16G16B16A16_SINT, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16A16_UNORM)] = { VK_FORMAT_R16G16B16A16_UNORM, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R16G16B16A16_SFLOAT)] = { VK_FORMAT_R16G16B16A16_SFLOAT, 8 };

					formats[static_cast<uint8_t>(Texture::PixelFormat::R32_UINT)] = { VK_FORMAT_R32_UINT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32_SINT)] = { VK_FORMAT_R32_SINT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32_SFLOAT)] = { VK_FORMAT_R32_SFLOAT, 4 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32_UINT)] = { VK_FORMAT_R32G32_UINT, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32_SINT)] = { VK_FORMAT_R32G32_SINT, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32_SFLOAT)] = { VK_FORMAT_R32G32_SFLOAT, 8 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32_UINT)] = { VK_FORMAT_R32G32B32_UINT, 12 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32_SINT)] = { VK_FORMAT_R32G32B32_SINT, 12 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32_SFLOAT)] = { VK_FORMAT_R32G32B32_SFLOAT, 12 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32A32_UINT)] = { VK_FORMAT_R32G32B32A32_UINT, 16 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32A32_SINT)] = { VK_FORMAT_R32G32B32A32_SINT, 16 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::R32G32B32A32_SFLOAT)] = { VK_FORMAT_R32G32B32A32_SFLOAT, 16 };

					formats[static_cast<uint8_t>(Texture::PixelFormat::D32_SFLOAT)] = { VK_FORMAT_D32_SFLOAT, 32 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::D32_SFLOAT_S8_UINT)] = { VK_FORMAT_D32_SFLOAT_S8_UINT, 5 };
					formats[static_cast<uint8_t>(Texture::PixelFormat::D24_UNORM_S8_UINT)] = { VK_FORMAT_D24_UNORM_S8_UINT, 4 };

					return formats;
				}();
			}

			Texture::PixelFormat VulkanImage::PixelFormatFromNativeFormat(VkFormat format) {
				static const std::unordered_map<VkFormat, PixelFormat>& FORMATS = []() -> std::unordered_map<VkFormat, PixelFormat>& {
					static std::unordered_map<VkFormat, PixelFormat> formats;
					static const uint8_t formatCount = static_cast<uint8_t>(Texture::PixelFormat::FORMAT_COUNT);
					for (size_t i = 0; i < formatCount; i++)
						formats.insert(std::make_pair(PIXEL_TO_NATIVE_FORMATS[i].format, static_cast<Texture::PixelFormat>(i)));
					return formats;
				}();
				std::unordered_map<VkFormat, PixelFormat>::const_iterator it = FORMATS.find(format);
				return (it == FORMATS.end()) ? Texture::PixelFormat::OTHER : it->second;
			}

			VkFormat VulkanImage::NativeFormatFromPixelFormat(PixelFormat format) {
				return (format < Texture::PixelFormat::FORMAT_COUNT) ? PIXEL_TO_NATIVE_FORMATS[static_cast<uint8_t>(format)].format : VK_FORMAT_UNDEFINED;
			}

			size_t VulkanImage::BytesPerPixel(PixelFormat format) {
				return (format < Texture::PixelFormat::FORMAT_COUNT) ? PIXEL_TO_NATIVE_FORMATS[static_cast<uint8_t>(format)].bytesPerPixel : 0;
			}

			VkImageType VulkanImage::NativeTypeFromTextureType(TextureType type) {
				static const VkImageType* TYPES = []() ->VkImageType* {
					static const uint8_t TYPE_COUNT = static_cast<uint8_t>(TextureType::TYPE_COUNT);
					static VkImageType types[TYPE_COUNT];
					for (uint8_t i = 0; i < TYPE_COUNT; i++) types[i] = VK_IMAGE_TYPE_MAX_ENUM;
					types[static_cast<uint8_t>(TextureType::TEXTURE_1D)] = VK_IMAGE_TYPE_1D;
					types[static_cast<uint8_t>(TextureType::TEXTURE_2D)] = VK_IMAGE_TYPE_2D;
					types[static_cast<uint8_t>(TextureType::TEXTURE_3D)] = VK_IMAGE_TYPE_3D;
					return types;
				}();
				return (type < TextureType::TYPE_COUNT) ? TYPES[static_cast<uint8_t>(type)] : VK_IMAGE_TYPE_MAX_ENUM;
			}


			Reference<TextureView> VulkanStaticImage::CreateView(TextureView::ViewType type
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount) {
				return Object::Instantiate<VulkanStaticTextureView>(this, type, baseMipLevel, mipLevelCount, baseArrayLayer, arrayLayerCount);
			}

			Reference<VulkanStaticImage> VulkanStaticImage::GetStaticHandle(VulkanCommandBuffer*) {
				return this;
			}
		}
	}
}
#pragma warning(default: 26812)
