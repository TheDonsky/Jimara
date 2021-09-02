#pragma once
#include "VulkanPipeline.h"
#include "../../Pipeline/ComputePipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Vulkan-backed compute pipeline
			/// </summary>
			class VulkanComputePipeline : public virtual VulkanPipeline, public virtual ComputePipeline {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="descriptor"> Pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of command buffers that can be in flight, while still running this pipeline </param>
				VulkanComputePipeline(VulkanDevice* device, ComputePipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanComputePipeline();

				/// <summary>
				/// Executes graphics pipeline 
				/// </summary>
				/// <param name="bufferInfo"> Command buffer, alongside it's index </param>
				virtual void Execute(const CommandBufferInfo& bufferInfo) override;

			private:
				// Pipeline descriptor
				const Reference<ComputePipeline::Descriptor> m_descriptor;

				// Vulkan API object
				VkPipeline m_computePipeline;
			};
		}
	}
}
