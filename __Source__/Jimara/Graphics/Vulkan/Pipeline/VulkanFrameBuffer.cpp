#include "VulkanFrameBuffer.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanFrameBuffer::VulkanFrameBuffer(const std::vector<Reference<VulkanStaticImageView>>& attachments, VkRenderPass renderPass)
				: m_attachments(attachments), m_frameBuffer(VK_NULL_HANDLE) {

				assert(m_attachments.size() > 0);

				// "Compile" all image views togather:
				std::vector<VkImageView> views;
				for (size_t i = 0; i < m_attachments.size(); i++)
					views.push_back(*m_attachments[i]);

				// Create frame buffer from views:
				VkFramebufferCreateInfo framebufferInfo = {};
				{
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferInfo.renderPass = renderPass;
					framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
					framebufferInfo.pAttachments = views.data();
					Size2 size = m_attachments[0]->TargetTexture()->Size();
					framebufferInfo.width = size.x;
					framebufferInfo.height = size.y;
					framebufferInfo.layers = 1;
				}
				if (vkCreateFramebuffer(*dynamic_cast<VulkanImage*>(m_attachments[0]->TargetTexture())->Device(), &framebufferInfo, nullptr, &m_frameBuffer) != VK_SUCCESS)
					dynamic_cast<VulkanImage*>(m_attachments[0]->TargetTexture())->Device()->Log()->Fatal("VulkanFrameBuffer - Failed to create framebuffer!");
			}

			VulkanFrameBuffer::~VulkanFrameBuffer() {
				if (m_frameBuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(*dynamic_cast<VulkanImage*>(m_attachments[0]->TargetTexture())->Device(), m_frameBuffer, nullptr);
					m_frameBuffer = VK_NULL_HANDLE;
				}
			}

			VulkanFrameBuffer::operator VkFramebuffer()const {
				return m_frameBuffer;
			}
		}
	}
}
