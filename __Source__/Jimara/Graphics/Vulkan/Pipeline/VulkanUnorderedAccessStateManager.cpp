#include "VulkanUnorderedAccessStateManager.h"
#include "../Memory/Textures/VulkanImage.h"
#include "../Memory/Textures/VulkanTextureView.h"
#include "VulkanCommandBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanUnorderedAccessStateManager::VulkanUnorderedAccessStateManager(VulkanCommandBuffer* commandBuffer)
				: m_commandBuffer(commandBuffer) {
				assert(m_commandBuffer != nullptr);
			}

			VulkanUnorderedAccessStateManager::~VulkanUnorderedAccessStateManager() {
				if (m_activeUnorderedAccess.Size() > 0u)
					m_commandBuffer->CommandPool()->Queue()->Device()->Log()->Error(
						"VulkanUnorderedAccessStateManager::~VulkanUnorderedAccessStateManager - ",
						"EnableUnorderedAccess was imvoked without corresponding DisableUnorderedAccess call! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			void VulkanUnorderedAccessStateManager::SetBindingSetInfo(const BindingSetRWImageInfo& info) {
				const size_t rwImageCount = (info.rwImages == nullptr) ? 0u : info.rwImageCount;
				if (m_boundSetInfos.Size() <= info.bindingSetIndex) {
					if (rwImageCount <= 0u) return;
					else m_boundSetInfos.Resize(size_t(info.bindingSetIndex) + 1u);
				}
				BoundSetRWImageInfo& boundImages = m_boundSetInfos[info.bindingSetIndex];
				boundImages.Clear();
				for (size_t i = 0u; i < rwImageCount; i++) {
					VulkanTextureView* image = info.rwImages[i];
					if (image == nullptr) continue;
					boundImages.Push(image);
				}
			}

			void VulkanUnorderedAccessStateManager::ClearBindingSetInfos() {
				for (size_t i = 0u; i < m_boundSetInfos.Size(); i++) {
					BindingSetRWImageInfo info = {};
					info.bindingSetIndex = static_cast<uint32_t>(i);
					SetBindingSetInfo(info);
				}
			}

			void VulkanUnorderedAccessStateManager::EnableUnorderedAccess(uint32_t bindingSetCount) {
				DisableUnorderedAccess();
				if (bindingSetCount >= m_boundSetInfos.Size())
					bindingSetCount = static_cast<uint32_t>(m_boundSetInfos.Size());
				assert(m_activeUnorderedAccess.Size() <= 0u);

				struct KnownImageInfo {
					VulkanImage* image = nullptr;
					size_t rangesIndex = ~size_t(0u);
				};

				struct ImageRange {
					uint32_t baseMipLevel = 0u;
					uint32_t mipLevelCount = 0u;
					uint32_t baseArrayLayer = 0u;
					uint32_t arrayLayerCount = 0u;
					size_t nextRange = ~size_t(0u);
				};

				// There will likely always be a really low amount of UAV-s active at a time, 
				// so using a fancy set or sometrrhing here would likely make stuff slower, not faster...
				static thread_local std::vector<KnownImageInfo> imageInfos;
				static thread_local std::vector<ImageRange> imageRanges;
				imageInfos.clear();
				imageRanges.clear();

				// Build linked lists of ranges per image:
				for (size_t bindingSetId = 0u; bindingSetId < bindingSetCount; bindingSetId++) {
					BoundSetRWImageInfo& boundImages = m_boundSetInfos[bindingSetId];
					for (size_t imageId = 0u; imageId < boundImages.Size(); imageId++) {
						VulkanTextureView* view = boundImages[imageId];
						VulkanImage* image = dynamic_cast<VulkanImage*>(view->TargetTexture());
						ImageRange imageRange = {};
						{
							imageRange.baseMipLevel = view->BaseMipLevel();
							imageRange.mipLevelCount = view->MipLevelCount();
							imageRange.baseArrayLayer = view->BaseArrayLayer();
							imageRange.arrayLayerCount = view->ArrayLayerCount();
							imageRange.nextRange = ~size_t(0u);
						}

						// Ignore if range does not cover anything:
						if (imageRange.mipLevelCount <= 0u || imageRange.arrayLayerCount <= 0u)
							continue;

						// Try finding a root record for the image:
						size_t knownImageId = imageInfos.size();
						for (size_t i = 0u; i < imageInfos.size(); i++)
							if (imageInfos[i].image == image) {
								knownImageId = i;
								break;
							}

						// If we do not have a root record, we might as well add it:
						if (knownImageId >= imageInfos.size()) {
							KnownImageInfo imageInfo = {};
							imageInfo.image = image;
							imageInfo.rangesIndex = imageRanges.size();
							imageRanges.push_back(imageRange);
							imageInfos.push_back(imageInfo);
							continue;
						}

						// Iterate over ranges and add new range if it is unique:
						KnownImageInfo imageInfo = imageInfos[knownImageId];
						size_t lastRangeIndex = imageInfo.rangesIndex;
						bool foundDuplicate = false;
						while (true) {
							const ImageRange& lastRange = imageRanges[lastRangeIndex];
							if (lastRange.baseMipLevel == imageRange.baseMipLevel &&
								lastRange.mipLevelCount == imageRange.mipLevelCount &&
								lastRange.baseArrayLayer == imageRange.baseArrayLayer &&
								lastRange.arrayLayerCount == imageRange.arrayLayerCount) {
								foundDuplicate = true;
								break;
							}
							else if (lastRange.nextRange < imageRanges.size())
								lastRangeIndex = lastRange.nextRange;
							else break;
						}
						if ((!foundDuplicate) && lastRangeIndex < imageRanges.size()) {
							imageRanges[lastRangeIndex].nextRange = imageRanges.size();
							imageRanges.push_back(imageRange);
						}
					}
				}

				// Extract ranges from linked lists:
				for (size_t imageIndex = 0u; imageIndex < imageInfos.size(); imageIndex++) {
					const KnownImageInfo info = imageInfos[imageIndex];
					
					// If we have a single range, no need to overcomplicate stuff:
					if (imageRanges[info.rangesIndex].nextRange >= imageRanges.size()) {
						TransitionedLayoutInfo transitionInfo = {};
						transitionInfo.image = info.image;
						const ImageRange range = imageRanges[info.rangesIndex];
						transitionInfo.baseMipLevel = range.baseMipLevel;
						transitionInfo.mipLevelCount = range.mipLevelCount;
						transitionInfo.baseArrayLayer = range.baseArrayLayer;
						transitionInfo.arrayLayerCount = range.arrayLayerCount;
						m_activeUnorderedAccess.Push(transitionInfo);
						continue;
					}

					// Store mip level and array layer counts:
					const uint32_t totalMipLevelCount = info.image->MipLevels();
					const uint32_t totalArrayLayerCount = info.image->ArraySize();

					// Also, if we have a single view that coveres entire image, we do not need complicated merge:
					{
						bool fullViewFound = false;
						size_t rangeIndex = info.rangesIndex;
						while (true) {
							const ImageRange& lastRange = imageRanges[rangeIndex];
							if (lastRange.baseMipLevel <= 0u && lastRange.mipLevelCount >= totalMipLevelCount &&
								lastRange.baseArrayLayer <= 0u && lastRange.arrayLayerCount >= totalArrayLayerCount) {
								fullViewFound = true;
								break;
							}
							else if (lastRange.nextRange >= imageRanges.size())
								break;
							else rangeIndex = lastRange.nextRange;
						}
						if (fullViewFound) {
							TransitionedLayoutInfo transitionInfo = {};
							transitionInfo.image = info.image;
							transitionInfo.baseMipLevel = 0u;
							transitionInfo.mipLevelCount = totalMipLevelCount;
							transitionInfo.baseArrayLayer = 0u;
							transitionInfo.arrayLayerCount = totalArrayLayerCount;
							m_activeUnorderedAccess.Push(transitionInfo);
							continue;
						}
					}

					// Sanity check for mipLevelCount, just in case:
					static const constexpr uint32_t MAX_SUPPORTED_MIP_LAYERS = static_cast<uint32_t>(sizeof(uint64_t) * 8u);
					static_assert(MAX_SUPPORTED_MIP_LAYERS == 64);
					if (totalMipLevelCount > MAX_SUPPORTED_MIP_LAYERS) {
						m_commandBuffer->CommandPool()->Queue()->Device()->Log()->Error(
							"VulkanUnorderedAccessStateManager::EnableUnorderedAccess - ",
							"Mip level count should not be more than ", MAX_SUPPORTED_MIP_LAYERS, " (got ", totalMipLevelCount, ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						continue;
					}

					// Build mip level bitmasks per array layer:
					static thread_local Stacktor<uint64_t> mipLevelsPerLayer;
					{
						// Cleanup:
						mipLevelsPerLayer.Resize(totalArrayLayerCount);
						for (size_t i = 0u; i < totalArrayLayerCount; i++)
							mipLevelsPerLayer[i] = 0u;

						size_t rangeIndex = info.rangesIndex;
						while (true) {
							// Make sure our range mask math works:
							static const constexpr uint64_t ALL_MIP_LEVELS = ~uint64_t(0u);
							static_assert((ALL_MIP_LEVELS << 1u) == (ALL_MIP_LEVELS - 1u));
							static_assert((ALL_MIP_LEVELS - (ALL_MIP_LEVELS >> 1u)) == (uint64_t(1u) << (MAX_SUPPORTED_MIP_LAYERS - 1u)));
#ifdef getRangeMaskFast
							static_assert(false);
#endif
#define getRangeMaskFast(baseLevel, levelCount) uint64_t((ALL_MIP_LEVELS << baseLevel) & (ALL_MIP_LEVELS >> (MAX_SUPPORTED_MIP_LAYERS - baseLevel - levelCount)))
							static_assert(getRangeMaskFast(0, 1) == 1u);	// 1 = 1
							static_assert(getRangeMaskFast(1, 1) == 2u);	// 2 = 2
							static_assert(getRangeMaskFast(2, 1) == 4u);	// 4 = 4
							static_assert(getRangeMaskFast(0, 2) == 3u);	// 1 + 2 = 3
							static_assert(getRangeMaskFast(1, 2) == 6u);	// 2 + 4 = 6
							static_assert(getRangeMaskFast(0, 5) == 31u);	// 1 + 2 + 4 + 8 + 16 = 31
							static_assert(getRangeMaskFast(1, 4) == 30u);	// 2 + 4 + 8 + 16 = 30
							static_assert(getRangeMaskFast(2, 3) == 28u);	// 4 + 8 + 16 = 28

							// Fill array layer bitmasks:
							const ImageRange& lastRange = imageRanges[rangeIndex];
							const uint64_t rangeMask = getRangeMaskFast(lastRange.baseMipLevel, lastRange.mipLevelCount);
							uint64_t* ptr = mipLevelsPerLayer.Data() + lastRange.baseArrayLayer;
							uint64_t* const end = ptr + Math::Min(lastRange.arrayLayerCount, totalArrayLayerCount - lastRange.baseArrayLayer);
							while (ptr < end) {
								(*ptr) |= rangeMask;
								ptr++;
							}
#undef getRangeMaskFast

							if (lastRange.nextRange >= imageRanges.size())
								break;
							else rangeIndex = lastRange.nextRange;
						}
					}

					// Build TransitionedLayoutInfo items from mipLevelsPerLayer:
					{
						assert(m_activeUnorderedAccess.Size() == 0u);
						uint64_t* ptr = mipLevelsPerLayer.Data();
						uint64_t* const end = ptr + totalArrayLayerCount;
						uint32_t firstArrayLayer = 0u;
						uint32_t layerCount = 0u;
						uint64_t rangeMask = 0u;
						auto extractTransitionInfos = [&]() {
							if (rangeMask == 0u || layerCount <= 0u) return;
							uint32_t firstMip = 0u;
							while (true) {
								while (firstMip < totalMipLevelCount &&
									((uint64_t(1u) << firstMip) & rangeMask) == 0u)
									firstMip++;
								if (firstMip >= totalMipLevelCount)
									break;
								uint32_t lastMip = firstMip;
								while (lastMip < totalMipLevelCount &&
									((uint64_t(1u) << lastMip) & rangeMask) != 0u)
									lastMip++;
								TransitionedLayoutInfo transitionInfo = {};
								{
									transitionInfo.image = info.image;
									transitionInfo.baseMipLevel = firstMip;
									transitionInfo.mipLevelCount = (lastMip - firstMip + 1u);
									transitionInfo.baseArrayLayer = firstArrayLayer;
									transitionInfo.arrayLayerCount = layerCount;
								}
								m_activeUnorderedAccess.Push(transitionInfo);
								firstMip = lastMip;
							}
						};
						while (ptr < end) {
							const uint64_t currentMask = (*ptr);
							ptr++;
							if (rangeMask == currentMask) 
								layerCount++;
							else {
								extractTransitionInfos();
								firstArrayLayer += layerCount;
								layerCount = 1u;
								rangeMask = currentMask;
							}
						}
						extractTransitionInfos();
					}
					mipLevelsPerLayer.Clear();
				}

				// Make Layout Transitions:
				{
					const TransitionedLayoutInfo* ptr = m_activeUnorderedAccess.Data();
					const TransitionedLayoutInfo* const end = ptr + m_activeUnorderedAccess.Size();
					while (ptr < end) {
						const TransitionedLayoutInfo& info = *ptr;
						ptr++;
						info.image->TransitionLayout(
							m_commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
							info.baseMipLevel, info.mipLevelCount, info.baseArrayLayer, info.arrayLayerCount);
					}
				}

				// Cleanup:
				imageInfos.clear();
				imageRanges.clear();
			}

			void VulkanUnorderedAccessStateManager::DisableUnorderedAccess() {
				const TransitionedLayoutInfo* ptr = m_activeUnorderedAccess.Data();
				const TransitionedLayoutInfo* const end = ptr + m_activeUnorderedAccess.Size();
				while (ptr < end) {
					const TransitionedLayoutInfo& info = *ptr;
					ptr++;
					info.image->TransitionLayout(
						m_commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						info.baseMipLevel, info.mipLevelCount, info.baseArrayLayer, info.arrayLayerCount);
				}
				m_activeUnorderedAccess.Clear();
			}
		}
	}
}
