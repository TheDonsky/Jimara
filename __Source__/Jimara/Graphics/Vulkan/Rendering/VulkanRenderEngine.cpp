#include "VulkanRenderEngine.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanImageRenderer::EngineData::EngineData(VulkanImageRenderer* renderer, VulkanRenderEngineInfo* engineInfo) 
				: m_renderer(renderer), m_engineInfo(engineInfo) {}

			VulkanImageRenderer* VulkanImageRenderer::EngineData::Renderer()const {
				return m_renderer;
			}

			VulkanRenderEngineInfo* VulkanImageRenderer::EngineData::EngineInfo()const {
				return m_engineInfo;
			}

			void VulkanImageRenderer::EngineData::Render(VulkanRenderEngine::CommandRecorder* commandRecorder) {
				m_renderer->Render(this, commandRecorder);
			}

			VulkanDevice* VulkanRenderEngine::Device()const { 
				return m_device; 
			}

			VulkanRenderEngine::VulkanRenderEngine(VulkanDevice* device)
				: m_device(device) {}
		}
	}
}
