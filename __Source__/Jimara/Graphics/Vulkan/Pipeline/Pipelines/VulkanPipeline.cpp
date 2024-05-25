#include "VulkanPipeline.h"
#include "../Bindings/VulkanBindlessSet.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			struct VulkanPipeline::Helpers {
				inline static bool AddBinding(BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) {
					// Find or allocate binding set info within the pipeline:
					if (self->m_bindingSetInfos.Size() <= bindingInfo.set)
						self->m_bindingSetInfos.Resize(bindingInfo.set + 1u);
					DescriptorSetInfo& setInfo = self->m_bindingSetInfos[bindingInfo.set];
					
					// Find or allocate binding info within the pipeline:
					// (no lookup table necessary, since I would not expect to have more than 16 or so bindings per set)
					size_t bindingIndex = setInfo.bindings.Size();
					for (size_t i = 0; i < bindingIndex; i++)
						if (setInfo.bindings[i].binding == bindingInfo.binding)
							bindingIndex = i;
					if (bindingIndex >= setInfo.bindings.Size())
						setInfo.bindings.Resize(bindingIndex + 1u);
					BindingInfo& binding = setInfo.bindings[bindingIndex];
					
					// Make sure binding id is set and stage mask is included:
					binding.binding = bindingInfo.binding;
					binding.stageMask |= stages;

					// Find or add name alias to the binding:
					// (no lookup table necessary, since I would not expect to have too many aliases)
					bool nameAliasIsNew = true;
					for (size_t i = 0; i < binding.nameAliases.Size(); i++)
						if (binding.nameAliases[i] == bindingInfo.name) {
							nameAliasIsNew = false;
							break;
						}
					if (nameAliasIsNew)
						binding.nameAliases.Push(bindingInfo.name);

					// Set type of the binding:
					if (binding.type >= SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
						binding.type = bindingInfo.type;
					else if (bindingInfo.type < SPIRV_Binary::BindingInfo::Type::TYPE_COUNT && binding.type != bindingInfo.type) {
						self->m_device->Log()->Warning("VulkanPipeline::Helpers::AddBinding - Binding type mismatch between included shaders! ",
							"(Set: ", bindingInfo.set, "; Binding: ", bindingInfo.binding, "; Name: '", bindingInfo.name, "'; ",
							"Type: ", static_cast<size_t>(bindingInfo.type), "; StageMask: ", static_cast<size_t>(stages), ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}

					// Log a warning in case we have an unknown binding type:
					if (bindingInfo.type >= SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
						self->m_device->Log()->Warning("VulkanPipeline::Helpers::AddBinding - Got binding of an unsupported type ",
							"(Set: ", bindingInfo.set, "; Binding: ", bindingInfo.binding, "; Name: '", bindingInfo.name, "'; ",
							"Type: ", static_cast<size_t>(bindingInfo.type), "; StageMask: ", static_cast<size_t>(stages), ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");

					return true;
				}


				inline static bool CreateDescriptorSetLayout(VulkanDevice* device, size_t setIndex, DescriptorSetInfo& setInfo) {
					// Vulkan binding descriptor:
					static thread_local std::vector<VkDescriptorSetLayoutBinding> bindings;
					bindings.clear();
					auto addBinding = [](PipelineStageMask stages, size_t shaderBinding, VkDescriptorType type) {
						VkDescriptorSetLayoutBinding binding = {};
						binding.binding = static_cast<uint32_t>(shaderBinding);
						binding.descriptorType = type;
						binding.descriptorCount = 1;
						binding.stageFlags =
							(((stages & PipelineStage::COMPUTE) != PipelineStage::NONE) ? VK_SHADER_STAGE_COMPUTE_BIT : 0) |
							(((stages & PipelineStage::VERTEX) != PipelineStage::NONE) ? VK_SHADER_STAGE_VERTEX_BIT : 0) |
							(((stages & PipelineStage::FRAGMENT) != PipelineStage::NONE) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0);
						binding.pImmutableSamplers = nullptr;
						bindings.push_back(binding);
					};

					// Engine to Vulkan binding type translation:
					static const constexpr VkDescriptorType BINDLESS_DESCRIPTOR_TYPE = VK_DESCRIPTOR_TYPE_MAX_ENUM;
					struct VulkanDescriptorTypeInfo {
						bool supported = false;
						VkDescriptorType type = BINDLESS_DESCRIPTOR_TYPE;
						VkDescriptorSetLayout(*createBindlessDescriptorSetLayout)(VulkanDevice*) = nullptr;
					};
					static const VulkanDescriptorTypeInfo* TYPE_TRANSLATION_TABLE = []() -> const VulkanDescriptorTypeInfo* {
						static VulkanDescriptorTypeInfo table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)];
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = { true, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr };
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = { true, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr };
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] = { true, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr };
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = { true, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr };
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::ACCELERATION_STRUCTURE)] = { 
							true, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, nullptr
						};
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] = { 
							true, BINDLESS_DESCRIPTOR_TYPE, VulkanBindlessInstance<TextureSampler>::CreateDescriptorSetLayout
						};
						table[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] = { 
							true, BINDLESS_DESCRIPTOR_TYPE, VulkanBindlessInstance<ArrayBuffer>::CreateDescriptorSetLayout
						};
						return table;
					}();

					// Add descriptions for bindings:
					for (size_t i = 0; i < setInfo.bindings.Size(); i++) {
						const BindingInfo& bindingInfo = setInfo.bindings[i];
						auto fail = [&](const auto& message, const auto& file, const auto& line) -> bool {
							device->Log()->Error("VulkanPipeline::Helpers::CreateDescriptorSetLayout - ", message, " ",
								"(Set: ", setIndex, "; Binding: ", bindingInfo.binding, "; Name: '", bindingInfo.nameAliases[0], "'; ",
								"Type: ", static_cast<size_t>(bindingInfo.type), "; StageMask: ", static_cast<size_t>(bindingInfo.stageMask), ")! ",
								"[File: ", file, "; Line: ", line, "]");
							bindings.clear();
							return false;
						};

						if (bindingInfo.type >= SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
							return fail("Binding has an unknown type!", __FILE__, __LINE__);

						const VulkanDescriptorTypeInfo typeInfo = TYPE_TRANSLATION_TABLE[static_cast<size_t>(bindingInfo.type)];
						if (!typeInfo.supported) 
							return fail("Binding type not supported!", __FILE__, __LINE__);

						if (typeInfo.type == BINDLESS_DESCRIPTOR_TYPE) {
							if (i != 0u || setInfo.bindings.Size() > 1u)
								return fail("Bindless array has to be bound to binding 0 and has to be the only binding within it's set!", __FILE__, __LINE__);
							setInfo.layout = typeInfo.createBindlessDescriptorSetLayout(device);
							if (setInfo.layout == VK_NULL_HANDLE)
								return fail("Failed to create descriptor set layout for bindless resources!", __FILE__, __LINE__);
							else return true;
						}
						else addBinding(bindingInfo.stageMask, bindingInfo.binding, typeInfo.type);
					}

					// Create binding set:
					VkDescriptorSetLayoutCreateInfo layoutInfo = {};
					{
						layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
						layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
						layoutInfo.pBindings = bindings.data();
					}
					VkDescriptorSetLayout layout;
					if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
						device->Log()->Error(
							"VulkanPipeline::Helpers::CreateDescriptorSetLayout - Failed to create descriptor set layout! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					else {
						setInfo.layout = layout;
						return true;
					}
				}

				inline static void DestroyBindings(VulkanDevice* device, const BindingSetInfos& setInfos, VkPipelineLayout layout) {
					if (layout != VK_NULL_HANDLE)
						vkDestroyPipelineLayout(*device, layout, nullptr);
					for (size_t i = 0; i < setInfos.Size(); i++) {
						const DescriptorSetInfo& info = setInfos[i];
						if (info.layout != VK_NULL_HANDLE)
							vkDestroyDescriptorSetLayout(*device, info.layout, nullptr);
					}
				}
			};


			VulkanPipeline::VulkanPipeline(BindingSetBuilder&& builder) 
				: m_device(builder.m_device)
				, m_shaders(std::move(builder.m_shaders))
				, m_bindingSetInfos(std::move(builder.m_bindingSetInfos))
				, m_pipelineLayout(builder.m_pipelineLayout) {
				if (builder.m_failed || m_pipelineLayout == VK_NULL_HANDLE)
					m_device->Log()->Error(
						"VulkanPipeline::VulkanPipeline - BindingSetBuilder failed or incomplete! ",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
				builder.m_bindingSetInfos.Clear();
				builder.m_pipelineLayout = VK_NULL_HANDLE;
			}

			VulkanPipeline::~VulkanPipeline() {
				Helpers::DestroyBindings(m_device, m_bindingSetInfos, m_pipelineLayout);
			}


			VulkanPipeline::BindingSetBuilder::BindingSetBuilder(VulkanDevice* device) 
				: m_device(device) {
				assert(m_device != nullptr);
			}

			VulkanPipeline::BindingSetBuilder::~BindingSetBuilder() {
				Helpers::DestroyBindings(m_device, m_bindingSetInfos, m_pipelineLayout);
			}

			bool VulkanPipeline::BindingSetBuilder::IncludeShaderBindings(const SPIRV_Binary* shader) {
				// No shader means no work to be done:
				if (shader == nullptr) return true;
				m_shaders.Push(shader);

				// If something failed already, why bother?
				if (m_failed) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::IncludeShaderBindings - Binding set has already failed! ",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				// If we already finished building the pipeline layout, we can not really add more shaders:
				if (m_pipelineLayout != VK_NULL_HANDLE) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::IncludeShaderBindings - BindingSetBuilder::Finish() already invoked; ",
						"additional shaders can not be included! [File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				
				// Add all bindings:
				const size_t shaderSetCount = shader->BindingSetCount();
				const PipelineStageMask shaderStages = shader->ShaderStages();
				for (size_t setIndex = 0u; setIndex < shaderSetCount; setIndex++) {
					const SPIRV_Binary::BindingSetInfo& setInfo = shader->BindingSet(setIndex);
					const size_t setBindingCount = setInfo.BindingCount();
					for (size_t bindingIndex = 0u; bindingIndex < setBindingCount; bindingIndex++)
						if (!Helpers::AddBinding(this, shaderStages, setInfo.Binding(bindingIndex)))
							m_failed = true;
				}

				return (!m_failed);
			}

			bool VulkanPipeline::BindingSetBuilder::Finish() {
				// If Finish was successful already, no need to do anything:
				if (m_pipelineLayout != VK_NULL_HANDLE) return true;

				// If something failed already, why bother?
				if (m_failed) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::Finish - Binding set has failed on previous call(s)! ",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				// We will need to cleanup on failure:
				auto fail = [&]() -> bool {
					m_failed = true;
					Helpers::DestroyBindings(m_device, m_bindingSetInfos, m_pipelineLayout);
					return false;
				};

				// Create set layouts:
				static thread_local std::vector<VkDescriptorSetLayout> setLayouts;
				setLayouts.resize(m_bindingSetInfos.Size());
				for (size_t setIndex = 0u; setIndex < m_bindingSetInfos.Size(); setIndex++) {
					DescriptorSetInfo& setInfo = m_bindingSetInfos[setIndex];
					if (!Helpers::CreateDescriptorSetLayout(m_device, setIndex, setInfo))
						return fail();
					else if (setInfo.layout == VK_NULL_HANDLE) {
						m_device->Log()->Error(
							"VulkanPipeline::BindingSetBuilder::Finish - Internal error: Descriptor set layout missing for set ", setIndex, "! ",
							"[File:", __FILE__, "; Line: ", __LINE__, "]");
						return fail();
					}
					else setLayouts[setIndex] = setInfo.layout;
				}

				// Create pipeline layout:
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				{
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
					pipelineLayoutInfo.pSetLayouts = (setLayouts.size() > 0) ? setLayouts.data() : nullptr;
					pipelineLayoutInfo.pushConstantRangeCount = 0;
				}
				VkPipelineLayout pipelineLayout;
				if (vkCreatePipelineLayout(*m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::Finish - Failed to create pipeline layout!",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return fail();
				}
				else m_pipelineLayout = pipelineLayout;
				return true;
			}

			bool VulkanPipeline::BindingSetBuilder::Failed()const { return m_failed; }
		}
	}
}
