#pragma once
#include "VulkanPipeline_Exp.h"
#include "../VulkanShader.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			using ComputePipeline = Graphics::Experimental::ComputePipeline;

#pragma warning(disable: 4250)
			class JIMARA_API VulkanComputePipeline : public virtual ComputePipeline, public virtual VulkanPipeline {
			public:
				static Reference<VulkanComputePipeline> Get(VulkanDevice* device, const SPIRV_Binary* computeShader);

				virtual ~VulkanComputePipeline() = 0u;

				virtual void Dispatch(CommandBuffer* commandBuffer, const Size3& workGroupCount) override;

			private:
				const VkPipeline m_pipeline;
				const Reference<VulkanShader> m_shaderModule;

				VulkanComputePipeline(VkPipeline pipeline, VulkanShader* shaderModule);
			};
#pragma warning(default: 4250)
		}
		}
	}
}
