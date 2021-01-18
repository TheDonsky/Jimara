#include "VulkanImageView.h"
#pragma warning(disable: 26812)


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanImageView::VulkanImageView(VulkanImage* image, VkImageViewType viewType, VkImageAspectFlags aspectFlags)
				: m_image(image), m_viewType(viewType), m_aspectFlags(aspectFlags), m_view(VK_NULL_HANDLE) {
				VkImageViewCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					createInfo.image = *image;
					createInfo.viewType = m_viewType;
					createInfo.format = image->Format();

					createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

					createInfo.subresourceRange.aspectMask = m_aspectFlags;
					createInfo.subresourceRange.baseMipLevel = 0;
					createInfo.subresourceRange.levelCount = image->MipLevels();
					createInfo.subresourceRange.baseArrayLayer = 0;
					createInfo.subresourceRange.layerCount = 1;
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
		}
	}
}
#pragma warning(default: 26812)
