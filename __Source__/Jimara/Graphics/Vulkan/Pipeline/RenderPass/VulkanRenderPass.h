#pragma once
#include "../../Memory/Textures/VulkanImage.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Simple wrapper on top of a VkRenderPass object with a single subpass
			/// </summary>
			class JIMARA_API VulkanRenderPass : public virtual RenderPass {
			public:
				/// <summary>
				/// Gets cached instance or creates one
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="sampleCount"> "MSAA" </param>
				/// <param name="numColorAttachments"> Color attachment count </param>
				/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
				/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
				/// <param name="flags"> Clear and resolve flags </param>
				/// <returns> Cached instance of a render pass </returns>
				static Reference<VulkanRenderPass> Get(
					VulkanDevice* device, 
					Texture::Multisampling sampleCount, 
					size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats, 
					Texture::PixelFormat depthFormat, 
					RenderPass::Flags flags);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanRenderPass() = 0;

				/// <summary> Type cast to API object </summary>
				operator VkRenderPass()const;

				/// <summary> First color attachment index witin the framebuffer layout </summary>
				size_t FirstColorAttachmentId()const;

				/// <summary> Index of the depth attachment witin the framebuffer layout </summary>
				size_t DepthAttachmentId()const;

				/// <summary> Index of the first resolve attachment within the framebuffer layout </summary>
				size_t FirstResolveAttachmentId()const;

				/// <summary> Index of the depth attachment within the framebuffer layout </summary>
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
				/// Creates framebuffer with no attachments
				/// <para/> Note that this is only viable if the render pass itself has no attachments
				/// </summary>
				/// <param name="size"> Size (in pixels) of the frame buffer </param>
				/// <returns> 'Empty' frame buffer </returns>
				virtual Reference<FrameBuffer> CreateFrameBuffer(Size2 size) override;

				/// <summary>
				/// Creates or retrieves a cached instance of a graphics pipeline based on the shaders and vertex input configuration
				/// </summary>
				/// <param name="descriptor"> Graphics pipeline descriptor </param>
				/// <returns> Instance of a pipeline </returns>
				virtual Reference<Graphics::GraphicsPipeline> GetGraphicsPipeline(
					const Graphics::GraphicsPipeline::Descriptor& descriptor) override;

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
				const VkRenderPass m_renderPass;

				// Actual constructor is private
				VulkanRenderPass(VulkanDevice* device, VkRenderPass renderPass);

				// Some private stuff resides in here
				struct Helpers;
			};
		}
	}
}
