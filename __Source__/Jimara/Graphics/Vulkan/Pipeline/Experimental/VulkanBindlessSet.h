#pragma once
#include "../../VulkanDevice.h"
#include "../../../Pipeline/Experimental/BindlessSet.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			template<typename DataType>
			class JIMARA_API VulkanBindlessSet : public virtual Graphics::Experimental::BindlessSet<DataType> {
			public:
				inline VulkanBindlessSet(VulkanDevice* device) 
					: m_device(device) {}

				inline ~VulkanBindlessSet() {}

				typedef typename Graphics::Experimental::BindlessSet<DataType>::Binding Binding;

				typedef typename Graphics::Experimental::BindlessSet<DataType>::Instance Instance;

				inline virtual Reference<Binding> GetBinding(DataType* object) override {

				}

				inline virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) override {

				}

			protected:


			private:
				const Reference<VulkanDevice> m_device;
				const Reference<TextureSampler> m_emptySampler;
			};
		}
		}
	}
}
