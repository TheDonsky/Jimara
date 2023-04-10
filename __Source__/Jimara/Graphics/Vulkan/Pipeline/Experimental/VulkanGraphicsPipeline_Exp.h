#pragma once
#include "VulkanPipeline_Exp.h"
#include "../VulkanShader.h"
#include "../VulkanRenderPass.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			using GraphicsPipeline = Graphics::Experimental::GraphicsPipeline;
			using VertexInput = Graphics::Experimental::VertexInput;

#pragma warning(disable: 4250)
			/// <summary>
			/// Graphics Pipeline implementation for Vulkan API
			/// </summary>
			class JIMARA_API VulkanGraphicsPipeline : public virtual GraphicsPipeline, public virtual VulkanPipeline {
			public:
				/// <summary>
				/// Gets cached instance or creates a new graphics pipeline
				/// </summary>
				/// <param name="renderPass"> Render pass </param>
				/// <param name="pipelineDescriptor"> Graphics pipeline descriptor </param>
				/// <returns> Shared VulkanGraphicsPipeline instance </returns>
				static Reference<VulkanGraphicsPipeline> Get(VulkanRenderPass* renderPass, const Descriptor& pipelineDescriptor);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanGraphicsPipeline() = 0u;

				/// <summary>
				/// Creates compatible vertex input
				/// </summary>
				/// <param name="vertexBuffers"> Vertex buffer bindings (array size should be the same as the vertexInput list within the descriptor) </param>
				/// <param name="indexBuffer"> 
				///		Index buffer binding 
				///		(bound objects have to be array buffers of uint32_t or uint16_t types; nullptr means indices from 0 to vertexId) 
				/// </param>
				/// <returns> New instance of a vertex input </returns>
				virtual Reference<VertexInput> CreateVertexInput(
					const ResourceBinding<Graphics::ArrayBuffer>* const* vertexBuffers,
					const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) override;

				/// <summary>
				/// Draws bound geometry using the graphics pipeline
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to draw on (has to be invoked within a render pass) </param>
				/// <param name="indexCount"> Number of indices to use from the bound VertexInput's index buffer </param>
				/// <param name="instanceCount"> Number of instances to draw </param>
				virtual void Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) override;

				/// <summary>
				/// Draws bound geometry using an indirect draw buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to draw on (has to be invoked within a render pass) </param>
				/// <param name="indirectBuffer"> Draw calls (can be uploaded to a buffer from CPU or built on GPU) </param>
				/// <param name="drawCount"> Number of draw calls (if one wishes to go below indirectBuffer->ObjectCount() feel free to input any number) </param>
				virtual void DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) override;

			private:
				// Vulkan pipeline
				const VkPipeline m_pipeline;

				// Number of consumed vertex buffers
				const size_t m_vertexBufferCount;

				// Vertex shader module
				const Reference<VulkanShader> m_vertexShader;

				// Fragment shader module
				const Reference<VulkanShader> m_fragmentShader;

				// Constructor is private
				VulkanGraphicsPipeline(
					VkPipeline pipeline, size_t vertexBufferCount,
					VulkanShader* vertexShader, VulkanShader* fragmentShader);

				// Private stuff
				struct Helpers;
			};
#pragma warning(default: 4250)

			/// <summary>
			/// VertexInput inplementation for Vulkan API
			/// </summary>
			class JIMARA_API VulkanVertexInput : public virtual VertexInput {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="vertexBuffers"> Vertex buffer list </param>
				/// <param name="vertexBufferCount"> Number of vertex buffers </param>
				/// <param name="indexBuffer"> Index buffer binding </param>
				VulkanVertexInput(
					VulkanDevice* device,
					const ResourceBinding<Graphics::ArrayBuffer>* const* vertexBuffers, size_t vertexBufferCount,
					const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanVertexInput();

				/// <summary>
				/// Binds vertex buffers to a command buffer
				/// <para/> Note: This should be executed before a corresponding draw call.
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record bind command to </param>
				virtual void Bind(CommandBuffer* commandBuffer) override;

			private:
				// Graphics device
				const Reference<VulkanDevice> m_device;

				// Vertex buffer bindings
				using VertexBindings = Stacktor<Reference<const ResourceBinding<Graphics::ArrayBuffer>>, 4u>;
				const VertexBindings m_vertexBuffers;

				// Index  buffer bindings
				const Reference<const ResourceBinding<Graphics::ArrayBuffer>> m_indexBuffer;

				// If index buffer is missing, we have a special shared instance ready
				Reference<Object> m_sharedIndexBufferHolder;

				// Private stuff
				struct Helpers;
			};
		}
		}
	}
}
