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
			class JIMARA_API VulkanGraphicsPipeline : public virtual GraphicsPipeline, public virtual VulkanPipeline {
			public:
				static Reference<VulkanGraphicsPipeline> Get(VulkanRenderPass* renderPass, const Descriptor& pipelineDescriptor);

				virtual ~VulkanGraphicsPipeline() = 0u;

				virtual Reference<VertexInput> CreateVertexInput(
					const ResourceBinding<Graphics::ArrayBuffer>** vertexBuffers,
					const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) override;

				virtual void Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) override;

				virtual void DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) override;

			private:
				const VkPipeline m_pipeline;
				const size_t m_vertexBufferCount;
				const Reference<VulkanShader> m_vertexShader;
				const Reference<VulkanShader> m_fragmentShader;

				VulkanGraphicsPipeline(
					VkPipeline pipeline, size_t vertexBufferCount,
					VulkanShader* vertexShader, VulkanShader* fragmentShader);

				struct Helpers;
			};
#pragma warning(default: 4250)
		}
		}
	}
}
