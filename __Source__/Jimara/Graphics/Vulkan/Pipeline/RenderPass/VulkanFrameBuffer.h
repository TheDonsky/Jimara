#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanFrameBuffer;
		}
	}
}
#include "../../Memory/Textures/VulkanTextureView.h"
#include "VulkanRenderPass.h"
#include <vector>
#include <mutex>


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed frame buffer
			/// </summary>
			class JIMARA_API VulkanFrameBuffer : public virtual FrameBuffer {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass (has to have at least one attachment) </param>
				/// <param name="colorAttachments"> Color attachments (can be nullptr if render pass has no color attachments) </param>
				/// <param name="depthAttachment"> Depth attachment (can be nullptr if render pass has no depth attachment) </param>
				/// <param name="colorResolveAttachments"> Resolve attachments for colorAttachments (can be nullptr if render pass does not resolve color) </param>
				/// <param name="depthResolveAttachment"> Resolve attachments for depthAttachment (can be nullptr if render pass does not resolve depth) </param>
				VulkanFrameBuffer(VulkanRenderPass* renderPass
					, const Reference<TextureView>* colorAttachments
					, const Reference<TextureView>& depthAttachment
					, const Reference<TextureView>* colorResolveAttachments
					, const Reference<TextureView>& depthResolveAttachment);

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass (has to have no attachments) </param>
				/// <param name="size"> Empty frame buffer size </param>
				VulkanFrameBuffer(VulkanRenderPass* renderPass, Size2 size);

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="renderPass"> Render pass </param>
				/// <param name="attachments"> "Precompiled" attachment list </param>
				VulkanFrameBuffer(VulkanRenderPass* renderPass, const std::vector<Reference<VulkanTextureView>>& attachments);

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
				const std::vector<Reference<VulkanTextureView>> m_attachments;
				
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
