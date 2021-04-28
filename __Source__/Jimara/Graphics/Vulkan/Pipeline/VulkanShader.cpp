#include "VulkanShader.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanShader::VulkanShader(VulkanDevice* device, const SPIRV_Binary* binary) 
				: m_device(device), m_shaderModule(VK_NULL_HANDLE) {
				if (m_device == nullptr) {
					m_device->Log()->Fatal("VulkanShader - [internal error] Device not of the correct type!");
					return;
				}
				VkShaderModuleCreateInfo createInfo = {};
				{
					createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					createInfo.codeSize = binary->BytecodeSize();
					createInfo.pCode = reinterpret_cast<const uint32_t*>(binary->Bytecode());
				}
				if (vkCreateShaderModule(*device, &createInfo, nullptr, &m_shaderModule) != VK_SUCCESS) {
					device->Log()->Fatal("VulkanShader - Failed to create shader module!");
					m_shaderModule = VK_NULL_HANDLE;
				}
			}

			VulkanShader::~VulkanShader() {
				if (m_shaderModule != VK_NULL_HANDLE) {
					vkDestroyShaderModule(*m_device, m_shaderModule, nullptr);
					m_shaderModule = VK_NULL_HANDLE;
				}
			}

			VulkanShader::operator VkShaderModule()const { return m_shaderModule; }
		}
	}
}
