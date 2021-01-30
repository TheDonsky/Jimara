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
#include "../Memory/VulkanDynamicTexture.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Command recorder.
			/// Note: When the graphics engine performs an update, 
			///		it requests all the underlying renderers to record their in-flight buffer specific commands 
			///		to a pre-constructed command buffer from the engine.
			/// </summary>
			class VulkanCommandRecorder {
			public:
				/// <summary> Virtual destructor </summary>
				inline virtual ~VulkanCommandRecorder() {}

				/// <summary> Target image index </summary>
				virtual size_t CommandBufferIndex()const = 0;

				/// <summary> Command buffer to record to </summary>
				virtual VkCommandBuffer CommandBuffer()const = 0;

				/// <summary> 
				/// If there are resources that should stay alive during command buffer execution, but might otherwise go out of scope,
				/// user can record those in a kind of a set that will delay their destruction till buffer execution ends using this call.
				/// </summary>
				virtual void RecordBufferDependency(Object* object) = 0;

				/// <summary> Waits for the given semaphore before executing the command buffer </summary>
				virtual void WaitForSemaphore(VkSemaphore semaphore) = 0;

				/// <summary> Signals given semaphore when the command buffer gets executed </summary>
				virtual void SignalSemaphore(VkSemaphore semaphore) = 0;

				/// <summary> Command pool for handling additional command buffer creation and what not </summary>
				virtual VulkanCommandPool* CommandPool()const = 0;
			};

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
			};
		}
	}
}
