#include "VulkanBindingPool.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			struct VulkanBindingPool::Helpers {
				template<typename ResourceType, typename OnBindingFoundFn>
				inline static bool FindByAliases(
					const VulkanPipeline::BindingInfo& bindingInfo,
					const BindingSet::Descriptor::BindingSearchFn<ResourceType> bindingSearchFn,
					const OnBindingFoundFn& onBindingFound) {
					auto tryFind = [&](const std::string_view& nameAlias) -> bool {
						VulkanBindingSet::Binding<ResourceType> binding = bindingSearchFn(
							BindingSet::BindingDescriptor{ nameAlias, bindingInfo.binding });
						if (binding != nullptr) {
							onBindingFound(binding);
							return true;
						}
						else return false;
					};
					if (bindingInfo.nameAliases.Size() <= 0u) {
						if (tryFind(""))
							return true;
					}
					else for (size_t i = 0u; i < bindingInfo.nameAliases.Size(); i++)
						if (tryFind(bindingInfo.nameAliases[i]))
							return true;
					return false;
				}

				template<typename ResourceType>
				inline static bool FindSingleBinding(
					const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
					const BindingSet::Descriptor::BindingSearchFn<ResourceType> bindingSearchFn,
					VulkanBindingSet::Bindings<ResourceType>& bindings) {
					auto onBindingFound = [&](const ResourceBinding<ResourceType>* binding) {
						bindings.Push(VulkanBindingSet::BindingInfo<ResourceType> { binding, static_cast<uint32_t>(bindingIndex) });
					};
					return FindByAliases<ResourceType>(bindingInfo, bindingSearchFn, onBindingFound);
				}

				template<typename ResourceType>
				inline static bool FindBindlessSetInstance(
					const VulkanPipeline::BindingInfo& bindingInfo,
					const BindingSet::Descriptor::BindingSearchFn<ResourceType> bindingSearchFn,
					VulkanBindingSet::Binding<ResourceType>& bindingReference) {
					auto onBindingFound = [&](const ResourceBinding<ResourceType>* binding) {
						bindingReference = binding;
					};
					return FindByAliases<ResourceType>(bindingInfo, bindingSearchFn, onBindingFound);
				}

				inline static bool FindBinding(
					VulkanDevice* device,
					const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
					const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) {

					typedef bool(*FindBindingFn)(const VulkanPipeline::BindingInfo&, size_t, const BindingSet::Descriptor&, VulkanBindingSet::SetBindings&);
					static const FindBindingFn* findBinding = []() -> const FindBindingFn* {
						static const constexpr size_t TYPE_COUNT = static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TYPE_COUNT);
						static FindBindingFn findFunctions[TYPE_COUNT];
						for (size_t i = 0u; i < TYPE_COUNT; i++)
							findFunctions[i] = nullptr;

						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindSingleBinding(bindingInfo, bindingIndex, descriptor.findConstantBuffer, bindings.constantBuffers);
						};
						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindSingleBinding(bindingInfo, bindingIndex, descriptor.findTextureSampler, bindings.textureSamplers);
						};
						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindSingleBinding(bindingInfo, bindingIndex, descriptor.findTextureView, bindings.textureViews);
						};
						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t bindingIndex,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindSingleBinding(bindingInfo, bindingIndex, descriptor.findStructuredBuffer, bindings.structuredBuffers);
						};
						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindBindlessSetInstance(bindingInfo, descriptor.findBindlessTextureSamplers, bindings.bindlessTextureSamplers);
						};
						findFunctions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] = [](
							const VulkanPipeline::BindingInfo& bindingInfo, size_t,
							const BindingSet::Descriptor& descriptor, VulkanBindingSet::SetBindings& bindings) -> bool {
								return FindBindlessSetInstance(bindingInfo, descriptor.findBindlessStructuredBuffers, bindings.bindlessStructuredBuffers);
						};
						return findFunctions;
					}();

					const FindBindingFn findFn = (bindingInfo.type < SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
						? findBinding[static_cast<size_t>(bindingInfo.type)] : nullptr;
					if (findFn == nullptr) {
						device->Log()->Error(
							"VulkanBindingPool::Helpers::FindBinding - Unsupported binding type: ", static_cast<size_t>(bindingInfo.type), "! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					else return findFn(bindingInfo, bindingIndex, descriptor, bindings);
				}

				inline static size_t RequiredBindingCount(const VulkanBindingSet::SetBindings& bindings, size_t inFlightBufferCount) {
					return Math::Max(inFlightBufferCount * Math::Max(
						Math::Max(bindings.constantBuffers.Size(), bindings.structuredBuffers.Size()),
						Math::Max(bindings.textureSamplers.Size(), bindings.textureViews.Size())), size_t(1u));
				}

				class BindingBucket : public virtual Object {
				private:
					const Reference<VulkanDevice> m_device;
					const VkDescriptorPool m_descriptorPool;
					const std::shared_ptr<SpinLock> m_descriptorWriteLock;
					const size_t m_totalBindingCount;

					inline BindingBucket(
						VulkanDevice* device, 
						VkDescriptorPool pool, 
						size_t bindingCount, 
						const std::shared_ptr<SpinLock>& descriptorWriteLock)
						: m_device(device), m_descriptorPool(pool)
						, m_descriptorWriteLock(descriptorWriteLock)
						, m_totalBindingCount(bindingCount) {
						assert(m_device != nullptr);
						assert(m_descriptorPool != VK_NULL_HANDLE);
						assert(m_totalBindingCount > 0u);
					}

				public:
					inline static Reference<BindingBucket> Create(
						VulkanDevice* device, 
						size_t bindingCount, 
						const std::shared_ptr<SpinLock>& descriptorWriteLock) {
						if (device == nullptr) return nullptr;
						bindingCount = Math::Max(bindingCount, size_t(1u));
						
						Stacktor<VkDescriptorPoolSize, 4u> sizes;
						auto addType = [&](VkDescriptorType type) {
							VkDescriptorPoolSize size = {};
							size.type = type;
							size.descriptorCount = bindingCount;
							sizes.Push(size);
						};
						addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
						addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
						addType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
						addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
						
						VkDescriptorPoolCreateInfo createInfo = {};
						{
							createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
							createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
							createInfo.poolSizeCount = static_cast<uint32_t>(sizes.Size());
							createInfo.pPoolSizes = sizes.Data();
							createInfo.maxSets = static_cast<uint32_t>(bindingCount * sizes.Size());
						}
						VkDescriptorPool pool = VK_NULL_HANDLE;
						if (vkCreateDescriptorPool(*device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
							pool = VK_NULL_HANDLE;
							device->Log()->Error(
								"VulkanBindingPool::Helpers::BindingBucket::Create - ",
								"Failed to create descriptor pool! [File:", __FILE__, "; Line: ", __LINE__, "]");
							return nullptr;
						}

						const Reference<BindingBucket> bucket = new BindingBucket(device, pool, bindingCount, descriptorWriteLock);
						bucket->ReleaseRef();
						return bucket;
					}

					inline virtual ~BindingBucket() {
						vkDestroyDescriptorPool(*m_device, m_descriptorPool, nullptr);
					}

					inline size_t BindingCount()const { return m_totalBindingCount; }

					inline VkResult TryAllocate(
						const VulkanBindingSet::SetBindings& bindings, 
						VkDescriptorSetLayout layout, 
						VulkanBindingSet::DescriptorSets& sets) {
						static thread_local std::vector<VkDescriptorSetLayout> layouts;
						static thread_local std::vector<VkDescriptorSet> descriptorSets;
						descriptorSets.resize(sets.Size());
						layouts.resize(sets.Size());

						for (size_t i = 0u; i < sets.Size(); i++)
							layouts[i] = layout;

						VkDescriptorSetAllocateInfo allocInfo = {};
						{
							allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
							allocInfo.descriptorPool = m_descriptorPool;
							allocInfo.descriptorSetCount = static_cast<uint32_t>(sets.Size());
							allocInfo.pSetLayouts = layouts.data();
						}

						const VkResult result = [&]() -> VkResult {
							std::unique_lock<SpinLock> lock(*m_descriptorWriteLock);
							return vkAllocateDescriptorSets(*m_device, &allocInfo, descriptorSets.data());
						}();
						if (result != VK_SUCCESS) return result;

						for (size_t i = 0u; i < sets.Size(); i++)
							sets[i].descriptorSet = descriptorSets[i];

						return VK_SUCCESS;
					}

					inline void Free(const VulkanBindingSet::SetBindings& bindings, VulkanBindingSet::DescriptorSets& sets) {
						static thread_local std::vector<VkDescriptorSet> descriptorSets;
						descriptorSets.resize(sets.Size());
						for (size_t i = 0u; i < sets.Size(); i++) {
							descriptorSets[i] = sets[i].descriptorSet;
							if (descriptorSets[i] == VK_NULL_HANDLE)
								m_device->Log()->Error(
									"VulkanBindingPool::Helpers::BindingBucket::Free - Empty descriptor set not expected! ",
									"[File:", __FILE__, "; Line: ", __LINE__, "]");
						}

						std::unique_lock<SpinLock> lock(*m_descriptorWriteLock);
						if (vkFreeDescriptorSets(
							*m_device, m_descriptorPool,
							static_cast<uint32_t>(descriptorSets.size()),
							descriptorSets.data()) != VK_SUCCESS)
							m_device->Log()->Error(
								"VulkanBindingPool::Helpers::BindingBucket::Free - Failed to free binding sets! ",
								"[File:", __FILE__, "; Line: ", __LINE__, "]");
					}
				};
			};

			VulkanBindingPool::VulkanBindingPool(VulkanDevice* device, size_t inFlightCommandBufferCount)
				: m_device(device), m_inFlightCommandBufferCount(Math::Max(inFlightCommandBufferCount, size_t(1u))) {
				assert(m_device != nullptr);
			}

			VulkanBindingPool::~VulkanBindingPool() {}

			Reference<BindingSet> VulkanBindingPool::AllocateBindingSet(const BindingSet::Descriptor& descriptor) {
				auto fail = [&](const auto&... message) {
					m_device->Log()->Error("VulkanBindingPool::AllocateBindingSet - ", message...);
					return nullptr;
				};

				const VulkanPipeline* pipeline = dynamic_cast<const VulkanPipeline*>(descriptor.pipeline.operator->());
				if (pipeline == nullptr)
					return fail("VulkanPipeline instance not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				if (descriptor.bindingSetId >= pipeline->BindingSetCount())
					return fail("Requested binding set ", descriptor.bindingSetId, 
						" while the pipeline has only ", pipeline->BindingSetCount(), " set descriptors! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");

				const VulkanPipeline::DescriptorSetInfo& setInfo = pipeline->BindingSetInfo(descriptor.bindingSetId);
				VulkanBindingSet::SetBindings bindings = {};
				for (size_t bindingIndex = 0u; bindingIndex < setInfo.bindings.Size(); bindingIndex++) {
					const VulkanPipeline::BindingInfo& bindingInfo = setInfo.bindings[bindingIndex];
					if (!Helpers::FindBinding(m_device, bindingInfo, bindingIndex, descriptor, bindings))
						return fail("Failed to find binding for '",
							(bindingInfo.nameAliases.Size() > 0u) ? bindingInfo.nameAliases[0] : std::string_view(""),
							"'(set: ", descriptor.bindingSetId, "; binding: ", bindingInfo.binding, ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				Reference<Helpers::BindingBucket> bindingBucket;
				VulkanBindingSet::DescriptorSets descriptors;
				descriptors.Resize(m_inFlightCommandBufferCount);

				auto createSet = [&]() {
					const Reference<VulkanBindingSet> bindingSet =
						new VulkanBindingSet(this, pipeline, bindingBucket, std::move(bindings), std::move(descriptors));
					bindingSet->ReleaseRef();
					return bindingSet;
				};

				if ((bindings.bindlessStructuredBuffers != nullptr) ||
					(bindings.bindlessTextureSamplers != nullptr)) 
					return createSet();

				std::unique_lock<std::mutex> allocationLock(m_bindingSetAllocationLock);
				
				bindingBucket = m_bindingBucket;
				const size_t requiredBindingCount = Helpers::RequiredBindingCount(bindings, m_inFlightCommandBufferCount);
				if (bindingBucket == nullptr)
					bindingBucket = Helpers::BindingBucket::Create(m_device, requiredBindingCount, m_descriptorWriteLock);
				
				while (true) {
					if (bindingBucket == nullptr)
						return fail("Failed to allocate binding bucket! [File:", __FILE__, "; Line: ", __LINE__, "]");
					const VkResult result = bindingBucket->TryAllocate(bindings, setInfo.layout, descriptors);
					if (result == VK_SUCCESS) {
						m_bindingBucket = bindingBucket;
						return createSet();
					}
					else if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
						bindingBucket = Helpers::BindingBucket::Create(
							m_device, Math::Max(requiredBindingCount, bindingBucket->BindingCount() << 1u), m_descriptorWriteLock);
					else return fail("Failed to allocate binding descriptors! [File:", __FILE__, "; Line: ", __LINE__, "]");
				}
			}

			void VulkanBindingPool::UpdateAllBindingSets(size_t inFlightCommandBufferIndex) {
				m_device->Log()->Error("VulkanBindingPool::UpdateAllBindingSets - Note yet implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
				// __TODO__: Implement this crap!
			}



			VulkanBindingSet::VulkanBindingSet(
				VulkanBindingPool* bindingPool, const VulkanPipeline* pipeline, Object* bindingBucket,
				SetBindings&& bindings, DescriptorSets&& descriptors)
				: m_pipeline(pipeline)
				, m_bindingPool(bindingPool)
				, m_bindingBucket(bindingBucket)
				, m_bindings(std::move(bindings))
				, m_descriptors(std::move(descriptors)) {
				assert(m_pipeline != nullptr);
				assert(m_bindingPool != nullptr);
				assert(m_descriptors.Size() == m_bindingPool->m_inFlightCommandBufferCount);
				if (m_bindingBucket == nullptr) return;
			}

			VulkanBindingSet::~VulkanBindingSet() {
				if (m_bindingBucket == nullptr) return;
				VulkanBindingPool::Helpers::BindingBucket* bindingBucket =
					dynamic_cast<VulkanBindingPool::Helpers::BindingBucket*>(m_bindingBucket.operator->());
				std::unique_lock<std::mutex> allocationLock(m_bindingPool->m_bindingSetAllocationLock);
				bindingBucket->Free(m_bindings, m_descriptors);
			}

			void VulkanBindingSet::Update(size_t inFlightCommandBufferIndex) {
				m_bindingPool->m_device->Log()->Error("VulkanBindingSet::Update - Note yet implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
				// __TODO__: Implement this crap!
			}

			void VulkanBindingSet::Bind(InFlightBufferInfo inFlightBuffer) {
				m_bindingPool->m_device->Log()->Error("VulkanBindingSet::Bind - Note yet implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
				// __TODO__: Implement this crap!
			}
		}
		}
	}
}
