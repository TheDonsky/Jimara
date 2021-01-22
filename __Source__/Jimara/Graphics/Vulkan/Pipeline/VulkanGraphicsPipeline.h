#pragma once
#include "../../Pipeline/GraphicsPipeline.h"
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

					virtual glm::uvec2 TargetSize() = 0;

					virtual bool HasDepthAttachment() = 0;
				};

				VulkanGraphicsPipeline(RendererContext* context, GraphicsPipeline::Descriptor* descriptor);

				virtual ~VulkanGraphicsPipeline();

				void Render(VulkanRenderEngine::CommandRecorder* recorder);

			private:
				Reference<RendererContext> m_context;
				Reference<GraphicsPipeline::Descriptor> m_descriptor;

				VkPipelineLayout m_pipelineLayout;

				VkPipeline m_graphicsPipeline;
			};
		}
	}
}
