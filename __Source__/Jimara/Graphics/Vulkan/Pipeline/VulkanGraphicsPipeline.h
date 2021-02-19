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

				void UpdateBindings(VulkanCommandRecorder* recorder);

				void Render(VulkanCommandRecorder* recorder);

			private:
				const Reference<GraphicsPipeline::Descriptor> m_descriptor;
				const Reference<VulkanRenderPass> m_renderPass;

				VkPipeline m_graphicsPipeline;

				std::vector<Reference<VulkanStaticBuffer>> m_vertexBuffers;
				std::vector<VkBuffer> m_vertexBindings;
				std::vector<VkDeviceSize> m_vertexBindingOffsets;

				Reference<VulkanStaticBuffer> m_indexBuffer;
				uint32_t m_indexCount;
				uint32_t m_instanceCount;
			};
		}
	}
}
