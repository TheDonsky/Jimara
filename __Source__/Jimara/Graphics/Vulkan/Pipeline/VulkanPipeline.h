#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanCommandRecorder;
			class VulkanPipeline;
		}
	}
}
#include "../VulkanDevice.h"
#include "../Pipeline/VulkanCommandPool.h"
#include "../Memory/Buffers/VulkanConstantBuffer.h"
#include "../Memory/TextureSamplers/VulkanTextureSampler.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanPipeline : public virtual Pipeline {
			public:
				VulkanPipeline(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers);

				virtual ~VulkanPipeline();

			protected:
				VkPipelineLayout PipelineLayout()const;

				void UpdateDescriptors(VulkanCommandRecorder* recorder);

				void SetDescriptors(VulkanCommandRecorder* recorder, VkPipelineBindPoint bindPoint);

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
					std::vector<Reference<VulkanStaticImageSampler>> samplers;
				} m_descriptorCache;

				struct DescriptorBindingRange {
					uint32_t start;
					std::vector<VkDescriptorSet> sets;

					inline DescriptorBindingRange() : start(0) {}
				};

				std::vector<std::vector<DescriptorBindingRange>> m_bindingRanges;

			};
		}
	}
}
