#pragma once
#include "VulkanRenderEngine.h"
#include "VulkanRenderSurface.h"
#include "VulkanSwapChain.h"
#include "../Synch/VulkanSemaphore.h"
#include "../Synch/VulkanFence.h"
#include "../Pipeline/VulkanCommandPool.h"
#include <unordered_map>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanSurfaceRenderEngine : public VulkanRenderEngine {
			public:
				VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface);

				virtual ~VulkanSurfaceRenderEngine();

				virtual void Update() override;

				virtual void AddRenderer(ImageRenderer* renderer) override;

				virtual void RemoveRenderer(ImageRenderer* renderer) override;



			private:
				class EngineInfo : public VulkanRenderEngineInfo {
				public:
					EngineInfo(VulkanSurfaceRenderEngine* engine);

					virtual GraphicsDevice* Device()const override;

					virtual glm::uvec2 TargetSize()const override;

					size_t ImageCount()const override;

					virtual VulkanImage* Image(size_t imageId)const override;

					VkSampleCountFlagBits MSAASamples(GraphicsSettings::MSAA desired)const override;

				private:
					VulkanSurfaceRenderEngine* m_engine;
				} m_engineInfo;

				VulkanCommandPool m_commandPool;
				Reference<VulkanWindowSurface> m_windowSurface;

				Reference<VulkanSwapChain> m_swapChain;
				
				std::vector<VulkanSemaphore> m_imageAvailableSemaphores;
				std::vector<VulkanSemaphore> m_renderFinishedSemaphores;
				std::vector<VulkanFence> m_inFlightFences;

				size_t m_semaphoreIndex;

				bool m_shouldRecreateComponents;

				std::vector<VkCommandBuffer> m_mainCommandBuffers;
				class Recorder : public VulkanRenderEngine::CommandRecorder {
				public:
					size_t imageIndex;
					VulkanImage* image;
					VkCommandBuffer commandBuffer;
					std::vector<Reference<Object>> dependencies;

					inline Recorder() : imageIndex(0), image(nullptr), commandBuffer(VK_NULL_HANDLE) {}

					inline virtual size_t ImageIndex()const override { return imageIndex; }

					inline virtual VulkanImage* Image()const override { return image; }

					inline virtual VkCommandBuffer CommandBuffer()const override { return commandBuffer; }

					inline virtual void RecordBufferDependency(Object* object)override { dependencies.push_back(object); }
				};
				std::vector<Recorder> m_commandRecorders;

				std::recursive_mutex m_rendererLock;
				std::unordered_map<VulkanImageRenderer*, size_t> m_rendererIndexes;
				std::vector<Reference<VulkanImageRenderer::EngineData>> m_rendererData;

				void RecreateComponents();

				void SurfaceSizeChanged(VulkanWindowSurface*);
			};
		}
	}
}
