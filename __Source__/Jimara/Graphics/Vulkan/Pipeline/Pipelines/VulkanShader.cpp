#include "VulkanShader.h"

#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			Reference<VulkanShader> VulkanShader::Create(VulkanDevice* device, const SPIRV_Binary* binary) {
				if (device == nullptr) 
					return nullptr;
				VkShaderModuleCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.codeSize = binary->BytecodeSize();
					createInfo.pCode = reinterpret_cast<const uint32_t*>(binary->Bytecode());
				}
				VkShaderModule shaderModule = VK_NULL_HANDLE;
				if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
					device->Log()->Error("VulkanShader::Create - Failed to create shader module! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				const Reference<VulkanShader> shader = new VulkanShader(device, shaderModule);
				shader->ReleaseRef();
				return shader;
			}

			VulkanShader::VulkanShader(VulkanDevice* device, VkShaderModule shaderModule) 
				: m_device(device), m_shaderModule(shaderModule) {
				assert(m_device != nullptr);
				assert(m_shaderModule != VK_NULL_HANDLE);
			}

			VulkanShader::~VulkanShader() {
				vkDestroyShaderModule(*m_device, m_shaderModule, nullptr);
			}

			VulkanShader::operator VkShaderModule()const { return m_shaderModule; }
		}
	}
}
#pragma warning(default: 26812)
