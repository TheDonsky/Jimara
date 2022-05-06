#pragma once
#include "../Memory/Textures/VulkanTexture.h"
#include "../../Pipeline/RenderPass.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Simple wrapper on top of a VkRenderPass object with a single subpass
			/// </summary>
			class VulkanRenderPass : public virtual RenderPass {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="sampleCount"> "MSAA" </param>
				/// <param name="numColorAttachments"> Color attachment count </param>
				/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
				/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
				/// <param name="flags"> Clear and resolve flags </param>
				VulkanRenderPass(
					VulkanDevice* device, 
					Texture::Multisampling sampleCount, 
					size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats, 
					Texture::PixelFormat depthFormat, 
					RenderPass::Flags flags);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanRenderPass();

				/// <summary> Type cast to API object </summary>
				operator VkRenderPass()const;

				/// <summary> First color attachment index witin the framebuffer layout </summary>
				size_t FirstColorAttachmentId()const;

				/// <summary> Index of the depth attachment witin the framebuffer layout </summary>
				size_t DepthAttachmentId()const;

				/// <summary> Index of the first resolve attachment within the framebuffer layout </summary>
				size_t FirstResolveAttachmentId()const;

				size_t DepthResolveAttachmentId()const;

				/// <summary> "Owner" device </summary>
				virtual GraphicsDevice* Device()const override;

				/// <summary>
				/// Creates a frame buffer based on given attachments
				/// Note: Array sizes should be as defined by the render pass itself, so they are not received here
				/// </summary>
				/// <param name="colorAttachments"> Color attachments (can and should be multisampled if the render pass is set up for msaa) </param>
				/// <param name="depthAttachment"> Depth attachments (can and should be multisampled if the render pass is set up for msaa) </param>
				/// <param name="colorResolveAttachments"> 
				/// Resolve attachments for colorAttachments (should not be multisampled; ignored, if the render pass is not multisampled or does not have RESOLVE_COLOR flag set) 
				/// </param>
				/// <param name="depthResolveAttachment"> 
				/// Resolve attachments for depthAttachment (should not be multisampled; ignored, if the render pass is not multisampled or does not have RESOLVE_DEPTH flag set) 
				/// </param>
				/// <returns> New instance of a frame buffer </returns>
				virtual Reference<FrameBuffer> CreateFrameBuffer(
					Reference<TextureView>* colorAttachments, Reference<TextureView> depthAttachment,
					Reference<TextureView>* colorResolveAttachments, Reference<TextureView> depthResolveAttachment) override;

				/// <summary>
				/// Creates a graphics pipeline, compatible with this render pass
				/// </summary>
				/// <param name="descriptor"> Graphics pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
				/// <returns> New instance of a graphics pipeline object </returns>
				virtual Reference<GraphicsPipeline> CreateGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) override;

				/// <summary>
				/// Begins render pass on the command buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to begin pass on </param>
				/// <param name="frameBuffer"> Frame buffer for the render pass </param>
				/// <param name="clearValues"> Clear values for the color attachments (Ignored, if the pass was created with 'false' for 'clearColor' argument) </param>
				/// <param name="renderWithSecondaryCommandBuffers"> If true, the render pass contents should be recorded using secondary command buffers </param>
				virtual void BeginPass(CommandBuffer* commandBuffer, FrameBuffer* frameBuffer, const Vector4* clearValues, bool renderWithSecondaryCommandBuffers) override;

				/// <summary>
				/// Ends render pass on the command buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to end the render pass on </param>
				virtual void EndPass(CommandBuffer* commandBuffer) override;


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Underlying API object
				VkRenderPass m_renderPass;
			};
		}
	}
}
