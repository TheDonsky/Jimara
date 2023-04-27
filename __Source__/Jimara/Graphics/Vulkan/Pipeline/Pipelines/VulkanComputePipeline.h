#pragma once
#include "VulkanPipeline.h"
#include "VulkanShader.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
#pragma warning(disable: 4250)
			/// <summary>
			/// ComputePipeline implementation for Vulkan API
			/// </summary>
			class JIMARA_API VulkanComputePipeline : public virtual ComputePipeline, public virtual VulkanPipeline {
			public:
				/// <summary>
				/// Gets cached instance or creates a new compute pipeline
				/// </summary>
				/// <param name="device"> Graphics device </param>
				/// <param name="computeShader"> Compute shader binary </param>
				/// <returns> Shared compute pipeline instance </returns>
				static Reference<VulkanComputePipeline> Get(VulkanDevice* device, const SPIRV_Binary* computeShader);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanComputePipeline() = 0;

				/// <summary>
				/// Runs compute kernel through a command buffer
				/// </summary>
				/// <param name="commandBuffer"> Command buffer to record dispatches to </param>
				/// <param name="workGroupCount"> Number of thead blocks to execute </param>
				virtual void Dispatch(CommandBuffer* commandBuffer, const Size3& workGroupCount) override;

			private:
				// Underlying Vulkan pipeline
				const VkPipeline m_pipeline;

				// Compute shader module
				const Reference<VulkanShader> m_shaderModule;

				// Constructor, is indeed, private
				VulkanComputePipeline(VkPipeline pipeline, VulkanShader* shaderModule);
			};
#pragma warning(default: 4250)
		}
	}
}
