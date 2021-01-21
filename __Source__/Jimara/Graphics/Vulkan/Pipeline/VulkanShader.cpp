#include "VulkanShader.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanShader::~VulkanShader() {
				if (m_shaderModule != VK_NULL_HANDLE) {
					vkDestroyShaderModule(*m_device, m_shaderModule, nullptr);
					m_shaderModule = VK_NULL_HANDLE;
				}
			}

			VulkanShader::operator VkShaderModule()const {
				return m_shaderModule;
			}

			VulkanShader::VulkanShader(VulkanDevice* device, VkShaderModule shaderModule)
				: m_device(device), m_shaderModule(shaderModule) {}


			VulkanShaderCache::VulkanShaderCache(VulkanDevice* device) 
				: ShaderCache(device) {}

			VulkanShaderCache::~VulkanShaderCache() {}

			Shader* VulkanShaderCache::CreateShader(char* data, size_t bytes) {
				VulkanDevice* device = dynamic_cast<VulkanDevice*>(Device());
				if (device == nullptr) {
					Device()->Log()->Fatal("VulkanShaderCache - [internal error] Device not of the correct type");
					return nullptr;
				}
				VkShaderModuleCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.codeSize = bytes;
					createInfo.pCode = reinterpret_cast<const uint32_t*>(data);
				}
				VkShaderModule shaderModule;
				if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
					device->Log()->Fatal("VulkanShaderCache - Failed to create shader module!");
					return nullptr;
				}
				else return new VulkanShader(device, shaderModule);
			}
		}
	}
}
