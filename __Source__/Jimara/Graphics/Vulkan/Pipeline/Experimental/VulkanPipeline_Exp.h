#pragma once
#include "../../VulkanDevice.h"
#include "../../../Pipeline/Experimental/Pipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
		namespace Experimental {
			/// <summary>
			/// Pipeline implementation for Vulkan API
			/// (Basically, this one is a wrapper on top of VkPipelineLayout)
			/// </summary>
			class JIMARA_API VulkanPipeline : public virtual Pipeline {
			public:
				/// <summary> Vulkan pipelines are built using BindingSetBuilder </summary>
				class BindingSetBuilder;

				/// <summary> Basic information about a simple binding </summary>
				struct JIMARA_API BindingInfo final {
					/// <summary> Binding slot within a binding set </summary>
					size_t binding = 0u;

					/// <summary> Binding resource type </summary>
					SPIRV_Binary::BindingInfo::Type type = SPIRV_Binary::BindingInfo::Type::UNKNOWN;

					/// <summary> Pipeline stages, this binding is used in </summary>
					PipelineStageMask stageMask = PipelineStageMask::NONE;

					/// <summary> List of different names the same binding is used with </summary>
					Stacktor<std::string_view, 1u> nameAliases;
				};

				/// <summary> Bindings from a single binding set </summary>
				using SetBindingInfos = Stacktor<BindingInfo, 4u>;

				/// <summary>
				/// Bindings from a single binding set alongside a descriptor set layout object
				/// </summary>
				struct JIMARA_API DescriptorSetInfo final {
					/// <summary> Bindings from the binding set </summary>
					SetBindingInfos bindings;

					/// <summary> Descriptor set layout for the binding set </summary>
					VkDescriptorSetLayout layout = VK_NULL_HANDLE;
				};

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanPipeline();

				/// <summary> Graphics device </summary>
				inline VulkanDevice* Device()const { return m_device; }

				/// <summary>
				/// Number of bound descriptor sets used by pipeline during execution.
				/// <para/> Note: Compatible binding sets are allocated through BindingPool's interface 
				///		and they should be updated and bound manually.
				/// </summary>
				inline virtual size_t BindingSetCount()const override { return m_bindingSetInfos.Size(); }

				/// <summary>
				/// Binding set information per binding set index
				/// </summary>
				/// <param name="index"> Binding set index </param>
				/// <returns> Bindings from a single binding set alongside a descriptor set layout object </returns>
				inline const DescriptorSetInfo& BindingSetInfo(size_t index)const { return m_bindingSetInfos[index]; }

				/// <summary> Vulkan pipeline layout </summary>
				inline VkPipelineLayout PipelineLayout()const { return m_pipelineLayout; }


			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="builder"> Move-reference of a BindingSetBuilder that has successfully Finish()-ed building underlying resources </param>
				VulkanPipeline(BindingSetBuilder&& builder);

			private:
				// Device
				const Reference<VulkanDevice> m_device;

				// Used shaders
				using ShaderList = Stacktor<Reference<const SPIRV_Binary>, 4u>;
				ShaderList m_shaders;

				// Binding set information
				using BindingSetInfos = Stacktor<DescriptorSetInfo, 4u>;
				const BindingSetInfos m_bindingSetInfos;

				// Pipeline layout
				const VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

				// Private stuff
				struct Helpers;
			};

			/// <summary> Vulkan pipelines are built using BindingSetBuilder </summary>
			class VulkanPipeline::BindingSetBuilder {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Vulkan device </param>
				BindingSetBuilder(VulkanDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~BindingSetBuilder();

				/// <summary>
				/// Includes binding sets from given shader binary
				/// </summary>
				/// <param name="shader"> Shader to include </param>
				/// <returns> False, if something fails (false means that the builder is invalidated and can not be used to create a valid pipeline) </returns>
				bool IncludeShaderBindings(const SPIRV_Binary* shader);

				/// <summary>
				/// Builds VkDescriptorSetLayout and VkPipelineLayout objects based on the shaders previously included using IncludeShaderBindings call
				/// <para/> Note: After this call, adding more shaders with IncludeShaderBindings() is not allowed
				/// </summary>
				/// <returns> False, if something fails (false means that the builder is invalidated and can not be used to create a valid pipeline) </returns>
				bool Finish();

				/// <summary>
				/// True, if any of the previous IncludeShaderBindings() or Finish() calls failed
				/// <para/> If BindingSetBuilder enters 'failed' state, it can not be used to build VulkanPipeline
				/// </summary>
				bool Failed()const;

				/// <summary>
				/// Pipeline layout
				/// <para/> Note: Available only after a successful Finish() call
				/// </summary>
				inline VkPipelineLayout PipelineLayout()const { return m_pipelineLayout; }

			private:
				// Device
				const Reference<VulkanDevice> m_device;

				// Used shaders
				ShaderList m_shaders;

				// Binding set information
				BindingSetInfos m_bindingSetInfos;

				// Pipeline layout
				VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

				// Becomes true, if anything fails
				bool m_failed = false;

				// Helpers and VulkanPipeline can access the internals, because they are good friends...
				friend struct Helpers;
				friend class VulkanPipeline;
			};
		}
		}
	}
}
