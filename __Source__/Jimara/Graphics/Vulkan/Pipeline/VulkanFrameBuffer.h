#pragma once
#include "../Memory/TextureViews/VulkanTextureView.h"
#include "VulkanRenderPass.h"
#include <vector>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary> Wrapper on top of a VkFramebuffer object </summary>
			class VulkanFrameBuffer : public virtual FrameBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass (has to have at least one attachment) </param>
				/// <param name="colorAttachments"> Color attachments (can be nullptr if render pass has no color attachments) </param>
				/// <param name="depthAttachment"> Depth attachment (can be nullptr if render pass has no depth attachment) </param>
				/// <param name="resolveAttachments"> Resolve attachments (can be nullptr if render pass has no resolve attachments) </param>
				VulkanFrameBuffer(VulkanRenderPass* renderPass
					, Reference<TextureView>* colorAttachments
					, Reference<TextureView> depthAttachment
					, Reference<TextureView>* resolveAttachments);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanFrameBuffer();

				/// <summary> Type cast to the underlying API object </summary>
				operator VkFramebuffer()const;

				/// <summary> Image size per attachment </summary>
				virtual Size2 Resolution()const override;


			private:
				// Render pass
				const Reference<VulkanRenderPass> m_renderPass;

				// Attachments
				std::vector<Reference<VulkanStaticImageView>> m_attachments;
				
				// Underlying API object
				VkFramebuffer m_frameBuffer;

				// Frame buffer resolution
				Size2 m_size;
			};
		}
	}
}
