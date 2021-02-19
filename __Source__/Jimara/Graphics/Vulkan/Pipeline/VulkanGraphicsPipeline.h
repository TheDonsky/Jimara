#pragma once
#include "VulkanPipeline.h"
#include "VulkanRenderPass.h"
#include "../../Pipeline/GraphicsPipeline.h"
#include "../Memory/Buffers/VulkanDynamicBuffer.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanGraphicsPipeline : public virtual VulkanPipeline, public virtual GraphicsPipeline {
			public:
				VulkanGraphicsPipeline(GraphicsPipeline::Descriptor* descriptor, VulkanRenderPass* renderPass, size_t maxInFlightCommandBuffers);

				virtual ~VulkanGraphicsPipeline();

				virtual void Execute(const CommandBufferInfo& bufferInfo) override;

			private:
				const Reference<GraphicsPipeline::Descriptor> m_descriptor;
				const Reference<VulkanRenderPass> m_renderPass;

				VkPipeline m_graphicsPipeline;
				Reference<VulkanStaticBuffer> m_indexBuffer;
			};
		}
	}
}
