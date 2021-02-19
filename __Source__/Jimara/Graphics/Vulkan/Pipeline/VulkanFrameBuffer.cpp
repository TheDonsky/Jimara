#include "VulkanFrameBuffer.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanFrameBuffer::VulkanFrameBuffer(VulkanRenderPass* renderPass
				, Reference<TextureView>* colorAttachments
				, Reference<TextureView> depthAttachment
				, Reference<TextureView>* resolveAttachments)
				: m_renderPass(renderPass), m_frameBuffer(VK_NULL_HANDLE), m_size(0, 0) {

				const size_t colorAttachmentCount = m_renderPass->ColorAttachmentCount();
				
				const size_t fisrtColorAttachment = m_renderPass->FirstColorAttachmentId();
				const size_t lastColorAttachment = (fisrtColorAttachment + colorAttachmentCount);

				const size_t depthAttchmentId = m_renderPass->DepthAttachmentId();
				
				const size_t firstResolveAttachment = m_renderPass->FirstResolveAttachmentId();
				const size_t lastResolveAttachment = (firstResolveAttachment + colorAttachmentCount);
				
				size_t attachmentCount = lastColorAttachment;

				if (m_renderPass->HasDepthAttachment() && attachmentCount < (depthAttchmentId + 1))
					attachmentCount = (depthAttchmentId + 1);

				if (m_renderPass->HasResolveAttachments() && attachmentCount < lastResolveAttachment)
					attachmentCount = lastResolveAttachment;

				if (attachmentCount <= 0) return;

				m_attachments.resize(attachmentCount);
				for (size_t i = fisrtColorAttachment; i < lastColorAttachment; i++)
					m_attachments[i] = colorAttachments[i];
				if (m_renderPass->HasDepthAttachment())
					m_attachments[depthAttchmentId] = depthAttachment;
				if (m_renderPass->HasResolveAttachments()) for (size_t i = firstResolveAttachment; i < lastResolveAttachment; i++)
					m_attachments[i] = resolveAttachments[i - firstResolveAttachment];

				// "Compile" all image views togather:
				static thread_local std::vector<VkImageView> views;
				if (views.size() < m_attachments.size())
					views.resize(m_attachments.size());
				for (size_t i = 0; i < m_attachments.size(); i++)
					views[i] = *m_attachments[i];

				// Create frame buffer from views:
				VkFramebufferCreateInfo framebufferInfo = {};
				{
					framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferInfo.renderPass = *m_renderPass;
					framebufferInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
					framebufferInfo.pAttachments = views.data();
					m_size = m_attachments[0]->TargetTexture()->Size();
					framebufferInfo.width = m_size.x;
					framebufferInfo.height = m_size.y;
					framebufferInfo.layers = 1;
				}
				if (vkCreateFramebuffer(*m_renderPass->Device(), &framebufferInfo, nullptr, &m_frameBuffer) != VK_SUCCESS)
					m_renderPass->Device()->Log()->Fatal("VulkanFrameBuffer - Failed to create framebuffer!");
			}

			VulkanFrameBuffer::~VulkanFrameBuffer() {
				if (m_frameBuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(*m_renderPass->Device(), m_frameBuffer, nullptr);
					m_frameBuffer = VK_NULL_HANDLE;
				}
			}

			VulkanFrameBuffer::operator VkFramebuffer()const {
				return m_frameBuffer;
			}

			Size2 VulkanFrameBuffer::Resolution()const {
				return m_size;
			}
		}
	}
}
