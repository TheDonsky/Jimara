#include "VulkanSurfaceRenderEngine.h"
#define MAX_FRAMES_IN_FLIGHT ((size_t)2)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanSurfaceRenderEngine::VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface) 
				: m_device(device), m_windowSurface(surface)
				, m_semaphoreIndex(0)
				, m_shouldRecreateComponents(false) {
				RecreateComponents();
				m_windowSurface->OnSizeChanged() += Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
			}

			VulkanSurfaceRenderEngine::~VulkanSurfaceRenderEngine() {
				m_windowSurface->OnSizeChanged() -= Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
				vkDeviceWaitIdle(*m_device);
				// __TODO__: Dispose underlying renderers
			}

			void VulkanSurfaceRenderEngine::Update() {
				// Semaphores we will be using for frame synchronisation:
				VulkanSemaphore& imageAvailableSemaphore = m_imageAvailableSemaphores[m_semaphoreIndex];
				VulkanSemaphore& renderFinishedSemaphore = m_renderFinishedSemaphores[m_semaphoreIndex];

				// We need to get the target image:
				size_t imageId;
				VulkanImage* targetImage;
				while (true) {
					// If the surface size is 0, there's no need to render anything to it
					glm::uvec2 size = m_windowSurface->Size();
					if (size.x <= 0 || size.y <= 0) return;

					// We need to recreate components if our swap chain is no longer valid
					if (m_swapChain->AquireNextImage(imageAvailableSemaphore, imageId, targetImage)) break;
					else RecreateComponents();
				}

				// Image is squired, so we just need to wait for it finish being presented in case it's still being rendered on
				VulkanFence& inFlightFence = m_inFlightFences[imageId];
				inFlightFence.WaitAndReset();
				m_semaphoreIndex = (m_semaphoreIndex + 1) % m_imageAvailableSemaphores.size();



				// Present rendered image
				if (m_swapChain->Present(imageId, /*renderFinishedSemaphore*/ VK_NULL_HANDLE)) m_shouldRecreateComponents = true;
				if (m_shouldRecreateComponents) RecreateComponents();
			}

			void VulkanSurfaceRenderEngine::RecreateComponents() {
				// __TODO__: Notify underlying renderers that swap chain got invalidated

				m_shouldRecreateComponents = false;

				m_swapChain = nullptr;
				m_swapChain = Object::Instantiate<VulkanSwapChain>(m_device, m_windowSurface);

				const size_t maxFramesInFlight = min(MAX_FRAMES_IN_FLIGHT, m_swapChain->ImageCount());
				while (m_imageAvailableSemaphores.size() < maxFramesInFlight) {
					m_imageAvailableSemaphores.push_back(VulkanSemaphore(m_device));
					m_renderFinishedSemaphores.push_back(VulkanSemaphore(m_device));
				}
				m_imageAvailableSemaphores.resize(maxFramesInFlight);
				m_renderFinishedSemaphores.resize(maxFramesInFlight);

				while (m_inFlightFences.size() < m_swapChain->ImageCount())
					m_inFlightFences.push_back(VulkanFence(m_device, true));

				// __TODO__: Notify underlying renderers that we've got a new swap chain
			}

			void VulkanSurfaceRenderEngine::SurfaceSizeChanged(VulkanWindowSurface*) {
				m_shouldRecreateComponents = true;
			}
		}
	}
}
