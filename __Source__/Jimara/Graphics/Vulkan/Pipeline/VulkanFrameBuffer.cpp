#include "VulkanFrameBuffer.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanFrameBuffer::VulkanFrameBuffer(const std::vector<Reference<VulkanImageView>>& attachments) 
				: m_attachments(attachments), m_frameBuffer(VK_NULL_HANDLE) {

				assert(m_attachments.size() > 0);

				// "Compile" all image views togather:
				std::vector<VkImageView> views;
				for (size_t i = 0; i < m_attachments.size(); i++)
					views.push_back(*m_attachments[i]);

				VkFramebufferCreateInfo framebufferInfo = {};
				{
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					//framebufferInfo.renderPass = m_renderPass->RenderPass();
					framebufferInfo.attachmentCount = 3;
					//framebufferInfo.pAttachments = attachments;
					//VkExtent3D size = m_resolveAttachment.Image()->Size();
					//framebufferInfo.width = size.width;
					//framebufferInfo.height = size.height;
					framebufferInfo.layers = 1;
				}
				//if (vkCreateFramebuffer(m_resolveAttachment.Image()->Device()->Device(), &framebufferInfo, nullptr, &m_frameBuffer) != VK_SUCCESS)
				//	m_attachments[0]->Image()->Device()->Log()->Fatal("VulkanFrameBuffer - Failed to create framebuffer!");
			}

			VulkanFrameBuffer::~VulkanFrameBuffer() {

			}

			VulkanFrameBuffer::operator VkFramebuffer()const {
				return m_frameBuffer;
			}
		}
	}
}
