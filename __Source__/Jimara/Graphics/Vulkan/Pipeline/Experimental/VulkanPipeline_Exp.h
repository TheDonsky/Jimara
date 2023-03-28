#pragma once
#include "../../VulkanDevice.h"
#include "../../../Pipeline/Experimental/Pipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			using Pipeline = Graphics::Experimental::Pipeline;

			class JIMARA_API VulkanPipeline : public virtual Pipeline {
			public:
				class BindingSetBuilder;

				struct JIMARA_API BindingInfo final {
					size_t binding = 0u;
					SPIRV_Binary::BindingInfo::Type type = SPIRV_Binary::BindingInfo::Type::UNKNOWN;
					PipelineStageMask stageMask = 0u;
					Stacktor<std::string_view, 1u> nameAliases;
				};

				using SetBindingInfos = Stacktor<BindingInfo, 4u>;

				struct JIMARA_API DescriptorSetInfo final {
					SetBindingInfos bindings;
					VkDescriptorSetLayout layout = VK_NULL_HANDLE;
				};

				virtual ~VulkanPipeline();

				inline VulkanDevice* Device()const { return m_device; }

				inline virtual size_t BindingSetCount()const override { return m_bindingSetInfos.Size(); }

				inline const DescriptorSetInfo& BindingSetInfo(size_t index)const { return m_bindingSetInfos[index]; }

				inline VkPipelineLayout PipelineLayout()const { return m_pipelineLayout; }

			protected:
				VulkanPipeline(BindingSetBuilder&& builder);

			private:
				using BindingSetInfos = Stacktor<DescriptorSetInfo, 4u>;

				const Reference<VulkanDevice> m_device;
				Stacktor<Reference<const SPIRV_Binary>> m_shaders;
				const BindingSetInfos m_bindingSetInfos;
				const VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

				struct Helpers;
			};

			class VulkanPipeline::BindingSetBuilder {
			public:
				BindingSetBuilder(VulkanDevice* device);

				virtual ~BindingSetBuilder();

				bool IncludeShaderBindings(const SPIRV_Binary* shader);

				bool Finish();

				bool Failed()const;

				inline VkPipelineLayout PipelineLayout()const { return m_pipelineLayout; }

			private:
				const Reference<VulkanDevice> m_device;
				Stacktor<Reference<const SPIRV_Binary>> m_shaders;
				BindingSetInfos m_bindingSetInfos;
				VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
				bool m_failed = false;

				friend struct Helpers;
				friend class VulkanPipeline;
			};
		}
		}
	}
}

