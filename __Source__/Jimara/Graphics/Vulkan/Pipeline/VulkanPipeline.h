#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanPipeline;
		}
	}
}
#include "../VulkanDevice.h"
#include "../Pipeline/VulkanCommandPool.h"
#include "../Memory/Buffers/VulkanConstantBuffer.h"
#include "../Memory/Buffers/VulkanStaticBuffer.h"
#include "../Memory/TextureSamplers/VulkanTextureSampler.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// General vulkan pipeline (takes care of the bindings)
			/// </summary>
			class VulkanPipeline : public virtual Pipeline {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="descriptor"> Pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of command buffers that can be in flight, while still running this pipeline </param>
				VulkanPipeline(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPipeline();


			protected:
				/// <summary> Pipeline layout </summary>
				VkPipelineLayout PipelineLayout()const;

				/// <summary>
				/// Refreshes descriptor buffer references
				/// </summary>
				/// <param name="bufferInfo"> Buffer information </param>
				void UpdateDescriptors(const CommandBufferInfo& bufferInfo);

				/// <summary>
				/// Sets pipeline descriptors
				/// </summary>
				/// <param name="bufferInfo"> Buffer information </param>
				/// <param name="bindPoint"> Bind point </param>
				void SetDescriptors(const CommandBufferInfo& bufferInfor, VkPipelineBindPoint bindPoint);


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Input descriptor
				const Reference<PipelineDescriptor> m_descriptor;

				// Number of in-flight command buffers
				const size_t m_commandBufferCount;

				// Descriptor set layouts
				std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;

				// Pipeline layout
				VkPipelineLayout m_pipelineLayout;
				
				// Descriptor pool
				VkDescriptorPool m_descriptorPool;

				// Descriptor sets 
				// (indices 0 to {however many internally set (SetByEnvironment() == false) layouts there are} correspond to the sets for the first command buffer;
				// Same number of following sets are for second command buffer and so on)
				std::vector<VkDescriptorSet> m_descriptorSets;
				
				// Cached attachments from last UpdateDescriptors() call (cache misses result in descriptor set writes)
				struct {
					std::vector<Reference<VulkanPipelineConstantBuffer>> constantBuffers;
					std::vector<Reference<VulkanStaticBuffer>> structuredBuffers;
					std::vector<Reference<VulkanStaticImageSampler>> samplers;
				} m_descriptorCache;

				// Grouped descriptors to set togather
				struct DescriptorBindingRange {
					uint32_t start;
					std::vector<VkDescriptorSet> sets;

					inline DescriptorBindingRange() : start(0) {}
				};

				// Grouped descriptors to set togather
				std::vector<std::vector<DescriptorBindingRange>> m_bindingRanges;
			};
		}
	}
}
