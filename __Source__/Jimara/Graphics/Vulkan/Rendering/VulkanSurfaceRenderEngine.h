#pragma once
#include "VulkanRenderEngine.h"
#include "VulkanRenderSurface.h"
#include "VulkanSwapChain.h"
#include "../Synch/VulkanSemaphore.h"
#include "../Synch/VulkanFence.h"
#include "../Synch/VulkanTimelineSemaphore.h"
#include "../Pipeline/Commands/VulkanCommandBuffer.h"
#include <unordered_map>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// VulkanRenderEngine that renders to a VulkanWindowSurface
			/// </summary>
			class JIMARA_API VulkanSurfaceRenderEngine : public VulkanRenderEngine {
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
				class EngineInfo : public RenderEngineInfo {
				public:
					EngineInfo(VulkanSurfaceRenderEngine* engine);

					virtual GraphicsDevice* Device()const override;

					virtual Size2 ImageSize()const override;

					virtual Texture::PixelFormat ImageFormat()const override;

					virtual size_t ImageCount()const override;

					virtual Texture* Image(size_t imageId)const override;

				private:
					VulkanSurfaceRenderEngine* m_engine;
				} m_engineInfo;

				// Command pool for main render commands
				Reference<VulkanCommandPool> m_commandPool;
				
				// Target window surface
				Reference<VulkanWindowSurface> m_windowSurface;

				// Window surface swap chain
				Reference<VulkanSwapChain> m_swapChain;
				std::vector<Reference<Texture>> m_swapChainImages;
				
				// Image availability synchronisation objects
				std::vector<Reference<VulkanSemaphore>> m_freeSemaphores;

				// True, if swap chain gets invalidated
				bool m_shouldRecreateComponents;

				// Main command buffers
				struct SubmittedCommandBuffer {
					Reference<PrimaryCommandBuffer> commandBuffer;
					Reference<VulkanSemaphore> imageAvailableSemaphore;
					Reference<VulkanSemaphore> renderFinishedSemaphore;
				};
				std::vector<SubmittedCommandBuffer> m_mainCommandBuffers;

				// Lock for renderer collections
				std::recursive_mutex m_rendererLock;

				// Renderer to engine data index map
				std::unordered_map<ImageRenderer*, size_t> m_rendererIndexes;

				// Renderer data
				std::vector<std::pair<Reference<ImageRenderer>, Reference<Object>>> m_rendererData;

				// Recreates swap chain and dependent objects
				void RecreateComponents();

				// Surface size change callback
				void SurfaceSizeChanged(VulkanWindowSurface*);
			};
		}
	}
}
