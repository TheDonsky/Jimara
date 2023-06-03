#pragma once
#include "../../VulkanDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Simple wrapper on top of VkShaderModule
			/// </summary>
			class JIMARA_API VulkanShader : public virtual Object {
			public:
				/// <summary>
				/// Creates vulkan shader module
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="shaderModule"> Shader bytecode </param>
				static Reference<VulkanShader> Create(VulkanDevice* device, const SPIRV_Binary* binary);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanShader();

				/// <summary> Type cast to API object </summary>
				operator VkShaderModule()const;

			private:
				// "Owner" device
				const Reference<VulkanDevice> m_device;

				// Shader module
				const VkShaderModule m_shaderModule;

				// Actual constructor is private
				VulkanShader(VulkanDevice* device, VkShaderModule shaderModule);
			};
		}
	}
}
