#include "VulkanBindingPool.h"
#include "../VulkanBindlessSet.h"


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
					const size_t m_totalBindingCount;

					inline BindingBucket(
						VulkanDevice* device, 
						VkDescriptorPool pool, 
						size_t bindingCount)
						: m_device(device), m_descriptorPool(pool)
						, m_totalBindingCount(bindingCount) {
						assert(m_device != nullptr);
						assert(m_descriptorPool != VK_NULL_HANDLE);
						assert(m_totalBindingCount > 0u);
					}

				public:
					inline static Reference<BindingBucket> Create(VulkanDevice* device, size_t bindingCount) {
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

						const Reference<BindingBucket> bucket = new BindingBucket(device, pool, bindingCount);
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
							return vkAllocateDescriptorSets(*m_device, &allocInfo, sets.Data());
						}();
						if (result != VK_SUCCESS) return result;

						return VK_SUCCESS;
					}

					inline void Free(const VulkanBindingSet::SetBindings& bindings, const VulkanBindingSet::DescriptorSets& sets) {
						if (vkFreeDescriptorSets(*m_device, m_descriptorPool, static_cast<uint32_t>(sets.Size()), sets.Data()) != VK_SUCCESS)
							m_device->Log()->Error(
								"VulkanBindingPool::Helpers::BindingBucket::Free - Failed to free binding sets! ",
								"[File:", __FILE__, "; Line: ", __LINE__, "]");
					}
				};

				inline static void UpdateDescriptorSets(VulkanDevice* device, VulkanBindingSet* const* sets, size_t count, size_t inFlightBufferId) {

					auto fail = [&](const auto... message) {
						device->Log()->Error("VulkanBindingPool::Helpers::UpdateDescriptorSets - ", message...);
					};
					
					static thread_local std::vector<VkWriteDescriptorSet> updates;
					updates.clear();

					VulkanBindingSet* const* setPtr = sets;
					VulkanBindingSet* const* const setEnd = setPtr + count;
					while (setPtr < setEnd) {
						VulkanBindingSet* set = *setPtr;
						setPtr++;

						const VulkanBindingSet::SetBindings& bindings = set->m_bindings;
						Reference<Object>* boundObjectPtr = 
							set->m_boundObjects.Data() + inFlightBufferId * set->m_setBindingCount;

						// Bindless buffers just need to assign the bound object:
						if (bindings.bindlessStructuredBuffers != nullptr)
							(*boundObjectPtr) = bindings.bindlessStructuredBuffers->BoundObject();

						// Bindless samplers just need to assign the bound object:
						else if (bindings.bindlessTextureSamplers != nullptr)
							(*boundObjectPtr) = bindings.bindlessTextureSamplers->BoundObject();
						
						else {
							VkDescriptorSet descriptorSet = set->m_descriptors[inFlightBufferId];

							auto updateBoundObjectEntry = [&](const auto& list, const auto& updateDirty) {
								const auto* infoPtr = list.Data();
								const auto* const infoEnd = infoPtr + list.Size();
								while (infoPtr < infoEnd) {
									const auto& info = *infoPtr;
									infoPtr++;
									Reference<Object>& boundObject = (*boundObjectPtr);
									boundObjectPtr++;
									using BoundObjectType = std::remove_pointer_t<decltype(info.binding->BoundObject())>;
									const Reference<BoundObjectType> objectToBind = info.binding->BoundObject();
									if (boundObject == objectToBind) continue;
									updateDirty(objectToBind.operator->(), info.bindingIndex);
									boundObject = objectToBind;
								}
							};

							// Check if any Cbuffer needs to be updated:
							updateBoundObjectEntry(bindings.constantBuffers, [&](Buffer* objectToBind, uint32_t bindingIndex) {
								fail("Setting Cbuffers not implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
								// __TODO__: Implement this crap!
								});

							// Check if any Structured Buffer needs to be updated:
							updateBoundObjectEntry(bindings.structuredBuffers, [&](ArrayBuffer* objectToBind, uint32_t bindingIndex) {
								fail("Setting Structured Buffers not implemented![File:", __FILE__, "; Line: ", __LINE__, "]");
								// __TODO__: Implement this crap!
								});

							// Check if any Texture sampler needs to be updated:
							updateBoundObjectEntry(bindings.textureSamplers, [&](TextureSampler* objectToBind, uint32_t bindingIndex) {
								fail("Setting Texture Samplers not implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
								// __TODO__: Implement this crap!
								});

							// Check if any Texture view needs to be updated:
							updateBoundObjectEntry(bindings.textureViews, [&](TextureView* objectToBind, uint32_t bindingIndex) {
								fail("Setting Texture Views not implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
								// __TODO__: Implement this crap!
								});
						}
					}

					// Update descriptor sets:
					if (updates.size() > 0) {
						vkUpdateDescriptorSets(*device, static_cast<uint32_t>(updates.size()), updates.data(), 0, nullptr);
						updates.clear();
					}
				}
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
				PipelineStageMask stageMask = 0u;
				for (size_t bindingIndex = 0u; bindingIndex < setInfo.bindings.Size(); bindingIndex++) {
					const VulkanPipeline::BindingInfo& bindingInfo = setInfo.bindings[bindingIndex];
					if (!Helpers::FindBinding(m_device, bindingInfo, bindingIndex, descriptor, bindings))
						return fail("Failed to find binding for '",
							(bindingInfo.nameAliases.Size() > 0u) ? bindingInfo.nameAliases[0] : std::string_view(""),
							"'(set: ", descriptor.bindingSetId, "; binding: ", bindingInfo.binding, ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					stageMask |= bindingInfo.stageMask;
				}

				Reference<Helpers::BindingBucket> bindingBucket;
				VulkanBindingSet::DescriptorSets descriptors;

				auto createSet = [&]() {
					const Reference<VulkanBindingSet> bindingSet = new VulkanBindingSet(
						this, pipeline, bindingBucket, std::move(bindings), std::move(descriptors), descriptor.bindingSetId, stageMask);
					bindingSet->ReleaseRef();
					return bindingSet;
				};

				if ((bindings.bindlessStructuredBuffers != nullptr) ||
					(bindings.bindlessTextureSamplers != nullptr)) 
					return createSet();
				else descriptors.Resize(m_inFlightCommandBufferCount);

				std::unique_lock<std::mutex> allocationLock(m_poolDataLock);
				
				bindingBucket = m_bindingBucket;
				const size_t requiredBindingCount = Helpers::RequiredBindingCount(bindings, m_inFlightCommandBufferCount);
				if (bindingBucket == nullptr)
					bindingBucket = Helpers::BindingBucket::Create(m_device, requiredBindingCount);
				
				while (true) {
					if (bindingBucket == nullptr)
						return fail("Failed to allocate binding bucket! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					const VkResult result = bindingBucket->TryAllocate(bindings, setInfo.layout, descriptors);
					if (result == VK_SUCCESS) {
						m_bindingBucket = bindingBucket;
						return createSet();
					}
					else if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
						bindingBucket = Helpers::BindingBucket::Create(
							m_device, Math::Max(requiredBindingCount, bindingBucket->BindingCount() << 1u));
					else return fail("Failed to allocate binding descriptors! [File:", __FILE__, "; Line: ", __LINE__, "]");
				}
			}

			void VulkanBindingPool::UpdateAllBindingSets(size_t inFlightCommandBufferIndex) {
				m_device->Log()->Error("VulkanBindingPool::UpdateAllBindingSets - Note yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				std::unique_lock<std::mutex> allocationLock(m_poolDataLock);
				if (m_allocatedSets.sets.size() != m_allocatedSets.sortedSets.size()) {
					m_allocatedSets.sortedSets.clear();
					for (std::set<VulkanBindingSet*>::const_iterator it = m_allocatedSets.sets.begin(); it != m_allocatedSets.sets.end(); ++it)
						m_allocatedSets.sortedSets.push_back(*it);
				}
				Helpers::UpdateDescriptorSets(
					m_device, m_allocatedSets.sortedSets.data(), m_allocatedSets.sortedSets.size(), inFlightCommandBufferIndex);
			}



			VulkanBindingSet::VulkanBindingSet(
				VulkanBindingPool* bindingPool, const VulkanPipeline* pipeline, Object* bindingBucket,
				SetBindings&& bindings, DescriptorSets&& descriptors,
				uint32_t bindingSetIndex, size_t pipelineStageMask)
				: m_pipeline(pipeline)
				, m_bindingPool(bindingPool)
				, m_bindingBucket(bindingBucket)
				, m_bindings(std::move(bindings))
				, m_descriptors(std::move(descriptors))
				, m_bindingSetIndex(bindingSetIndex)
				, m_pipelineStageMask(pipelineStageMask) {
				assert(m_pipeline != nullptr);
				assert(m_bindingPool != nullptr);
				assert(
					m_descriptors.Size() == 0u || 
					m_descriptors.Size() == m_bindingPool->m_inFlightCommandBufferCount);
				
				m_setBindingCount =
					m_bindings.constantBuffers.Size() +
					m_bindings.structuredBuffers.Size() +
					m_bindings.textureSamplers.Size() +
					m_bindings.textureViews.Size() +
					size_t(m_bindings.bindlessStructuredBuffers == nullptr ? 0u : 1u) +
					size_t(m_bindings.bindlessTextureSamplers == nullptr ? 0u : 1u);
				m_boundObjects.Resize(m_setBindingCount * m_bindingPool->m_inFlightCommandBufferCount);
				
				if (m_bindingBucket == nullptr) return;
				m_bindingPool->m_allocatedSets.sets.insert(this);
				m_bindingPool->m_allocatedSets.sortedSets.clear();
			}

			VulkanBindingSet::~VulkanBindingSet() {
				if (m_bindingBucket == nullptr) return;
				VulkanBindingPool::Helpers::BindingBucket* bindingBucket =
					dynamic_cast<VulkanBindingPool::Helpers::BindingBucket*>(m_bindingBucket.operator->());
				std::unique_lock<std::mutex> poolDataLock(m_bindingPool->m_poolDataLock);
				bindingBucket->Free(m_bindings, m_descriptors);
				m_bindingPool->m_allocatedSets.sets.erase(this);
				m_bindingPool->m_allocatedSets.sortedSets.clear();
			}

			void VulkanBindingSet::Update(size_t inFlightCommandBufferIndex) {
				if (inFlightCommandBufferIndex >= m_bindingPool->m_inFlightCommandBufferCount) {
					m_bindingPool->m_device->Log()->Error(
						"VulkanBindingSet::Update - inFlightCommandBufferIndex out of bounds! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				std::unique_lock<std::mutex> poolDataLock(m_bindingPool->m_poolDataLock);
				VulkanBindingSet* const self = this;
				VulkanBindingPool::Helpers::UpdateDescriptorSets(
					m_bindingPool->m_device, &self, 1u, inFlightCommandBufferIndex);
			}

			void VulkanBindingSet::Bind(InFlightBufferInfo inFlightBuffer) {
				auto fail = [&](const auto&... message) {
					m_bindingPool->m_device->Log()->Error("VulkanBindingSet::Update - ", message...);
				};

				VulkanCommandBuffer* const commandBuffer = dynamic_cast<VulkanCommandBuffer*>(inFlightBuffer.commandBuffer);
				if (commandBuffer == nullptr)
					return fail("Null or incompatible command buffer provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				if (inFlightBuffer.inFlightBufferId >= m_bindingPool->m_inFlightCommandBufferCount)
					return fail("inFlightCommandBufferIndex out of bounds! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const VkPipelineLayout layout = m_pipeline->PipelineLayout();
				const uint32_t bindingSetIndex = m_bindingSetIndex;
				const VkCommandBuffer buffer = *commandBuffer;

				auto bindDescriptors = [&](const VkDescriptorSet set, const VkPipelineBindPoint bindPoint) {
					vkCmdBindDescriptorSets(buffer, bindPoint, layout, bindingSetIndex, 1u, &set, 0, nullptr);
				};

				auto bindOnAllPoints = [&](const VkDescriptorSet set) {
					const PipelineStageMask mask = m_pipelineStageMask;
					auto hasStage = [&](const auto stage) { return (mask & static_cast<PipelineStageMask>(stage)) != 0u; };
					if (hasStage(PipelineStage::COMPUTE)) 
						bindDescriptors(set, VK_PIPELINE_BIND_POINT_COMPUTE);
					if (hasStage(StageMask(PipelineStage::FRAGMENT, PipelineStage::VERTEX)))
						bindDescriptors(set, VK_PIPELINE_BIND_POINT_GRAPHICS);
				};

				auto bindBindless = [&](const auto& castBoundObject) {
					std::unique_lock<std::mutex> poolDataLock(m_bindingPool->m_poolDataLock);
					auto* bindlessSet = castBoundObject(m_boundObjects[inFlightBuffer.inFlightBufferId].operator->());
					if (bindlessSet == nullptr)
						return fail(
							"Binding set correspons to a ", TypeId::Of<std::remove_pointer_t<decltype(bindlessSet)>>().Name(),
							" instance, but no valid address is set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					bindOnAllPoints(bindlessSet->GetDescriptorSet(inFlightBuffer.inFlightBufferId));
				};

				if (m_bindings.bindlessStructuredBuffers != nullptr)
					bindBindless([](Object* boundObject) { return dynamic_cast<VulkanBindlessInstance<ArrayBuffer>*>(boundObject); });
				else if (m_bindings.bindlessTextureSamplers != nullptr)
					bindBindless([](Object* boundObject) { return dynamic_cast<VulkanBindlessInstance<TextureSampler>*>(boundObject); });
				else {
					bindOnAllPoints(m_descriptors[inFlightBuffer.inFlightBufferId]);
					if (m_bindings.textureViews.Size() > 0u) {
						std::unique_lock<std::mutex> poolDataLock(m_bindingPool->m_poolDataLock);
						// __TODO__: Implement this crap!
						fail ("Texture views need layout transition and are not yet supported! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
				}
			}
		}
		}
	}
}
