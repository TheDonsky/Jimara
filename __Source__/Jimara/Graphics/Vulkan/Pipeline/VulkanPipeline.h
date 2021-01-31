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
				const Reference<VulkanDevice> m_device;
				const Reference<PipelineDescriptor> m_descriptor;
				const size_t m_commandBufferCount;

				std::vector<std::pair<VkDescriptorSetLayout, uint32_t>> m_descriptorSetLayouts;
				
				VkDescriptorPool m_descriptorPool;
				std::vector<VkDescriptorSet> m_descriptorSets;

				VkPipelineLayout m_pipelineLayout;
				
				struct DescriptorBindingRange {
					uint32_t start;
					std::vector<VkDescriptorSet> sets;

					inline DescriptorBindingRange() : start(0) {}
				};

				std::vector<std::vector<DescriptorBindingRange>> m_bindingRanges;

				struct {
					std::vector<Reference<VulkanStaticImageSampler>> samplers;
				} m_descriptorCache;
			};
		}
	}
}
