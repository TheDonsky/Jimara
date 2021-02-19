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
					, Reference<TextureView>* resolveAttachments) {
					std::vector<Reference<ViewType>> attachments;

					const size_t colorAttachmentCount = renderPass->ColorAttachmentCount();

					const size_t fisrtColorAttachment = renderPass->FirstColorAttachmentId();
					const size_t lastColorAttachment = (fisrtColorAttachment + colorAttachmentCount);

					const size_t depthAttchmentId = renderPass->DepthAttachmentId();

					const size_t firstResolveAttachment = renderPass->FirstResolveAttachmentId();
					const size_t lastResolveAttachment = (firstResolveAttachment + colorAttachmentCount);

					size_t attachmentCount = lastColorAttachment;

					if (renderPass->HasDepthAttachment() && attachmentCount < (depthAttchmentId + 1))
						attachmentCount = (depthAttchmentId + 1);

					if (renderPass->HasResolveAttachments() && attachmentCount < lastResolveAttachment)
						attachmentCount = lastResolveAttachment;

					attachments.resize(attachmentCount);
					for (size_t i = fisrtColorAttachment; i < lastColorAttachment; i++)
						attachments[i] = colorAttachments[i];
					if (renderPass->HasDepthAttachment())
						attachments[depthAttchmentId] = dynamic_cast<ViewType*>(depthAttachment);
					if (renderPass->HasResolveAttachments()) for (size_t i = firstResolveAttachment; i < lastResolveAttachment; i++)
						attachments[i] = resolveAttachments[i - firstResolveAttachment];

					return attachments;
				}
			}

			VulkanDynamicFrameBuffer::VulkanDynamicFrameBuffer(VulkanRenderPass* renderPass
				, Reference<TextureView>* colorAttachments
				, Reference<TextureView> depthAttachment
				, Reference<TextureView>* resolveAttachments) 
				: m_renderPass(renderPass)
				, m_attachments(GetherAttachments<VulkanImageView>(renderPass, colorAttachments, depthAttachment, resolveAttachments)), m_size(0, 0) {
				if (m_attachments.size() > 0) m_size = m_attachments[0]->TargetTexture()->Size();
			}

			Reference<VulkanStaticFrameBuffer> VulkanDynamicFrameBuffer::GetStaticHandle(VulkanCommandBuffer* commanBuffer) {
				std::unique_lock<std::mutex> lock(m_staticBufferLock);

				static thread_local std::vector<Reference<VulkanStaticImageView>> staticViews;
				for (size_t i = 0; i < m_attachments.size(); i++) 
					staticViews.push_back(m_attachments[i]->GetStaticHandle(commanBuffer));
				
				bool needsRecreation = (m_staticFrameBuffer == nullptr);
				if (!needsRecreation) for (size_t i = 0; i < m_attachments.size(); i++)
					if (staticViews[i] != m_staticFrameBuffer->m_attachments[i]) {
						needsRecreation = true;
						break;
					}
				if (needsRecreation)
					m_staticFrameBuffer = Object::Instantiate<VulkanStaticFrameBuffer>(m_renderPass, staticViews);

				staticViews.clear();

				commanBuffer->RecordBufferDependency(m_staticFrameBuffer);
				return m_staticFrameBuffer;
			}

			Size2 VulkanDynamicFrameBuffer::Resolution()const {
				return m_size;
			}


			namespace {
				inline static VkFramebuffer CreateFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanStaticImageView>>& attachments, Size2& size) {
					if (attachments.size() <= 0) {
						size = Size2(0, 0);
						return VK_NULL_HANDLE;
					}

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

			VulkanStaticFrameBuffer::VulkanStaticFrameBuffer(VulkanRenderPass* renderPass
				, Reference<TextureView>* colorAttachments
				, Reference<TextureView> depthAttachment
				, Reference<TextureView>* resolveAttachments)
				: VulkanStaticFrameBuffer(renderPass, GetherAttachments<VulkanStaticImageView>(renderPass, colorAttachments, depthAttachment, resolveAttachments)) {}

			VulkanStaticFrameBuffer::VulkanStaticFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanStaticImageView>>& attachments)
				: m_renderPass(renderPass), m_attachments(attachments)
				, m_frameBuffer(VK_NULL_HANDLE), m_size(0, 0) {
				m_frameBuffer = CreateFrameBuffer(m_renderPass, m_attachments, m_size);
			}

			VulkanStaticFrameBuffer::~VulkanStaticFrameBuffer() {
				if (m_frameBuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(*dynamic_cast<VulkanDevice*>(m_renderPass->Device()), m_frameBuffer, nullptr);
					m_frameBuffer = VK_NULL_HANDLE;
				}
			}

			VulkanStaticFrameBuffer::operator VkFramebuffer()const {
				return m_frameBuffer;
			}

			Size2 VulkanStaticFrameBuffer::Resolution()const {
				return m_size;
			}

			Reference<VulkanStaticFrameBuffer> VulkanStaticFrameBuffer::GetStaticHandle(VulkanCommandBuffer*) {
				return this;
			}
		}
	}
}
