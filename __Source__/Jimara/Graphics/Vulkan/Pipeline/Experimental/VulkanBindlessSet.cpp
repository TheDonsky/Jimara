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

#pragma warning(disable: 26812)
			template<typename DataType>
			struct VulkanBindlessInstance<DataType>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					static_assert(false);
					return VK_DESCRIPTOR_TYPE_MAX_ENUM;
				}
			};

			template<>
			struct VulkanBindlessInstance<Buffer>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}
			};

			template<>
			struct VulkanBindlessInstance<ArrayBuffer>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}
			};

			template<>
			struct VulkanBindlessInstance<TextureSampler>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
				}
			};

			template<typename DataType>
			inline VulkanBindlessInstance<DataType>::VulkanBindlessInstance(VulkanBindlessSet<DataType>* owner, size_t maxInFlightCommandBuffers)
				: m_owner(owner) {
				if (maxInFlightCommandBuffers <= 0) 
					maxInFlightCommandBuffers = 1;
				const constexpr uint32_t maxBoundObjects = VulkanBindlessSet<DataType>::MaxBoundObjects();

				// Create descriptor pool:
				{
					VkDescriptorPoolSize size = {};
					{
						size.descriptorCount = static_cast<uint32_t>(maxInFlightCommandBuffers) * maxBoundObjects;
						size.type = Helpers::DescriptorType();
					}
					VkDescriptorPoolCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
						createInfo.pNext = nullptr;
						createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
						createInfo.maxSets = static_cast<uint32_t>(maxInFlightCommandBuffers);
						createInfo.poolSizeCount = 1;
						createInfo.pPoolSizes = &size;
					}
					if (vkCreateDescriptorPool(*m_owner->m_device, &createInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
						m_owner->m_device->Log()->Fatal(
							"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::VulkanBindlessInstance - Failed to create descriptor pool! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Create descriptor set layout:
				{
					VkDescriptorSetLayoutBinding binding = {};
					{
						binding.binding = 0u;
						binding.descriptorType = Helpers::DescriptorType();
						binding.descriptorCount = maxBoundObjects;
						binding.stageFlags = VK_SHADER_STAGE_ALL;
						binding.pImmutableSamplers = nullptr;
					}

					VkDescriptorBindingFlags bindlessFlags = 
						VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | 
						VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | 
						VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
					
					VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo = {};
					{
						extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
						extendedInfo.pNext = nullptr;
						extendedInfo.bindingCount = 1;
						extendedInfo.pBindingFlags = &bindlessFlags;
					};

					VkDescriptorSetLayoutCreateInfo createInfo = {};
					{
						createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
						createInfo.pNext = &extendedInfo;
						createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
						createInfo.bindingCount = 1;
						createInfo.pBindings = &binding;
					}

					if (vkCreateDescriptorSetLayout(*m_owner->m_device, &createInfo, nullptr, &m_setLayout) != VK_SUCCESS) {
						vkDestroyDescriptorPool(*m_owner->m_device, m_descriptorPool, nullptr);
						m_owner->m_device->Log()->Fatal(
							"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::VulkanBindlessInstance - Failed to create descriptor set layout! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
				}

				// Allocate descriptor set:
				std::vector<VkDescriptorSet> descriptorSets(maxInFlightCommandBuffers); 
				{
					std::vector<VkDescriptorSetLayout> layouts(maxInFlightCommandBuffers);
					for (size_t i = 0; i < maxInFlightCommandBuffers; i++) layouts[i] = m_setLayout;

					VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo = {};
					{
						countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
						countInfo.pNext = nullptr;
						countInfo.descriptorSetCount = 1;
						countInfo.pDescriptorCounts = &maxBoundObjects;
					}

					VkDescriptorSetAllocateInfo allocateInfo = {};
					{
						allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
						allocateInfo.pNext = &countInfo;
						allocateInfo.descriptorPool = m_descriptorPool;
						allocateInfo.descriptorSetCount = static_cast<uint32_t>(maxInFlightCommandBuffers);
						allocateInfo.pSetLayouts = layouts.data();
					}

					if (vkAllocateDescriptorSets(*m_owner->m_device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
						for (size_t i = 0; i < descriptorSets.size(); i++) 
							descriptorSets[i] = VK_NULL_HANDLE;
						m_owner->m_device->Log()->Fatal(
							"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::VulkanBindlessInstance - Failed to allocate descriptor sets! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
				}

				// Create per in-flight buffer data:
				m_bufferData.resize(maxInFlightCommandBuffers);
				for (size_t i = 0; i < maxInFlightCommandBuffers; i++) {
					CommandBufferData& data = m_bufferData[i];
					data.cachedBindings.resize(maxBoundObjects);
					data.dirtyIndices.resize(maxBoundObjects);
					for (size_t j = 0; j < maxBoundObjects; j++)
						data.dirtyIndices[j] = static_cast<uint32_t>(j);
					data.descriptorSet = descriptorSets[i];
				}
				Event<uint32_t>& onDirty = m_owner->m_descriptorDirty;
				onDirty += Callback(&VulkanBindlessInstance::IndexDirty, this);
			}

			template<typename DataType>
			inline VulkanBindlessInstance<DataType>::~VulkanBindlessInstance() {
				Event<uint32_t>& onDirty = m_owner->m_descriptorDirty;
				onDirty -= Callback(&VulkanBindlessInstance::IndexDirty, this);

				if (m_descriptorPool != VK_NULL_HANDLE) {
					vkDestroyDescriptorPool(*m_owner->m_device, m_descriptorPool, nullptr);
					m_descriptorPool = VK_NULL_HANDLE;
				}

				if (m_setLayout != VK_NULL_HANDLE) {
					vkDestroyDescriptorSetLayout(*m_owner->m_device, m_setLayout, nullptr);
					m_setLayout = VK_NULL_HANDLE;
				}
			}

			template<typename DataType>
			inline VkDescriptorSet VulkanBindlessInstance<DataType>::GetDescriptorSet(size_t inFlightBufferId) {
				if (inFlightBufferId >= m_bufferData.size()) {
					m_owner->m_device->Log()->Error(
						"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::GetDescriptorSet - inFlightBufferId(", inFlightBufferId, " out of bounds!)! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return VK_NULL_HANDLE;
				}
				CommandBufferData& data = m_bufferData[inFlightBufferId];
				if (data.dirty) {
					std::shared_lock<std::shared_mutex> lock(m_owner->m_lock);
					// __TODO__: Update bindings...
				}
				return data.descriptorSet;
			}

			template<typename DataType>
			inline void VulkanBindlessInstance<DataType>::IndexDirty(uint32_t index) {
				for (size_t i = 0; i < m_bufferData.size(); i++) {
					CommandBufferData& data = m_bufferData[i];
					if (data.cachedBindings[index].dirty) continue;
					data.dirty = true;
					data.cachedBindings[index].dirty = true;
					data.dirtyIndices.push_back(index);
				}
			}
#pragma warning(default: 26812)
		}
		}
	}
}
#endif
