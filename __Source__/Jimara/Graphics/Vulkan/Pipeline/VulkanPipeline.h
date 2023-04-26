#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanPipeline;
		}
	}
}
#include "../VulkanDevice.h"
#include "VulkanCommandPool.h"
#include "VulkanBindlessSet.h"
#include "../Memory/Buffers/VulkanConstantBuffer.h"
#include "../Memory/Buffers/VulkanArrayBuffer.h"
#include "../Memory/Textures/VulkanTextureSampler.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// General vulkan pipeline (takes care of the bindings)
			/// </summary>
			class JIMARA_API VulkanPipeline : public virtual Graphics::Legacy::Pipeline {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="descriptor"> Pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of command buffers that can be in flight, while still running this pipeline </param>
				VulkanPipeline(VulkanDevice* device, Graphics::Legacy::PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPipeline();

				/// <summary> Owner device </summary>
				VulkanDevice* Device()const;


			protected:
				/// <summary> Pipeline layout </summary>
				VkPipelineLayout PipelineLayout()const;

				/// <summary> Input descriptor </summary>
				Graphics::Legacy::PipelineDescriptor* Descriptor()const;

				/// <summary>
				/// Updates and binds descriptor sets
				/// </summary>
				/// <param name="bufferInfo"> Buffer information </param>
				/// <param name="bindPoints"> Array of bind points </param>
				/// <param name="bindPointCount"> Number of entries in bind points </param>
				void BindDescriptors(const InFlightBufferInfo& bufferInfo, const VkPipelineBindPoint* bindPoints, size_t bindPointCount);

			private:
				/// <summary>
				/// Refreshes descriptor buffer references
				/// </summary>
				/// <param name="bufferInfo"> Buffer information </param>
				void UpdateDescriptors(const InFlightBufferInfo& bufferInfo);

				/// <summary>
				/// Sets pipeline descriptors
				/// </summary>
				/// <param name="bufferInfo"> Buffer information </param>
				/// <param name="bindPoint"> Bind point </param>
				void BindDescriptors(const InFlightBufferInfo& bufferInfor, VkPipelineBindPoint bindPoint);


			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Input descriptor
				const Reference<Graphics::Legacy::PipelineDescriptor> m_descriptor;

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
				std::mutex m_descriptorUpdateLock;
				
				// Cached attachments from last UpdateDescriptors() call (cache misses result in descriptor set writes)
				struct {
					std::vector<Reference<VulkanPipelineConstantBuffer>> constantBuffers;
					std::vector<Reference<VulkanPipelineConstantBuffer>> boundBuffers;
					std::vector<Reference<VulkanArrayBuffer>> structuredBuffers;
					std::vector<Reference<VulkanTextureSampler>> samplers;
					std::vector<Reference<VulkanTextureView>> views;
				} m_descriptorCache;

				// Cached information about bindless set bindings
				struct BindlessSetBinding {
					uint32_t setId = 0;
					VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
					std::mutex* descriptorSetLock = nullptr;
					Reference<Object> bindlessInstance;
				};
				std::vector<BindlessSetBinding> m_bindlessCache;

				// Grouped descriptors to set togather
				struct DescriptorBindingRange {
					uint32_t start;
					std::vector<VkDescriptorSet> sets;

					inline DescriptorBindingRange() : start(0) {}
				};

				// Grouped descriptors to set togather
				std::vector<std::vector<DescriptorBindingRange>> m_bindingRanges;
			};


			/// <summary>
			/// Vulkan pipeline for setting "environment-provided" bindings
			/// </summary>
			class JIMARA_API VulkanEnvironmentPipeline : public VulkanPipeline {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="descriptor"> Pipeline descriptor </param>
				/// <param name="maxInFlightCommandBuffers"> Maximal number of command buffers that can be in flight, while still running this pipeline </param>
				/// <param name="bindPointCount"> Pipeline bind point count </param>
				/// <param name="bindPoints"> Pipeline bind points </param>
				VulkanEnvironmentPipeline(
					VulkanDevice* device, Graphics::Legacy::PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers, size_t bindPointCount, const VkPipelineBindPoint* bindPoints);

				/// <summary>
				/// Sets the environment
				/// </summary>
				/// <param name="bufferInfo"> Command buffer information </param>
				virtual void Execute(const InFlightBufferInfo& bufferInfo) override;

			private:
				// Pipeline bind points
				const std::vector<VkPipelineBindPoint> m_bindPoints;
			};
		}
	}
}
