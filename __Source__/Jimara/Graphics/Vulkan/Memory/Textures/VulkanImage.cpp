#include "VulkanImage.h"
#include "../Textures/VulkanTextureView.h"
#include "../Buffers/VulkanArrayBuffer.h"
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
				, VkAccessFlags* srcAccessMask, VkAccessFlags* dstAccessMask, VkPipelineStageFlags* srcStage, VkPipelineStageFlags* dstStage, GraphicsDevice* device) {
				// Code below needs further examination...I do not understand what's going on...
				if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = 0;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_TRANSFER_WRITE_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_TRANSFER_READ_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
					if (srcAccessMask != nullptr) {
						if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
							(*srcAccessMask) = VK_ACCESS_TRANSFER_WRITE_BIT;
						else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
							(*srcAccessMask) = VK_ACCESS_TRANSFER_READ_BIT;
						else (*srcAccessMask) = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
					}
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;

					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
						(device->PhysicalDevice()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING)
							? VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR : 0u);
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
				else if (newLayout == VK_IMAGE_LAYOUT_GENERAL) {
					if (srcAccessMask != nullptr) (*srcAccessMask) = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
					if (dstAccessMask != nullptr) (*dstAccessMask) = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

					// __TODO__: srcStage & dstStage have to be different... INVESTIGATE!!!
					if (srcStage != nullptr) (*srcStage) = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_HOST_BIT;
					if (dstStage != nullptr) (*dstStage) = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				}
				else return false;
				return true;
			}

			VkImageMemoryBarrier VulkanImage::LayoutTransitionBarrier(
				VkImageLayout oldLayout, VkImageLayout newLayout
				, VkImageAspectFlags aspectFlags
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount
				, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)const {
				VkImageMemoryBarrier barrier = {};
				{
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.oldLayout = oldLayout;
					barrier.newLayout = newLayout;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.image = *this;
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

			VkImageMemoryBarrier VulkanImage::LayoutTransitionBarrier(VkImageLayout oldLayout, VkImageLayout newLayout
				, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount)const {

				VkAccessFlags srcAccessMask, dstAccessMask;
				if (!GetDefaultAccessMasksAndStages(oldLayout, newLayout, &srcAccessMask, &dstAccessMask, nullptr, nullptr, Device()))
					Device()->Log()->Error("VulkanImage::LayoutTransitionBarrier - Can not automatically deduce srcAccessMask and dstAccessMask");

				return LayoutTransitionBarrier(
					oldLayout, newLayout
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
				, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)const {
				
				if (oldLayout == newLayout) return;

				VkImageMemoryBarrier barrier = LayoutTransitionBarrier(
					oldLayout, newLayout
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
				, uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount)const {
				
				VkAccessFlags srcAccessMask, dstAccessMask;
				VkPipelineStageFlags srcStage, dstStage;
				if (!GetDefaultAccessMasksAndStages(oldLayout, newLayout, &srcAccessMask, &dstAccessMask, &srcStage, &dstStage, Device()))
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
					barrier.image = *this;
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

			void VulkanImage::Blit(CommandBuffer* commandBuffer, Texture* srcTexture, const SizeAABB& dstRegion, const SizeAABB& srcRegion) {

				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr) {
					Device()->Log()->Error("VulkanImage::Blit - invalid commandBuffer provided!");
					return;
				}

				VulkanImage* srcImage = dynamic_cast<VulkanImage*>(srcTexture);
				if (srcImage == nullptr) {
					Device()->Log()->Error("VulkanImage::Blit - invalid srcTexture provided!");
					return;
				}

				vulkanBuffer->RecordBufferDependency(this);

				vulkanBuffer->RecordBufferDependency(srcImage);

				const uint32_t sharedMipLevels = min(MipLevels(), srcImage->MipLevels());
				const uint32_t sharedArrayLayers = min(ArraySize(), srcImage->ArraySize());

				{
					TransitionLayout(
						vulkanBuffer, ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, sharedMipLevels, 0, sharedArrayLayers);

					srcImage->TransitionLayout(
						vulkanBuffer, srcImage->ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, sharedMipLevels, 0, sharedArrayLayers);
				}

				static thread_local std::vector<VkImageBlit> regions;
				regions.clear();
				for (uint32_t mipLevel = 0; mipLevel < sharedMipLevels; mipLevel++) {
					auto fitAABB = [](const SizeAABB& aabb, const Size3& size) {
						return SizeAABB(
							Size3(min(aabb.start.x, size.x), min(aabb.start.y, size.y), min(aabb.start.z, size.z)),
							Size3(min(aabb.end.x, size.x), min(aabb.end.y, size.y), min(aabb.end.z, size.z)));
					};
					auto toOffset3 = [&](const Size3& size) ->VkOffset3D {
						return { static_cast<int32_t>(size.x) >> mipLevel, static_cast<int32_t>(size.y) >> mipLevel, static_cast<int32_t>(size.z) >> mipLevel };
					};
					VkImageBlit blit = {};
					{
						const SizeAABB region = fitAABB(srcRegion, srcImage->Size());
						if (region.start.x >= region.end.x || region.start.y >= region.end.y) continue;
						blit.srcOffsets[0] = toOffset3(region.start);
						blit.srcOffsets[1] = toOffset3(region.end);
						blit.srcSubresource.aspectMask = srcImage->VulkanImageAspectFlags();
						blit.srcSubresource.mipLevel = mipLevel;
						blit.srcSubresource.baseArrayLayer = 0;
						blit.srcSubresource.layerCount = sharedArrayLayers;
					}
					{
						const SizeAABB region = fitAABB(dstRegion, Size());
						if (region.start.x >= region.end.x || region.start.y >= region.end.y) continue;
						blit.dstOffsets[0] = toOffset3(region.start);
						blit.dstOffsets[1] = toOffset3(region.end);
						blit.dstSubresource.aspectMask = VulkanImageAspectFlags();
						blit.dstSubresource.mipLevel = mipLevel;
						blit.dstSubresource.baseArrayLayer = 0;
						blit.dstSubresource.layerCount = sharedArrayLayers;
					}
					regions.push_back(blit);
				}
				if (regions.size() > 0) {
					vkCmdBlitImage(
						*vulkanBuffer,
						*srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						static_cast<uint32_t>(regions.size()), regions.data(), VK_FILTER_LINEAR);
					regions.clear();
				}

				{
					TransitionLayout(
						vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ShaderAccessLayout(), 0, sharedMipLevels, 0, sharedArrayLayers);

					srcImage->TransitionLayout(
						vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImage->ShaderAccessLayout(), 0, sharedMipLevels, 0, sharedArrayLayers);
				}
			}

			void VulkanImage::Copy(CommandBuffer* commandBuffer, Texture* srcTexture, const Size3& dstOffset, const Size3& srcOffset, const Size3& regionSize) {
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr) {
					Device()->Log()->Error("VulkanImage::Copy - invalid commandBuffer provided!");
					return;
				}

				VulkanImage* srcImage = dynamic_cast<VulkanImage*>(srcTexture);
				if (srcImage == nullptr) {
					Device()->Log()->Error("VulkanImage::Copy - invalid srcTexture provided!");
					return;
				}

				vulkanBuffer->RecordBufferDependency(this);
				vulkanBuffer->RecordBufferDependency(srcImage);

				const uint32_t sharedMipLevels = min(MipLevels(), srcImage->MipLevels());
				const uint32_t sharedArrayLayers = min(ArraySize(), srcImage->ArraySize());

				{
					TransitionLayout(
						vulkanBuffer, ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, sharedMipLevels, 0, sharedArrayLayers);

					srcImage->TransitionLayout(
						vulkanBuffer, srcImage->ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0, sharedMipLevels, 0, sharedArrayLayers);
				}

				static thread_local std::vector<VkImageCopy> regions;
				regions.clear();
				for (uint32_t mipLevel = 0; mipLevel < sharedMipLevels; mipLevel++) {
					auto toMipExtent = [&](const Size3& size) -> VkExtent3D {
						return { size.x >> mipLevel, size.y >> mipLevel, size.z >> mipLevel };
					};
					auto toMipOffset = [&](const Size3& size) -> VkOffset3D {
						const VkExtent3D extent = toMipExtent(size);
						return { static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height), static_cast<int32_t>(extent.depth) };
					};
					const VkExtent3D srcMipSize = toMipExtent(srcImage->Size());
					const VkExtent3D dstMipSize = toMipExtent(Size());
					VkImageCopy copy = {};
					{
						copy.srcSubresource.aspectMask = srcImage->VulkanImageAspectFlags();
						copy.srcSubresource.mipLevel = mipLevel;
						copy.srcSubresource.baseArrayLayer = 0;
						copy.srcSubresource.layerCount = sharedArrayLayers;
						copy.srcOffset = toMipOffset(srcOffset);
						if (static_cast<uint32_t>(copy.srcOffset.x) >= srcMipSize.width || 
							static_cast<uint32_t>(copy.srcOffset.y) >= srcMipSize.height || 
							static_cast<uint32_t>(copy.srcOffset.z) >= srcMipSize.depth)
							continue;
					}
					{
						copy.dstSubresource.aspectMask = VulkanImageAspectFlags();
						copy.dstSubresource.mipLevel = mipLevel;
						copy.dstSubresource.baseArrayLayer = 0;
						copy.dstSubresource.layerCount = sharedArrayLayers;
						copy.dstOffset = toMipOffset(dstOffset);
						if (static_cast<uint32_t>(copy.dstOffset.x) >= dstMipSize.width || 
							static_cast<uint32_t>(copy.dstOffset.y) >= dstMipSize.height || 
							static_cast<uint32_t>(copy.dstOffset.z) >= dstMipSize.depth)
							continue;
					}
					{
						const VkExtent3D mipRegionSize = toMipExtent(regionSize);
						const VkExtent3D maxSrcRegionSize = { 
							static_cast<uint32_t>(srcMipSize.width - copy.srcOffset.x), 
							static_cast<uint32_t>(srcMipSize.height - copy.srcOffset.y),
							static_cast<uint32_t>(srcMipSize.depth - copy.srcOffset.z)
						};
						const VkExtent3D maxDstRegionSize = { 
							static_cast<uint32_t>(dstMipSize.width - copy.dstOffset.x),
							static_cast<uint32_t>(dstMipSize.height - copy.dstOffset.y),
							static_cast<uint32_t>(dstMipSize.depth - copy.dstOffset.z)
						};
						copy.extent.width = Math::Min(mipRegionSize.width, maxSrcRegionSize.width, maxDstRegionSize.width);
						copy.extent.height = Math::Min(mipRegionSize.height, maxSrcRegionSize.height, maxDstRegionSize.height);
						copy.extent.depth = Math::Min(mipRegionSize.depth, maxSrcRegionSize.depth, maxDstRegionSize.depth);
						assert(copy.extent.width <= srcMipSize.width);
						assert(copy.extent.height <= srcMipSize.height);
						assert(copy.extent.depth <= srcMipSize.depth);
						if (copy.extent.width <= 0 || copy.extent.height <= 0 || copy.extent.depth <= 0) continue;
					}
					regions.push_back(copy);
				}
				if (regions.size() > 0) {
					vkCmdCopyImage(
						*vulkanBuffer,
						*srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						static_cast<uint32_t>(regions.size()), regions.data());
					regions.clear();
				}

				{
					TransitionLayout(
						vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ShaderAccessLayout(), 0, sharedMipLevels, 0, sharedArrayLayers);

					srcImage->TransitionLayout(
						vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImage->ShaderAccessLayout(), 0, sharedMipLevels, 0, sharedArrayLayers);
				}
			}

			void VulkanImage::Copy(CommandBuffer* commandBuffer, ArrayBuffer* srcBuffer,
				const Size3& bufferImageLayerSize, const Size3& dstOffset, const Size3& srcOffset, const Size3& regionSize) {
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr) {
					Device()->Log()->Error("VulkanImage::Copy - invalid commandBuffer provided!");
					return;
				}

				if (regionSize.x <= 0u || regionSize.y <= 0u || regionSize.z <= 0u)
					return; // Nothing to copy...

				const Size3 size = Size();
				if (dstOffset.x >= size.x || dstOffset.y >= size.y || dstOffset.z >= size.z)
					return; // Nothing to copy...

				VulkanArrayBuffer* srcBuf = dynamic_cast<VulkanArrayBuffer*>(srcBuffer);
				if (srcBuf == nullptr) {
					Device()->Log()->Error("VulkanImage::Copy - invalid srcBuffer provided!");
					return;
				}

				vulkanBuffer->RecordBufferDependency(this);
				vulkanBuffer->RecordBufferDependency(srcBuffer);

				const uint32_t mipLevels = 1u;
				const uint32_t arrayLayers = 1u;

				TransitionLayout(
					vulkanBuffer, ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, mipLevels, 0, arrayLayers);

				VkBufferImageCopy region = {};
				{
					region.bufferOffset = TexelSize(ImageFormat()) *
						(static_cast<VkDeviceSize>(bufferImageLayerSize.x) *
							(static_cast<VkDeviceSize>(bufferImageLayerSize.y) * srcOffset.z + srcOffset.y) + srcOffset.x);
					region.bufferRowLength = bufferImageLayerSize.x;
					region.bufferImageHeight = bufferImageLayerSize.y;

					region.imageSubresource.aspectMask = VulkanImageAspectFlags();
					region.imageSubresource.mipLevel = 0;
					region.imageSubresource.baseArrayLayer = 0;
					region.imageSubresource.layerCount = ArraySize();

					region.imageOffset.x = dstOffset.x;
					region.imageOffset.y = dstOffset.y;
					region.imageOffset.z = dstOffset.z;
					region.imageExtent.width = Math::Min(regionSize.x, size.x - dstOffset.x);
					region.imageExtent.height = Math::Min(regionSize.y, size.y - dstOffset.y);
					region.imageExtent.depth = Math::Min(regionSize.z, size.z - dstOffset.z);
				}
				vkCmdCopyBufferToImage(*vulkanBuffer, *srcBuf, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				TransitionLayout(
					vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ShaderAccessLayout(), 0, mipLevels, 0, arrayLayers);
			}

			void VulkanImage::Clear(CommandBuffer * commandBuffer, const Vector4& color,
				uint32_t baseMipLevel, uint32_t mipLevelCount, uint32_t baseArrayLayer, uint32_t arrayLayerCount) {
				
				if (baseMipLevel >= MipLevels() || baseArrayLayer >= ArraySize())
					return;
				
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr) {
					Device()->Log()->Error("VulkanImage::Copy - invalid commandBuffer provided!");
					return;
				}

				vulkanBuffer->RecordBufferDependency(this);

				VkClearColorValue clearColor = {};
				{
					clearColor.float32[0u] = color.r;
					clearColor.float32[1u] = color.g;
					clearColor.float32[2u] = color.b;
					clearColor.float32[3u] = color.a;
				}

				VkImageSubresourceRange range = {};
				{
					range.aspectMask = VulkanImageAspectFlags();
					range.baseMipLevel = baseMipLevel;
					range.levelCount = Math::Min(mipLevelCount, MipLevels() - baseMipLevel);
					range.baseArrayLayer = baseArrayLayer;
					range.layerCount = Math::Min(arrayLayerCount, ArraySize() - baseArrayLayer);
				}


				TransitionLayout(
					vulkanBuffer, ShaderAccessLayout(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount);

				vkCmdClearColorImage(*vulkanBuffer, *this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1u, &range);

				TransitionLayout(
					vulkanBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ShaderAccessLayout(), 
					range.baseMipLevel, range.levelCount, range.baseArrayLayer, range.layerCount);
			}

			void VulkanImage::GenerateMipmaps(CommandBuffer* commandBuffer) {
				VulkanCommandBuffer* vulkanBuffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
				if (vulkanBuffer == nullptr) {
					Device()->Log()->Error("VulkanImage::GenerateMipmaps - invalid commandBuffer provided!");
					return;
				}
				vulkanBuffer->RecordBufferDependency(this);
				GenerateMipmaps(vulkanBuffer, ShaderAccessLayout(), ShaderAccessLayout());
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


			Reference<TextureView> VulkanImage::CreateView(TextureView::ViewType type
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount) {
				return Object::Instantiate<VulkanTextureView>(this, type, baseMipLevel, mipLevelCount, baseArrayLayer, arrayLayerCount);
			}
		}
	}
}
#pragma warning(default: 26812)
