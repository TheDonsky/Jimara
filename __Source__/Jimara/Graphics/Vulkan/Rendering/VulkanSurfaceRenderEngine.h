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
			/// <summary>
			/// VulkanRenderEngine that renders to a VulkanWindowSurface
			/// </summary>
			class VulkanSurfaceRenderEngine : public VulkanRenderEngine {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" Vulkan device </param>
				/// <param name="surface"> Target window surface </param>
				VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanSurfaceRenderEngine();

				/// <summary> Renders to window </summary>
				virtual void Update() override;

				/// <summary>
				/// Adds an ImageRenderer to the engine (has to be of a VulkanImageRenderer type)
				/// </summary>
				/// <param name="renderer"> Renderer to add </param>
				virtual void AddRenderer(ImageRenderer* renderer) override;

				/// <summary>
				/// Removes an image renderere for the engine
				/// </summary>
				/// <param name="renderer"> Renderer to remove </param>
				virtual void RemoveRenderer(ImageRenderer* renderer) override;



			private:
				// VulkanSurfaceRenderEngine information provider
				class EngineInfo : public VulkanRenderEngineInfo {
				public:
					EngineInfo(VulkanSurfaceRenderEngine* engine);

					virtual GraphicsDevice* Device()const override;

					virtual Size2 TargetSize()const override;

					size_t ImageCount()const override;

					virtual VulkanImage* Image(size_t imageId)const override;

					virtual VkFormat ImageFormat()const override;

					VkSampleCountFlagBits MSAASamples(GraphicsSettings::MSAA desired)const override;

				private:
					VulkanSurfaceRenderEngine* m_engine;
				} m_engineInfo;

				// Command pool for main render commands
				VulkanCommandPool m_commandPool;
				
				// Target window surface
				Reference<VulkanWindowSurface> m_windowSurface;

				// Window surface swap chain
				Reference<VulkanSwapChain> m_swapChain;
				
				// Image availability synchronisation objects
				std::vector<VulkanSemaphore> m_imageAvailableSemaphores;

				// Render completion synchronisation objects
				std::vector<VulkanSemaphore> m_renderFinishedSemaphores;

				// In-flight frame fences
				std::vector<VulkanFence> m_inFlightFences;

				// Current semaphore index
				size_t m_semaphoreIndex;

				// True, if swap chain gets invalidated
				bool m_shouldRecreateComponents;

				// Main command buffers
				std::vector<VkCommandBuffer> m_mainCommandBuffers;

				// Command recorder
				class Recorder : public VulkanCommandRecorder {
				public:
					size_t imageIndex;
					VulkanImage* image;
					VkCommandBuffer commandBuffer;
					VulkanCommandPool* commandPool;
					std::vector<Reference<Object>> dependencies;
					std::vector<VkSemaphore> semaphoresToWaitFor;
					std::vector<VkSemaphore> semaphoresToSignal;

					inline Recorder() : imageIndex(0), image(nullptr), commandBuffer(VK_NULL_HANDLE), commandPool(nullptr) {}

					inline virtual size_t CommandBufferIndex()const override { return imageIndex; }

					inline virtual VkCommandBuffer CommandBuffer()const override { return commandBuffer; }

					inline virtual void RecordBufferDependency(Object* object)override { dependencies.push_back(object); }

					inline virtual void WaitForSemaphore(VkSemaphore semaphore)override { semaphoresToWaitFor.push_back(semaphore); }

					inline virtual void SignalSemaphore(VkSemaphore semaphore)override { semaphoresToSignal.push_back(semaphore); }

					inline virtual VulkanCommandPool* CommandPool()const override { return commandPool; }
				};
				// Per-frame command recorders
				std::vector<Recorder> m_commandRecorders;

				// Lock for renderer collections
				std::recursive_mutex m_rendererLock;

				// Renderer to engine data index map
				std::unordered_map<VulkanImageRenderer*, size_t> m_rendererIndexes;

				// Renderer data
				std::vector<Reference<VulkanImageRenderer::EngineData>> m_rendererData;

				// Recreates swap chain and dependent objects
				void RecreateComponents();

				// Surface size change callback
				void SurfaceSizeChanged(VulkanWindowSurface*);
			};
		}
	}
}
