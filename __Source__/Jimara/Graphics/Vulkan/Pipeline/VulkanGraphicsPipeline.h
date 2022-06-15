#pragma once
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "../../Pipeline/GraphicsPipeline.h"
#include "../Memory/Buffers/VulkanDynamicBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed graphics pipeline
			/// </summary>
			class JIMARA_API VulkanGraphicsPipeline : public virtual VulkanPipeline, public virtual GraphicsPipeline {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="descriptor"> Pipeline descriptor </param>
				/// <param name="renderPass"> Render pass </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of command buffers, we allow the pipeline to be used by at the same time </param>
				VulkanGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, VulkanRenderPass* renderPass, size_t maxInFlightCommandBuffers);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanGraphicsPipeline();

				/// <summary>
				/// Executes graphics pipeline 
				/// Should run in-between RenderPass::BeginPass() and RenderPass::EndPass()
				/// </summary>
				/// <param name="bufferInfo"> Command buffer, alongside it's index </param>
				virtual void Execute(const CommandBufferInfo& bufferInfo) override;

			private:
				// Pipeline descriptor
				const Reference<GraphicsPipeline::Descriptor> m_descriptor;

				// Render pass
				const Reference<VulkanRenderPass> m_renderPass;

				// Vulkan API object
				VkPipeline m_graphicsPipeline;

				// Index buffer (can be internally instantiated as a substitude, so we keep a reference)
				Reference<VulkanStaticBuffer> m_indexBuffer;
			};
		}
	}
}
