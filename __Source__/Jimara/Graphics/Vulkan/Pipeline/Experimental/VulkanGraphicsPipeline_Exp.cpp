#include "VulkanGraphicsPipeline_Exp.h"



namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			Reference<VulkanGraphicsPipeline> VulkanGraphicsPipeline::Get(VulkanRenderPass* renderPass, Descriptor pipelineDescriptor) {
				if (renderPass == nullptr) return nullptr;
				// __TODO__: Implement this crap!
				renderPass->Device()->Log()->Error("VulkanGraphicsPipeline::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			VulkanGraphicsPipeline::VulkanGraphicsPipeline() {
				// __TODO__: Implement this crap!
			}

			VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
				// __TODO__: Implement this crap!
			}

			Reference<VertexInput> VulkanGraphicsPipeline::CreateVertexInput(
				const ResourceBinding<Graphics::ArrayBuffer>** vertexBuffers,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) {
				// __TODO__: Implement this crap!
				Device()->Log()->Error("VulkanGraphicsPipeline::CreateVertexInput - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			void VulkanGraphicsPipeline::Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) {
				// __TODO__: Implement this crap!
			}

			void VulkanGraphicsPipeline::DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) {
				// __TODO__: Implement this crap!
			}
		}
		}
	}
}
