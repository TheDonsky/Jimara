#ifndef VulkanBindlessSet_IMPLEMENTED
#define VulkanBindlessSet_IMPLEMENTED
#include "VulkanBindlessSet.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			// #################################################################################################################################
			// ####################################################### VulkanBindlessSet #######################################################
			// #################################################################################################################################

			template<typename DataType>
			inline VulkanBindlessSet<DataType>::VulkanBindlessSet(VulkanDevice* device)
				: m_device(device), m_emptyBinding(VulkanBindlessInstance<DataType>::Helpers::CreateEmptyBinding(device)) {
				VulkanBindlessBinding<DataType>* bindings = Bindings();
				for (uint32_t i = 0; i < MaxBoundObjects(); i++) {
					VulkanBindlessBinding<DataType>* binding = bindings + i;
					new(binding)VulkanBindlessBinding<DataType>(i);
					binding->ReleaseRef();
					binding->m_value = nullptr;
				}
				for (uint32_t i = 0; i < MaxBoundObjects(); i++)
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
					assert(binding->m_value == nullptr);
					assert(binding->RefCount() == 0);
					binding->~VulkanBindlessBinding();
				}
			}

			template<typename DataType>
			inline Reference<typename BindlessSet<DataType>::Binding> VulkanBindlessSet<DataType>::GetBinding(DataType* object) {
				if (object == nullptr) {
					m_device->Log()->Warning(
						"VulkanBindlessSet<", TypeId::Of<DataType>().Name(), ">::GetBinding - nullptr object provided!",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				auto findBinding = [&]() -> Reference<typename BindlessSet<DataType>::Binding> {
					auto it = m_index.find(object);
					if (it == m_index.end()) return nullptr;
					else return Bindings() + it->second;
				};
				{
					std::shared_lock<std::shared_mutex> lock(m_lock);
					Reference<typename BindlessSet<DataType>::Binding> binding = findBinding();
					if (binding != nullptr) return binding;
				}
				{
					std::unique_lock<std::shared_mutex> lock(m_lock);
					Reference<typename BindlessSet<DataType>::Binding> binding = findBinding();
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
			inline Reference<typename BindlessSet<DataType>::Instance> VulkanBindlessSet<DataType>::CreateInstance(size_t maxInFlightCommandBuffers) {
				return Object::Instantiate<VulkanBindlessInstance<DataType>>(this, maxInFlightCommandBuffers);
			}





			// #################################################################################################################################
			// ##################################################### VulkanBindlessBinding #####################################################
			// #################################################################################################################################

			template<typename DataType>
			inline void VulkanBindlessBinding<DataType>::OnOutOfScope()const {
				if (m_owner == nullptr) return;
				const uint32_t index = BindlessSet<DataType>::Binding::Index();
				Reference<VulkanBindlessSet<DataType>> owner = m_owner;
				{
					std::unique_lock<std::shared_mutex> lock(owner->m_lock);
					if (Object::RefCount() > 0) return;
					owner->m_index.erase(m_value);
					owner->m_freeList.push_back(index);
					m_value = nullptr;
					m_owner = nullptr;
					owner->m_descriptorDirty(index);
				}
			}





			// #################################################################################################################################
			// #################################################### VulkanBindlessInstance #####################################################
			// #################################################################################################################################

#pragma warning(disable: 26812)
			template<typename DataType>
			struct VulkanBindlessInstance<DataType>::Helpers { };

			template<>
			struct VulkanBindlessInstance<Buffer>::Helpers {
				class AllocationGroup;
				class SubAllocation;

				class Allocator : public virtual Object {
				public:
					inline Allocator(VulkanDevice* device) : m_device(device) {}

					inline static const constexpr uint32_t AllocationBatchSize() { return 1024u; }
					inline static const constexpr size_t MinBufferSize() { return 16u; }

					SubAllocation* Allocate(size_t bufferSize) {
						size_t index = 0;
						size_t allocationSize = MinBufferSize();
						while (allocationSize < bufferSize) {
							allocationSize <<= 1;
							index++;
						}
						
						while (m_sizeEntries.size() <= index)
							m_sizeEntries.push_back(Object::Instantiate<SizeGroup>());
						SizeGroup* group = m_sizeEntries[index];

						if (group->freeList.empty()) {
							Reference<AllocationGroup> allocations = Object::Instantiate<AllocationGroup>(m_device, bufferSize, AllocationBatchSize());
							for (size_t i = 0; i < allocations->SubAllocationCount(); i++)
								group->freeList.push_back(allocations->SubAllocations() + i);
							m_allocatedGroups.push_back(allocations);
						}

						SubAllocation* rv = group->freeList.back();
						group->freeList.pop_back();
						return rv;
					}

					void Free(SubAllocation* subAllocation) {
						size_t index = 0;
						size_t allocationSize = MinBufferSize();
						size_t bufferSize = subAllocation->Buffer()->ObjectSize();
						while (allocationSize < bufferSize) {
							allocationSize <<= 1;
							index++;
						}
						if (index >= m_sizeEntries.size()) {
							m_device->Log()->Fatal(
								"VulkanBindlessInstance<Buffer>::Helpers::Allocator::Free - Size bucket does not exist! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
						m_sizeEntries[index]->freeList.push_back(subAllocation);
					}

				private:
					const Reference<VulkanDevice> m_device;
					std::vector<Reference<Object>> m_allocatedGroups;
					struct SizeGroup : public virtual Object {
						std::vector<SubAllocation*> freeList;
					};
					std::vector<Reference<SizeGroup>> m_sizeEntries;
				};

				class SubAllocation {
				public:
					inline VulkanArrayBuffer* Buffer()const { return m_buffer; }

					inline uint32_t BufferOffset()const { return m_bufferOffset; }

				private:
					Reference<VulkanArrayBuffer> m_buffer;
					uint32_t m_bufferOffset = 0;
					friend class AllocationGroup;
				};

				class AllocationGroup : public virtual Object {
				public:
					inline AllocationGroup(VulkanDevice* device, size_t bufferSize, size_t bufferCount) {
						Reference<VulkanArrayBuffer> buffer = device->CreateArrayBuffer(bufferSize, bufferCount, ArrayBuffer::CPUAccess::CPU_READ_WRITE);
						m_subAllocations.resize(bufferSize);
						for (size_t i = 0; i < bufferCount; i++) {
							SubAllocation* allocation = m_subAllocations.data() + i;
							allocation->m_buffer = buffer;
							allocation->m_bufferOffset = static_cast<uint32_t>(i * bufferSize);
						}
					}

					inline SubAllocation* SubAllocations() { return m_subAllocations.data(); }

					inline size_t SubAllocationCount()const { return m_subAllocations.size(); }

				private:
					std::vector<SubAllocation> m_subAllocations;
				};

				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}

				typedef VkDescriptorBufferInfo VulkanResourceWriteInfo;
			};

			template<>
			struct VulkanBindlessInstance<Buffer>::CachedBinding {
				Reference<VulkanConstantBuffer> value;
				VulkanBindlessInstance<Buffer>::Helpers::SubAllocation* subAllocation = nullptr;
				bool dirty = false;
			};

			template<>
			struct VulkanBindlessInstance<ArrayBuffer>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}

				inline static Reference<ArrayBuffer> CreateEmptyBinding(GraphicsDevice* device) {
					return device->CreateArrayBuffer<uint32_t>(1u);
				}

				typedef VkDescriptorBufferInfo VulkanResourceWriteInfo;

				inline static void FillWriteInfo(ArrayBuffer* object, VulkanResourceWriteInfo* info, VkWriteDescriptorSet* write) {
					(*info) = {};
					info->buffer = *dynamic_cast<VulkanArrayBuffer*>(object);
					info->offset = 0;
					info->range = VK_WHOLE_SIZE;
					write->pBufferInfo = info;
				}
			};

			template<>
			struct VulkanBindlessInstance<TextureSampler>::Helpers {
				inline static constexpr VkDescriptorType DescriptorType() {
					return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				}

				inline static Reference<TextureSampler> CreateEmptyBinding(GraphicsDevice* device) {
					return device->CreateMultisampledTexture(
						Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::B8G8R8A8_SRGB,
						Size3(1), 1, Texture::Multisampling::SAMPLE_COUNT_1)->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();
				}

				typedef VkDescriptorImageInfo VulkanResourceWriteInfo;

				inline static void FillWriteInfo(TextureSampler* object, VulkanResourceWriteInfo* info, VkWriteDescriptorSet* write) {
					(*info) = {};
					info->sampler = *dynamic_cast<VulkanTextureSampler*>(object);
					info->imageView = *dynamic_cast<VulkanTextureView*>(object->TargetView());
					info->imageLayout = dynamic_cast<VulkanImage*>(object->TargetView()->TargetTexture())->ShaderAccessLayout();
					write->pImageInfo = info;
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
					m_setLayout = CreateDescriptorSetLayout(owner->m_device);
					if (m_setLayout == VK_NULL_HANDLE) {
						vkDestroyDescriptorPool(*owner->m_device, m_descriptorPool, nullptr);
						m_owner->m_device->Log()->Fatal(
							"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::VulkanBindlessInstance - Failed to create descriptor set layout! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
				}

				// Allocate descriptor set:
				std::vector<VkDescriptorSet> descriptorSets(maxInFlightCommandBuffers);
				{
					std::vector<VkDescriptorSetLayout> layouts(maxInFlightCommandBuffers);
					std::vector<uint32_t> counts(maxInFlightCommandBuffers);
					for (size_t i = 0; i < maxInFlightCommandBuffers; i++) {
						layouts[i] = m_setLayout;
						counts[i] = maxBoundObjects;
					}

					VkDescriptorSetVariableDescriptorCountAllocateInfoEXT countInfo = {};
					{
						countInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
						countInfo.pNext = nullptr;
						countInfo.descriptorSetCount = static_cast<uint32_t>(maxInFlightCommandBuffers);
						countInfo.pDescriptorCounts = counts.data();
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
				{
					m_bufferData.resize(maxInFlightCommandBuffers);
					for (size_t i = 0; i < maxInFlightCommandBuffers; i++) {
						CommandBufferData& data = m_bufferData[i];
						data.cachedBindings.resize(maxBoundObjects);
						data.descriptorSet = descriptorSets[i];
					}
				}

				// Subscribe to OnDirty event:
				{
					Event<uint32_t>& onDirty = m_owner->m_descriptorDirty;
					onDirty += Callback(&VulkanBindlessInstance::IndexDirty, this);
				}

				// Mark filled entries as dirty:
				{
					std::shared_lock<std::shared_mutex> ownerLock(m_owner->m_lock);
					for (typename decltype(m_owner->m_index)::const_iterator it = m_owner->m_index.begin(); it != m_owner->m_index.end(); ++it) {
						const uint32_t index = it->second;
						for (size_t i = 1; i < maxInFlightCommandBuffers; i++) {
							CommandBufferData& data = m_bufferData[i];
							data.cachedBindings[index].dirty = true;
							data.dirtyIndices.push_back(index);
							data.dirty = true;
						}
					}
				}
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
			inline VkDescriptorSetLayout VulkanBindlessInstance<DataType>::CreateDescriptorSetLayout(VulkanDevice* device) {
				VkDescriptorSetLayoutBinding binding = {};
				{
					binding.binding = 0u;
					binding.descriptorType = Helpers::DescriptorType();
					binding.descriptorCount = VulkanBindlessSet<DataType>::MaxBoundObjects();
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

				VkDescriptorSetLayout layout;
				if (vkCreateDescriptorSetLayout(*device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
					device->Log()->Fatal(
						"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::CreateDescriptorSetLayout - Failed to create descriptor set layout! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return VK_NULL_HANDLE;
				}
				else return layout;
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
					std::shared_lock<std::shared_mutex> ownerLock(m_owner->m_lock);
					std::unique_lock<std::mutex> updateLock(data.updateLock);

					static thread_local std::vector<VkWriteDescriptorSet> writes;
					static thread_local std::vector<typename Helpers::VulkanResourceWriteInfo> infos;
					writes.resize(data.dirtyIndices.size());
					infos.resize(data.dirtyIndices.size());

					for (size_t i = 0; i < data.dirtyIndices.size(); i++) {
						const uint32_t index = data.dirtyIndices[i];
						CachedBinding& cachedBinding = data.cachedBindings[index];
						const VulkanBindlessBinding<DataType>& binding = m_owner->Bindings()[index];
						DataType* boundObject = binding.m_value;
						cachedBinding.value = boundObject;
						if (boundObject == nullptr)
							boundObject = m_owner->m_emptyBinding;
						cachedBinding.dirty = false;
						VkWriteDescriptorSet& write = writes[i];
						{
							write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.pNext = nullptr;
							write.dstSet = data.descriptorSet;
							write.dstBinding = 0;
							write.dstArrayElement = index;
							write.descriptorCount = 1;
							write.descriptorType = Helpers::DescriptorType();
						}
						Helpers::FillWriteInfo(boundObject, &infos[i], &write);
					}

					if (data.dirtyIndices.size() > 0u)
						vkUpdateDescriptorSets(*m_owner->m_device, static_cast<uint32_t>(data.dirtyIndices.size()), writes.data(), 0, nullptr);
					data.dirtyIndices.clear();
					data.dirty = false;
				}
				return data.descriptorSet;
			}

			template<typename DataType>
			inline std::mutex& VulkanBindlessInstance<DataType>::GetDescriptorSetLock(size_t inFlightBufferId) {
				if (inFlightBufferId >= m_bufferData.size()) {
					m_owner->m_device->Log()->Error(
						"VulkanBindlessInstance<", TypeId::Of<DataType>().Name(), ">::GetDescriptorSetLock - inFlightBufferId(", inFlightBufferId, " out of bounds!)! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					static thread_local std::mutex noLock;
					return noLock;
				}
				else return m_bufferData[inFlightBufferId].updateLock;
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
#endif
