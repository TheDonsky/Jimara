#include "VulkanPipeline.h"



namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			struct VulkanPipeline::Helpers {
				inline static bool AddBinding(
					BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) {
					typedef bool(*AddBindingOfType)(BindingSetBuilder*, PipelineStageMask, const SPIRV_Binary::BindingInfo&);
					static const constexpr AddBindingOfType addBindingOfUnknownType =
						[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
						self->m_device->Log()->Warning("VulkanPipeline::Helpers::AddBinding - Got binding of an unsupported type ",
							"(Set: ", bindingInfo.set, "; Binding: ", bindingInfo.binding, "; Name: '", bindingInfo.name, "'; ",
							"Type: ", static_cast<size_t>(bindingInfo.type), "; StageMask: ", static_cast<size_t>(stages), ")! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return true;
					};
					static const AddBindingOfType*const bindingAddFunctions = []() -> const AddBindingOfType* {
						static const constexpr size_t functionCount = static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TYPE_COUNT);
						static AddBindingOfType functions[functionCount];
						for (size_t i = 0; i < functionCount; i++)
							functions[i] = addBindingOfUnknownType;

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER_ARRAY)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE_ARRAY)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						functions[static_cast<size_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] =
							[](BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingInfo& bindingInfo) -> bool {
							// __TODO__: Implement this crap!
							self->m_device->Log()->Error("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};

						return functions;
					}();
					const AddBindingOfType addBinding = (bindingInfo.type >= SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
						? addBindingOfUnknownType : bindingAddFunctions[static_cast<size_t>(bindingInfo.type)];
					return addBinding(self, stages, bindingInfo);
				}

				inline static bool AddBindingSet(BindingSetBuilder* self, PipelineStageMask stages, const SPIRV_Binary::BindingSetInfo& setInfo) {
					const size_t setBindingCount = setInfo.BindingCount();
					for (size_t i = 0u; i < setBindingCount; i++)
						if (!AddBinding(self, stages, setInfo.Binding(i)));
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
				, m_bindingSetInfos(std::move(builder.m_bindingSetInfos))
				, m_pipelineLayout(builder.m_pipelineLayout) {
				if (builder.m_failed || m_pipelineLayout == VK_NULL_HANDLE)
					m_device->Log()->Fatal(
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
				if (shader == nullptr) return true;
				
				if (m_failed) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::IncludeShaderBindings - Binding set has already failed! ",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				if (m_pipelineLayout != VK_NULL_HANDLE) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::IncludeShaderBindings - BindingSetBuilder::Finish() already invoked; ",
						"additional shaders can not be included! [File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				
				const size_t shaderSetCount = shader->BindingSetCount();
				const PipelineStageMask shaderStages = shader->ShaderStages();
				for (size_t i = 0u; i < shaderSetCount; i++)
					if (!Helpers::AddBindingSet(this, shaderStages, shader->BindingSet(i))) {
						m_failed = true;
						return false;
					}
				return (!m_failed);
			}

			bool VulkanPipeline::BindingSetBuilder::Finish() {
				if (m_pipelineLayout != VK_NULL_HANDLE) return true;
				if (m_failed) {
					m_device->Log()->Error(
						"VulkanPipeline::BindingSetBuilder::Finish - Binding set has failed on previous call(s)! ",
						"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				// __TODO__: Implement this crap!
				m_device->Log()->Error(
					"VulkanPipeline::BindingSetBuilder::Finish - Not yet implemented! ",
					"[File:", __FILE__, "; Line: ", __LINE__, "]");
				m_failed = true;
				return false;
			}

			bool VulkanPipeline::BindingSetBuilder::Failed()const { return m_failed; }
		}
		}
	}
}
