#pragma once
#include "../../Pipeline/GraphicsPipeline.h"
#include "../Memory/VulkanDeviceResidentBuffer.h"
#include "../Rendering/VulkanRenderEngine.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanGraphicsPipeline : public virtual GraphicsPipeline {
			public:
				class RendererContext : public virtual Object {
				public:
					virtual VulkanDevice* Device() = 0;

					virtual VkRenderPass RenderPass() = 0;

					virtual VkSampleCountFlagBits TargetSampleCount() = 0;

					virtual Size2 TargetSize() = 0;

					virtual bool HasDepthAttachment() = 0;
				};

				VulkanGraphicsPipeline(RendererContext* context, GraphicsPipeline::Descriptor* descriptor);

				virtual ~VulkanGraphicsPipeline();

				void UpdateBindings(VulkanRenderEngine::CommandRecorder* recorder);

				void Render(VulkanRenderEngine::CommandRecorder* recorder);

			private:
				Reference<RendererContext> m_context;
				Reference<GraphicsPipeline::Descriptor> m_descriptor;

				std::vector<Reference<VulkanDeviceResidentBuffer>> m_vertexBuffers;
				Reference<VulkanDeviceResidentBuffer> m_indexBuffer;
				
				uint32_t m_indexCount;
				uint32_t m_instanceCount;

				VkPipelineLayout m_pipelineLayout;

				VkPipeline m_graphicsPipeline;

				std::vector<VkBuffer> m_vertexBindings;
				std::vector<VkDeviceSize> m_vertexBindingOffsets;

				VkBuffer m_indexDataBuffer;
			};
		}
	}
}
