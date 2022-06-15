#pragma once
#include "../VulkanDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Simple wrapper on top of VkShaderModule
			/// </summary>
			class JIMARA_API VulkanShader : public virtual Shader {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="shaderModule"> Shader bytecode </param>
				VulkanShader(VulkanDevice* device, const SPIRV_Binary* binary);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanShader();

				/// <summary> Type cast to API object </summary>
				operator VkShaderModule()const;

			private:
				// "Owner" device
				Reference<VulkanDevice> m_device;

				// Shader module
				VkShaderModule m_shaderModule;
			};
		}
	}
}
