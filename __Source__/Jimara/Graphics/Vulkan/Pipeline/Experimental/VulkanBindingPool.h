#pragma once
#include "VulkanPipeline_Exp.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			using BindingPool = Graphics::Experimental::BindingPool;
			using BindingSet = Graphics::Experimental::BindingSet;
			using InFlightBufferInfo = Graphics::Experimental::InFlightBufferInfo;
			class VulkanBindingSet;

			class JIMARA_API VulkanBindingPool : public virtual BindingPool {
			public:
				VulkanBindingPool(VulkanDevice* device, size_t inFlightCommandBufferCount);

				virtual ~VulkanBindingPool();

				virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) override;

				virtual void UpdateAllBindingSets(size_t inFlightCommandBufferIndex) override;

			private:
				const Reference<VulkanDevice> m_device;
				const size_t m_inFlightCommandBufferCount;
				std::mutex m_bindingSetAllocationLock;
				const std::shared_ptr<SpinLock> m_descriptorWriteLock;
				Reference<Object> m_bindingBucket;

				struct Helpers;
				friend class VulkanBindingSet;
			};

			class JIMARA_API VulkanBindingSet : public virtual BindingSet {
			public:
				virtual ~VulkanBindingSet();

				virtual void Update(size_t inFlightCommandBufferIndex) override;

				virtual void Bind(InFlightBufferInfo inFlightBuffer) override;

			private:
				template<typename ResourceType>
				using Binding = Reference<const ResourceBinding<ResourceType>>;

				template<typename ResourceType>
				struct BindingInfo {
					Binding<ResourceType> binding;
					uint32_t bindingIndex = 0u;
				};

				template<typename ResourceType>
				using Bindings = Stacktor<BindingInfo<ResourceType>, 4u>;

				struct SetBindings {
					Bindings<Buffer> constantBuffers;
					Bindings<ArrayBuffer> structuredBuffers;
					Bindings<TextureSampler> textureSamplers;
					Bindings<TextureView> textureViews;
					Binding<BindlessSet<ArrayBuffer>::Instance> bindlessStructuredBuffers;
					Binding<BindlessSet<TextureSampler>::Instance> bindlessTextureSamplers;
				};

				struct DescriptorSet {
					Stacktor<Reference<Object>, 4u> boundObjects;
					VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
				};

				using DescriptorSets = Stacktor<DescriptorSet, 4u>;

				const Reference<const VulkanPipeline> m_pipeline;
				const Reference<VulkanBindingPool> m_bindingPool;
				const Reference<Object> m_bindingBucket;
				const SetBindings m_bindings;
				DescriptorSets m_descriptors;

				VulkanBindingSet(
					VulkanBindingPool* bindingPool, const VulkanPipeline* pipeline, Object* bindingBucket,
					SetBindings&& bindings, DescriptorSets&& descriptors);
				friend class VulkanBindingPool;
			};
		}
		}
	}
}
