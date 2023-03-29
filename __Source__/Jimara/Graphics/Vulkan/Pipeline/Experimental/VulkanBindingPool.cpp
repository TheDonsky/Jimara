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
			};

			VulkanBindingPool::VulkanBindingPool(VulkanDevice* device) 
				: m_device(device) { 
				assert(m_device != nullptr);
			}

			VulkanBindingPool::~VulkanBindingPool() {
				// __TODO__: Implement this crap!
			}

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

				// __TODO__: Implement this crap!
				return fail("Not yet implemented! [File:", __FILE__, "; Line: ", __LINE__, "]");
			}

			void VulkanBindingPool::UpdateAllBindingSets(size_t inFlightCommandBufferIndex) {
				// __TODO__: Implement this crap!
			}



			VulkanBindingSet::VulkanBindingSet(SetBindings&& bindings)
				: m_bindings(std::move(bindings)) {

			}

			VulkanBindingSet::~VulkanBindingSet() {
				// __TODO__: Implement this crap!
			}
		}
		}
	}
}
