#include "VulkanStaticTextureView.h"
#include "../TextureSamplers/VulkanStaticTextureSampler.h"


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

			VulkanStaticTextureView::VulkanStaticTextureView(
				VulkanStaticImage* image, ViewType viewType
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount)
				: m_image(image), m_viewType(viewType)
				, m_baseMipLevel(min(image->MipLevels(), baseMipLevel)), m_mipLevelCount(min(image->MipLevels() - min(image->MipLevels(), baseMipLevel), mipLevelCount))
				, m_baseArrayLayer(min(image->ArraySize(), baseArrayLayer)), m_arrayLayerCount(min(image->ArraySize() - min(image->ArraySize(), baseArrayLayer), arrayLayerCount))
				, m_view(VK_NULL_HANDLE) {
				VkImageViewCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					createInfo.image = *m_image;
					createInfo.viewType = (m_viewType < ViewType::TYPE_COUNT) ? VIEW_TYPES[static_cast<uint8_t>(m_viewType)] : VK_IMAGE_VIEW_TYPE_MAX_ENUM;
					createInfo.format = m_image->VulkanFormat();

					createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

					createInfo.subresourceRange.aspectMask = m_image->VulkanImageAspectFlags();
					{
						createInfo.subresourceRange.baseMipLevel = m_baseMipLevel;
						createInfo.subresourceRange.levelCount = m_mipLevelCount;
					}
					{
						createInfo.subresourceRange.baseArrayLayer = m_baseArrayLayer;
						createInfo.subresourceRange.layerCount = m_arrayLayerCount;
					}
				}
				if (vkCreateImageView(*m_image->Device(), &createInfo, nullptr, &m_view) != VK_SUCCESS)
					m_image->Device()->Log()->Fatal("VulkanTextureView - Failed to create image views!");
			}

			VulkanStaticTextureView::~VulkanStaticTextureView() {
				if (m_view != VK_NULL_HANDLE) {
					vkDestroyImageView(*m_image->Device(), m_view, nullptr);
					m_view = VK_NULL_HANDLE;
				}
			}

			VulkanStaticTextureView::operator VkImageView()const { 
				return m_view; 
			}

			TextureView::ViewType VulkanStaticTextureView::Type()const { 
				return m_viewType; 
			}

			Texture* VulkanStaticTextureView::TargetTexture()const { 
				return m_image; 
			}

			uint32_t VulkanStaticTextureView::BaseMipLevel()const { 
				return m_baseMipLevel; 
			}

			uint32_t VulkanStaticTextureView::MipLevelCount()const { 
				return m_mipLevelCount; 
			}

			uint32_t VulkanStaticTextureView::BaseArrayLayer()const { 
				return m_baseArrayLayer; 
			}

			uint32_t VulkanStaticTextureView::ArrayLayerCount()const { 
				return m_arrayLayerCount; 
			}
			
			Reference<TextureSampler> VulkanStaticTextureView::CreateSampler(TextureSampler::FilteringMode filtering, TextureSampler::WrappingMode wrapping, float lodBias) {
				return Object::Instantiate<VulkanStaticTextureSampler>(this, filtering, wrapping, lodBias);
			}
		}
	}
}
#pragma warning(default: 26812)
