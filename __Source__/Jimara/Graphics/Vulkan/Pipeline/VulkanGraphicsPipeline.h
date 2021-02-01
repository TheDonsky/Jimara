#pragma once
#include "VulkanPipeline.h"
#include "../../Pipeline/GraphicsPipeline.h"
#include "../Memory/Buffers/VulkanDynamicBuffer.h"
#include "../Rendering/VulkanRenderEngine.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanGraphicsPipeline : public virtual VulkanPipeline {
			public:
				class RendererContext : public virtual Object {
				public:
					virtual VulkanDevice* Device() = 0;

					virtual VkRenderPass RenderPass() = 0;

					virtual VkSampleCountFlagBits TargetSampleCount() = 0;

					virtual Size2 TargetSize() = 0;

					virtual size_t TargetCount() = 0;

					virtual bool HasDepthAttachment() = 0;
				};

				VulkanGraphicsPipeline(RendererContext* context, GraphicsPipeline::Descriptor* descriptor);

				virtual ~VulkanGraphicsPipeline();

				void UpdateBindings(VulkanCommandRecorder* recorder);

				void Render(VulkanCommandRecorder* recorder);

			private:
				const Reference<RendererContext> m_context;
				const Reference<GraphicsPipeline::Descriptor> m_descriptor;

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
