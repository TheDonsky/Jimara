#include "VulkanImageView.h"
#pragma warning(disable: 26812)


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				static const VkImageViewType* VIEW_TYPES = []() -> VkImageViewType* {
					static const uint8_t TYPE_COUNT = static_cast<uint8_t>(TextureView::ViewType::TYPE_COUNT);
					static VkImageViewType types[TYPE_COUNT];
					for (uint8_t i = 0; i < TYPE_COUNT; i++) types[i] = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_1D)] = VK_IMAGE_VIEW_TYPE_1D;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_2D)] = VK_IMAGE_VIEW_TYPE_2D;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_3D)] = VK_IMAGE_VIEW_TYPE_3D;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_CUBE)] = VK_IMAGE_VIEW_TYPE_CUBE;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_1D_ARRAY)] = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_2D_ARRAY)] = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					types[static_cast<uint8_t>(TextureView::ViewType::VIEW_CUBE_ARRAY)] = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					return types;
				}();
			}

			VulkanImageView::VulkanImageView(
				VulkanImage* image, ViewType viewType
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount
				, VkImageAspectFlags aspectFlags)
				: m_image(image), m_viewType(viewType), m_aspectFlags(aspectFlags), m_view(VK_NULL_HANDLE) {
				VkImageViewCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					createInfo.image = *image;
					createInfo.viewType = (m_viewType < ViewType::TYPE_COUNT) ? VIEW_TYPES[static_cast<uint8_t>(m_viewType)] : VK_IMAGE_VIEW_TYPE_MAX_ENUM;
					createInfo.format = image->VulkanFormat();

					createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

					createInfo.subresourceRange.aspectMask = m_aspectFlags;
					{
						createInfo.subresourceRange.baseMipLevel = baseMipLevel;
						const uint32_t possibleMipLevels = image->MipLevels() - baseMipLevel;
						if (possibleMipLevels > mipLevelCount) mipLevelCount = possibleMipLevels;
						createInfo.subresourceRange.levelCount = mipLevelCount;
					}
					{
						createInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
						const uint32_t possibleArrayLayers = image->ArraySize() - baseArrayLayer;
						if (possibleArrayLayers > arrayLayerCount) arrayLayerCount = possibleArrayLayers;
						createInfo.subresourceRange.layerCount = arrayLayerCount;
					}
				}
				if (vkCreateImageView(*m_image->Device(), &createInfo, nullptr, &m_view) != VK_SUCCESS)
					m_image->Device()->Log()->Fatal("VulkanTextureView - Failed to create image views!");
			}

			VulkanImageView::~VulkanImageView() {
				if (m_view != VK_NULL_HANDLE) {
					vkDestroyImageView(*m_image->Device(), m_view, nullptr);
					m_view = VK_NULL_HANDLE;
				}
			}

			VulkanImage* VulkanImageView::Image()const { return m_image; }

			VulkanImageView::operator VkImageView()const { return m_view; }

			TextureView::ViewType VulkanImageView::Type()const { return m_viewType; }

			Texture* VulkanImageView::TargetTexture()const { return m_image; }
			
			Reference<TextureSampler> VulkanImageView::CreateSampler(TextureSampler::FilteringMode filtering, TextureSampler::WrappingMode wrapping, float lodBias) {
				m_image->Device()->Log()->Fatal("VulkanImageView - CreateSampler not yet implemented!");
				return nullptr;
			}
		}
	}
}
#pragma warning(default: 26812)
