#pragma once
#include "../VulkanDevice.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanShaderCache;

			/// <summary>
			/// Simple wrapper on top of VkShaderModule
			/// </summary>
			class VulkanShader : public virtual Shader {
			public:
				/// <summary> Virtual destructor </summary>
				virtual ~VulkanShader();

				/// <summary> Type cast to API object </summary>
				operator VkShaderModule()const;

			private:
				/// <summary>
				/// Constructor (normally, accessed only by VulkanShaderCache)
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				/// <param name="shaderModule"> Shader module </param>
				VulkanShader(VulkanDevice* device, VkShaderModule shaderModule);

				// "Owner" device
				Reference<VulkanDevice> m_device;

				// Shader module
				VkShaderModule m_shaderModule;

				// VulkanShaderCache should be able to invoke the constructor
				friend class VulkanShaderCache;
			};


			/// <summary>
			/// Vulkan-backed shader cache
			/// </summary>
			class VulkanShaderCache : public ShaderCache {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" device </param>
				VulkanShaderCache(VulkanDevice* device);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanShaderCache();

			protected:
				/// <summary>
				/// Instantiates a shader module
				/// </summary>
				/// <param name="data"> Shader file data </param>
				/// <param name="bytes"> Number of bytes within data </param>
				/// <returns> New shader instance </returns>
				virtual Shader* CreateShader(char* data, size_t bytes) override;
			};
		}
	}
}
