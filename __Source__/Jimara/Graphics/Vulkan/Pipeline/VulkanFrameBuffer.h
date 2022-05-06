#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanFrameBuffer;
			class VulkanDynamicFrameBuffer;
			class VulkanStaticFrameBuffer;
		}
	}
}
#include "../Memory/TextureViews/VulkanTextureView.h"
#include "VulkanRenderPass.h"
#include <vector>
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed frame buffer
			/// </summary>
			class VulkanFrameBuffer : public virtual FrameBuffer {
			public:
				/// <summary>
				/// Access immutable handle to VkFramebuffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer thar may rely on the resource </param>
				/// <returns> Reference to the buffer </returns>
				virtual Reference<VulkanStaticFrameBuffer> GetStaticHandle(VulkanCommandBuffer* commanBuffer) = 0;
			};

			/// <summary>
			/// Vulkan-backed frame buffer, comprized of arbitrary texture views
			/// </summary>
			class VulkanDynamicFrameBuffer : public virtual VulkanFrameBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass (has to have at least one attachment) </param>
				/// <param name="colorAttachments"> Color attachments (can be nullptr if render pass has no color attachments) </param>
				/// <param name="depthAttachment"> Depth attachment (can be nullptr if render pass has no depth attachment) </param>
				/// <param name="colorResolveAttachments"> Resolve attachments for colorAttachments (can be nullptr if render pass does not resolve color) </param>
				/// <param name="depthResolveAttachment"> Resolve attachments for depthAttachment (can be nullptr if render pass does not resolve depth) </param>
				VulkanDynamicFrameBuffer(VulkanRenderPass* renderPass
					, Reference<TextureView>* colorAttachments
					, Reference<TextureView> depthAttachment
					, Reference<TextureView>* colorResolveAttachments
					, Reference<TextureView> depthResolveAttachment);

				/// <summary>
				/// Access immutable handle to VkFramebuffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer thar may rely on the resource </param>
				/// <returns> Reference to the buffer </returns>
				virtual Reference<VulkanStaticFrameBuffer> GetStaticHandle(VulkanCommandBuffer* commanBuffer) override;

				/// <summary> Image size per attachment </summary>
				virtual Size2 Resolution()const override;

			private:
				// Render pass
				const Reference<VulkanRenderPass> m_renderPass;

				// Attachments
				const std::vector<Reference<VulkanImageView>> m_attachments;

				// Frame buffer resolution
				Size2 m_size;

				// Actual VkFrameBuffer instance
				Reference<VulkanStaticFrameBuffer> m_staticFrameBuffer;

				// Lock for static buffer update
				std::mutex m_staticBufferLock;
			};

			/// <summary> Wrapper on top of a VkFramebuffer object </summary>
			class VulkanStaticFrameBuffer : public virtual VulkanFrameBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass (has to have at least one attachment) </param>
				/// <param name="colorAttachments"> Color attachments (can be nullptr if render pass has no color attachments) </param>
				/// <param name="depthAttachment"> Depth attachment (can be nullptr if render pass has no depth attachment) </param>
				/// <param name="colorResolveAttachments"> Resolve attachments for colorAttachments (can be nullptr if render pass does not resolve color) </param>
				/// <param name="depthResolveAttachment"> Resolve attachments for depthAttachment (can be nullptr if render pass does not resolve depth) </param>
				VulkanStaticFrameBuffer(VulkanRenderPass* renderPass
					, Reference<TextureView>* colorAttachments
					, Reference<TextureView> depthAttachment
					, Reference<TextureView>* colorResolveAttachments
					, Reference<TextureView> depthResolveAttachment);

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass </param>
				/// <param name="attachments"> "Precompiled" attachment list </param>
				VulkanStaticFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanStaticImageView>>& attachments);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanStaticFrameBuffer();

				/// <summary> Type cast to the underlying API object </summary>
				operator VkFramebuffer()const;

				/// <summary> Image size per attachment </summary>
				virtual Size2 Resolution()const override;

				/// <summary>
				/// Access immutable handle to VkFramebuffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer thar may rely on the resource </param>
				/// <returns> Reference to the buffer </returns>
				virtual Reference<VulkanStaticFrameBuffer> GetStaticHandle(VulkanCommandBuffer* commanBuffer) override;


			private:
				// Render pass
				const Reference<VulkanRenderPass> m_renderPass;

				// Attachments
				const std::vector<Reference<VulkanStaticImageView>> m_attachments;
				
				// Underlying API object
				VkFramebuffer m_frameBuffer;

				// Frame buffer resolution
				Size2 m_size;

				// VulkanDynamicFrameBuffer has to check frame buffer insternals
				friend class VulkanDynamicFrameBuffer;
			};
		}
	}
}
