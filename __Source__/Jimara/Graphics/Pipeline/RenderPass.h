#pragma once
namespace Jimara { namespace Graphics { class RenderPass; } }
#include "FrameBuffer.h"
#include "CommandBuffer.h"
#include "GraphicsPipeline.h"


namespace Jimara { 
	namespace Graphics {
		/// <summary>
		/// Render pass
		/// Note: Mainly defines the shape of a frame buffer.
		/// </summary>
		class RenderPass : public virtual Object {
		public:
			/// <summary>
			/// Creates a frame buffer based on given attachments
			/// Note: Array sizes should be as defined by the render pass itself, so they are not received here
			/// </summary>
			/// <param name="colorAttachments"> Color attachments (can and should be multisampled if the render pass is set up for msaa) </param>
			/// <param name="depthAttachment"> Depth attachments (can and should be multisampled if the render pass is set up for msaa) </param>
			/// <param name="resolveAttachments"> Resolve attachmnets (should not be multisampled) </param>
			/// <returns> New instance of a frame buffer </returns>
			virtual Reference<FrameBuffer> CreateFrameBuffer(
				Reference<TextureView>* colorAttachments, Reference<TextureView> depthAttachment, Reference<TextureView>* resolveAttachments) = 0;

			/// <summary>
			/// Creates a graphics pipeline, compatible with this render pass
			/// </summary>
			/// <param name="descriptor"> Graphics pipeline descriptor </param>
			/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
			/// <returns> New instance of a graphics pipeline object </returns>
			virtual Reference<GraphicsPipeline> CreateGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) = 0;

			/// <summary>
			/// Begins render pass on the command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to begin pass on </param>
			/// <param name="frameBuffer"> Frame buffer for the render pass </param>
			/// <param name="clearValues"> Clear values for the color attachments </param>
			virtual void BeginPass(CommandBuffer* commandBuffer, FrameBuffer* frameBuffer, const Vector4* clearValues) = 0;

			/// <summary>
			/// Ends render pass on the command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to end the render pass on </param>
			virtual void EndPass(CommandBuffer* commandBuffer) = 0;
		};
	} 
}
