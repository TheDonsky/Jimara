#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			template<typename DataType> class VulkanBindlessBinding;
			template<typename DataType> class VulkanBindlessInstance;
			template<typename DataType> class VulkanBindlessSet;
		}
		}
	}
}
#include "../../VulkanDevice.h"
#include "../../../Pipeline/Experimental/BindlessSet.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			template<typename DataType>
			class JIMARA_API VulkanBindlessBinding : public virtual Graphics::Experimental::BindlessSet<DataType>::Binding {
			protected:
				inline virtual void OnOutOfScope()const override;

				inline virtual DataType* BoundObject()const override { return m_value; }

			private:
				mutable Reference<VulkanBindlessSet<DataType>> m_owner;
				mutable Reference<DataType> m_value;

				friend class VulkanBindlessSet<DataType>;
				inline VulkanBindlessBinding(uint32_t index) : VulkanBindlessSet<DataType>::Binding(index) {}
			};

			template<typename DataType>
			class JIMARA_API VulkanBindlessInstance : public virtual Graphics::Experimental::BindlessSet<DataType>::Instance {
			public:
				inline VulkanBindlessInstance(VulkanBindlessSet<DataType>* owner, size_t maxInFlightCommandBuffers);

				inline virtual ~VulkanBindlessInstance();

				inline VkDescriptorSet GetDescriptorSet(size_t inFlightBufferId);

			private:
				const Reference<VulkanBindlessSet<DataType>> m_owner;
				VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
				VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
				struct CachedBinding {
					Reference<DataType> value;
					bool dirty = true;
				};
				struct CommandBufferData {
					std::vector<CachedBinding> cachedBindings;
					std::vector<uint32_t> dirtyIndices;
					std::atomic<bool> dirty = true;
					VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

					inline CommandBufferData() {}
					inline CommandBufferData(const CommandBufferData& other)
						: cachedBindings(other.cachedBindings), dirtyIndices(other.dirtyIndices), dirty(other.dirty.load()) {}
					inline CommandBufferData(CommandBufferData&& other)
						: cachedBindings(std::move(other.cachedBindings)), dirtyIndices(std::move(other.dirtyIndices)), dirty(other.dirty.load()) {}
				};
				std::vector<CommandBufferData> m_bufferData;

				inline void IndexDirty(uint32_t index);
				struct Helpers;
			};

			template<typename DataType>
			class JIMARA_API VulkanBindlessSet : public virtual Graphics::Experimental::BindlessSet<DataType> {
			public:
				inline VulkanBindlessSet(VulkanDevice* device);

				inline virtual ~VulkanBindlessSet();

				inline static const constexpr uint32_t MaxBoundObjects() {
					return sizeof(((VulkanBindlessSet*)nullptr)->m_bindings) / sizeof(VulkanBindlessBinding<DataType>);
				}

				typedef typename Graphics::Experimental::BindlessSet<DataType>::Binding Binding;

				typedef typename Graphics::Experimental::BindlessSet<DataType>::Instance Instance;

				inline virtual Reference<Binding> GetBinding(DataType* object) override;

				inline virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) override;


			private:
				const Reference<VulkanDevice> m_device;
				const Reference<DataType> m_emptyBinding;

				std::shared_mutex m_lock;
				char m_bindings[sizeof(VulkanBindlessBinding<DataType>) * (1 << 18)];
				std::vector<uint32_t> m_freeList;
				std::unordered_map<DataType*, uint32_t> m_index;

				EventInstance<uint32_t> m_descriptorDirty;

				inline VulkanBindlessBinding<DataType>* Bindings() { return reinterpret_cast<VulkanBindlessBinding<DataType>*>(m_bindings + 0); }

				friend class VulkanBindlessBinding<DataType>;
				friend class VulkanBindlessInstance<DataType>;
			};
		}
		}
	}
}
#include "VulkanBindlessSet.cpp"
