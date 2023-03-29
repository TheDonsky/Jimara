#pragma once
#include "VulkanPipeline_Exp.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			using BindingPool = Graphics::Experimental::BindingPool;
			using BindingSet = Graphics::Experimental::BindingSet;
			class VulkanBindingSet;

			class JIMARA_API VulkanBindingPool : public virtual BindingPool {
			public:
				VulkanBindingPool(VulkanDevice* device);

				virtual ~VulkanBindingPool();

				virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) override;

				virtual void UpdateAllBindingSets(size_t inFlightCommandBufferIndex) override;

			private:
				const Reference<VulkanDevice> m_device;

				struct Helpers;
				friend class VulkanBindingSet;
			};

			class JIMARA_API VulkanBindingSet : public virtual BindingSet {
			public:
				virtual ~VulkanBindingSet();

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
				const SetBindings m_bindings;

				VulkanBindingSet(SetBindings&& bindings);
				friend class VulkanBindingPool;
			};
		}
		}
	}
}
