#include "VulkanFrameBuffer.h"
#include <cassert>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				template<typename ViewType>
				inline static std::vector<Reference<ViewType>> GetherAttachments(VulkanRenderPass* renderPass
					, Reference<TextureView>* colorAttachments
					, TextureView* depthAttachment
					, Reference<TextureView>* colorResolveAttachments
					, TextureView* depthResolveAttachment) {
					std::vector<Reference<ViewType>> attachments;

					const size_t colorAttachmentCount = renderPass->ColorAttachmentCount();

					const size_t fisrtColorAttachment = renderPass->FirstColorAttachmentId();
					const size_t lastColorAttachment = (fisrtColorAttachment + colorAttachmentCount);

					const size_t depthAttchmentId = renderPass->DepthAttachmentId();
					const size_t depthResolveAttachmentId = renderPass->DepthResolveAttachmentId();

					const size_t firstResolveAttachment = renderPass->FirstResolveAttachmentId();
					const size_t lastResolveAttachment = (firstResolveAttachment + colorAttachmentCount);

					size_t attachmentCount = lastColorAttachment;

					if (renderPass->ResolvesDepth() && attachmentCount < (depthResolveAttachmentId + 1))
						attachmentCount = (depthResolveAttachmentId + 1);
					if (renderPass->HasDepthAttachment() && attachmentCount < (depthAttchmentId + 1))
						attachmentCount = (depthAttchmentId + 1);

					if (renderPass->ResolvesColor() && attachmentCount < lastResolveAttachment)
						attachmentCount = lastResolveAttachment;

					attachments.resize(attachmentCount);
					for (size_t i = fisrtColorAttachment; i < lastColorAttachment; i++)
						attachments[i] = colorAttachments[i];
					if (renderPass->HasDepthAttachment())
						attachments[depthAttchmentId] = dynamic_cast<ViewType*>(depthAttachment);
					if (renderPass->ResolvesDepth())
						attachments[depthResolveAttachmentId] = dynamic_cast<ViewType*>(depthResolveAttachment);
					if (renderPass->ResolvesColor()) for (size_t i = firstResolveAttachment; i < lastResolveAttachment; i++)
						attachments[i] = colorResolveAttachments[i - firstResolveAttachment];

					return attachments;
				}
			}

			namespace {
				inline static VkFramebuffer CreateFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanTextureView>>& attachments, Size2& size) {
					// "Compile" all image views togather:
					static thread_local std::vector<VkImageView> views;
					if (views.size() < attachments.size())
						views.resize(attachments.size());
					for (size_t i = 0; i < attachments.size(); i++)
						views[i] = *attachments[i];

					// Create frame buffer from views:
					VkFramebufferCreateInfo framebufferInfo = {};
					{
						framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
						framebufferInfo.renderPass = *renderPass;
						framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
						framebufferInfo.pAttachments = views.data();
						if (attachments.size() > 0u)
							size = attachments[0]->TargetTexture()->Size();
						framebufferInfo.width = size.x;
						framebufferInfo.height = size.y;
						framebufferInfo.layers = 1;
					}
					VkFramebuffer frameBuffer = VK_NULL_HANDLE;
					if (vkCreateFramebuffer(*dynamic_cast<VulkanDevice*>(renderPass->Device()), &framebufferInfo, nullptr, &frameBuffer) != VK_SUCCESS) {
						frameBuffer = VK_NULL_HANDLE;
						renderPass->Device()->Log()->Fatal("VulkanFrameBuffer - Failed to create framebuffer!");
					}
					return frameBuffer;
				}
			}

			VulkanFrameBuffer::VulkanFrameBuffer(VulkanRenderPass* renderPass
				, Reference<TextureView>* colorAttachments
				, Reference<TextureView> depthAttachment
				, Reference<TextureView>* colorResolveAttachments
				, Reference<TextureView> depthResolveAttachment)
				: VulkanFrameBuffer(
					renderPass, GetherAttachments<VulkanTextureView>(renderPass, colorAttachments, depthAttachment, colorResolveAttachments, depthResolveAttachment)) {}

			VulkanFrameBuffer::VulkanFrameBuffer(VulkanRenderPass* renderPass, Size2 size) 
				: m_renderPass(renderPass), m_frameBuffer(VK_NULL_HANDLE), m_size(size) {
				m_frameBuffer = CreateFrameBuffer(m_renderPass, m_attachments, m_size);
			}

			VulkanFrameBuffer::VulkanFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanTextureView>>& attachments)
				: m_renderPass(renderPass), m_attachments(attachments)
				, m_frameBuffer(VK_NULL_HANDLE), m_size(0, 0) {
				m_frameBuffer = CreateFrameBuffer(m_renderPass, m_attachments, m_size);
			}

			VulkanFrameBuffer::~VulkanFrameBuffer() {
				if (m_frameBuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(*dynamic_cast<VulkanDevice*>(m_renderPass->Device()), m_frameBuffer, nullptr);
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
