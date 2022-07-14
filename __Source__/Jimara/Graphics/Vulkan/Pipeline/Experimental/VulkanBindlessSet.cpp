#ifndef VulkanBindlessSet_IMPLEMENTED
#define VulkanBindlessSet_IMPLEMENTED
#include "VulkanBindlessSet.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			// #################################################################################################################################
			// ####################################################### VulkanBindlessSet #######################################################
			// #################################################################################################################################

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
				for (size_t i = 0; i < MaxBoundObjects(); i++) {
					VulkanBindlessBinding<DataType>* binding = bindings + i;
					assert(binding->m_owner == nullptr);
					assert(binding->m_value == m_emptyBinding);
					assert(binding->RefCount() == 0);
					binding->~VulkanBindlessBinding();
				}
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
						m_descriptorDirty(index);
					}
					return binding;
				}
			}

			template<typename DataType>
			inline Reference<typename VulkanBindlessSet<DataType>::Instance> VulkanBindlessSet<DataType>::CreateInstance(size_t maxInFlightCommandBuffers) {
				return Object::Instantiate<VulkanBindlessInstance<DataType>>(this, maxInFlightCommandBuffers);
			}





			// #################################################################################################################################
			// ##################################################### VulkanBindlessBinding #####################################################
			// #################################################################################################################################

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
					owner->m_descriptorDirty(index);
				}
			}





			// #################################################################################################################################
			// #################################################### VulkanBindlessInstance #####################################################
			// #################################################################################################################################

			template<typename DataType>
			inline VulkanBindlessInstance<DataType>::VulkanBindlessInstance(VulkanBindlessSet<DataType>* owner, size_t maxInFlightCommandBuffers)
				: m_owner(owner) {
				m_bufferData.resize(maxInFlightCommandBuffers);
				for (size_t i = 0; i < maxInFlightCommandBuffers; i++) {
					CommandBufferData& data = m_bufferData[i];
					data.cachedBindings.resize(VulkanBindlessSet<DataType>::MaxBoundObjects());
					data.dirtyIndices.resize(VulkanBindlessSet<DataType>::MaxBoundObjects());
					for (size_t j = 0; j < VulkanBindlessSet<DataType>::MaxBoundObjects(); j++)
						data.dirtyIndices[j] = j;
				}
				Event<uint32_t>& onDirty = m_owner->m_descriptorDirty;
				onDirty += Callback(&VulkanBindlessInstance::IndexDirty, this);
			}

			template<typename DataType>
			inline VulkanBindlessInstance<DataType>::~VulkanBindlessInstance() {
				Event<uint32_t>& onDirty = m_owner->m_descriptorDirty;
				onDirty -= Callback(&VulkanBindlessInstance::IndexDirty, this);
			}

			template<typename DataType>
			inline void VulkanBindlessInstance<DataType>::IndexDirty(uint32_t index) {
				for (size_t i = 0; i < m_bufferData.size(); i++) {
					CommandBufferData& data = m_bufferData[i];
					if (data.cachedBindings[index].dirty) continue;
					data.cachedBindings[index].dirty = true;
					data.dirtyIndices.push_back(index);
				}
			}
		}
		}
	}
}
#endif
