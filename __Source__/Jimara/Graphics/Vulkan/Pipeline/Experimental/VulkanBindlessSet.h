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

			private:
				friend class VulkanBindlessSet<DataType>;
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

				inline virtual Reference<Instance> CreateInstance(size_t maxInFlightCommandBuffers) override {
					return nullptr;
				}


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
				class Helpers;
			};





			template<typename DataType>
			inline VulkanBindlessSet<DataType>::VulkanBindlessSet(VulkanDevice* device)
				: m_device(device) {
				VulkanBindlessBinding<DataType>* bindings = Bindings();
				for (size_t i = 0; i < MaxBoundObjects(); i++) {
					VulkanBindlessBinding<DataType>* binding = bindings + i;
					new(binding)VulkanBindlessBinding<DataType>(i);
					binding->ReleaseRef();
					binding->m_value = m_emptyBinding;
				}
				for (size_t i = 0; i < MaxBoundObjects(); i++)
					m_freeList.push_back(MaxBoundObjects() - 1 - i);
			}

			template<typename DataType>
			inline VulkanBindlessSet<DataType>::~VulkanBindlessSet() {
				if (m_freeList.size() != MaxBoundObjects())
					m_device->Log()->Error(
						"VulkanBindlessSet<", TypeId::Of<DataType>().Name(), ">::~VulkanBindlessSet - FreeList incomplete on destruction! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				VulkanBindlessBinding<DataType>* bindings = Bindings();
				for (size_t i = 0; i < MaxBoundObjects(); i++)
					(bindings + i)->~VulkanBindlessBinding();
			}

			template<typename DataType>
			inline Reference<typename VulkanBindlessSet<DataType>::Binding> VulkanBindlessSet<DataType>::GetBinding(DataType* object) {
				if (object == nullptr) {
					m_device->Log()->Warning(
						"VulkanBindlessSet<", TypeId::Of<DataType>().Name(), ">::GetBinding - nullptr object provided!",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				auto findBinding = [&]() -> Reference<Binding> {
					auto it = m_index.find(object);
					if (it == m_index.end()) return nullptr;
					else return Bindings() + it->second;
				};
				{
					std::shared_lock<std::shared_mutex> lock(m_lock);
					Reference<Binding> binding = findBinding();
					if (binding != nullptr) return binding;
				}
				{
					std::unique_lock<std::shared_mutex> lock(m_lock);
					Reference<Binding> binding = findBinding();
					if (binding != nullptr) return binding;
					else if (m_freeList.empty()) {
						m_device->Log()->Error(
							"VulkanBindlessSet<", TypeId::Of<DataType>().Name(), ">::GetBinding - Binding limit of ", MaxBoundObjects(), " reached!",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}
					else {
						const uint32_t index = m_freeList.back();
						m_freeList.pop_back();
						VulkanBindlessBinding<DataType>* result = Bindings() + index;
						assert(result->Index() == index);
						result->m_owner = this;
						result->m_value = object;
						m_index[object] = index;
						binding = result;
					}
					return binding;
				}
			}

			template<typename DataType>
			inline void VulkanBindlessBinding<DataType>::OnOutOfScope()const {
				if (m_owner == nullptr) return;
				const uint32_t index = Graphics::Experimental::BindlessSet<DataType>::Binding::Index();
				Reference<VulkanBindlessSet<DataType>> owner = m_owner;
				{
					std::unique_lock<std::shared_mutex> lock(owner->m_lock);
					if (Object::RefCount() > 0) return;
					owner->m_index.erase(m_value);
					owner->m_freeList.push_back(index);
					m_value = owner->m_emptyBinding;
					m_owner = nullptr;
				}
				owner->m_descriptorDirty(index);
			}
		}
		}
	}
}
