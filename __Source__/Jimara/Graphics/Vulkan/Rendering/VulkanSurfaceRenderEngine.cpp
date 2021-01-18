#include "VulkanSurfaceRenderEngine.h"
#define MAX_FRAMES_IN_FLIGHT ((size_t)2)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanSurfaceRenderEngine::VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface) 
				: m_commandPool(device), m_windowSurface(surface)
				, m_semaphoreIndex(0)
				, m_shouldRecreateComponents(false) {
				RecreateComponents();
				m_windowSurface->OnSizeChanged() += Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
			}

			VulkanSurfaceRenderEngine::~VulkanSurfaceRenderEngine() {
				m_windowSurface->OnSizeChanged() -= Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
				vkDeviceWaitIdle(*m_commandPool.Device());
				// __TODO__: Dispose underlying renderers

				m_commandPool.DestroyCommandBuffers(m_mainCommandBuffers);
			}

			void VulkanSurfaceRenderEngine::Update() {
				std::unique_lock<std::mutex> resizeLock(m_windowSurface->ResizeLock());

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
				
				VkCommandBuffer mainCommandBuffer = m_mainCommandBuffers[imageId];

				{
					if (vkResetCommandBuffer(mainCommandBuffer, 0) != VK_SUCCESS)
						m_commandPool.Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to reset command buffer");
					{
						VkCommandBufferBeginInfo beginInfo = {};
						beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
						beginInfo.flags = 0; // Optional
						beginInfo.pInheritanceInfo = nullptr; // Optional
						if (vkBeginCommandBuffer(mainCommandBuffer, &beginInfo) != VK_SUCCESS)
							m_commandPool.Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to begin recording command buffer!");
					}

					if (vkEndCommandBuffer(mainCommandBuffer) != VK_SUCCESS)
						m_commandPool.Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to end command buffer!");
				}

				{
					VkSubmitInfo submitInfo = {};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

					// Wait for image availability
					VkSemaphore imageAvailable = imageAvailableSemaphore;
					submitInfo.waitSemaphoreCount = 1;
					submitInfo.pWaitSemaphores = &imageAvailable;
					
					// Wait for semaphores when we need to access color attachment
					VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
					submitInfo.pWaitDstStageMask = waitStages;

					// Command buffer to submit
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &mainCommandBuffer;

					// Semaphores to signal
					VkSemaphore renderFinished = renderFinishedSemaphore;
					submitInfo.signalSemaphoreCount = 1;
					submitInfo.pSignalSemaphores = &renderFinished;

					if (vkQueueSubmit(m_commandPool.Queue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
						m_commandPool.Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to submit draw command buffer!");
				}

				// Present rendered image
				if (!m_swapChain->Present(imageId, renderFinishedSemaphore)) m_shouldRecreateComponents = true;
				if (m_shouldRecreateComponents) RecreateComponents();
			}

			void VulkanSurfaceRenderEngine::RecreateComponents() {
				// __TODO__: Notify underlying renderers that swap chain got invalidated

				m_shouldRecreateComponents = false;

				m_swapChain = nullptr;
				m_swapChain = Object::Instantiate<VulkanSwapChain>(m_commandPool.Device(), m_windowSurface);

				// Temporary...
				m_commandPool.SubmitSingleTimeCommandBuffer([&](VkCommandBuffer buffer) {
					for (size_t i = 0; i < m_swapChain->ImageCount(); i++) {
						VulkanImage* image = m_swapChain->Image(i);
						VkImageMemoryBarrier barrier = {};
						barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

						barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

						barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

						barrier.image = *image;

						barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						barrier.subresourceRange.baseMipLevel = 0;
						barrier.subresourceRange.levelCount = 1;
						barrier.subresourceRange.baseArrayLayer = 0;
						barrier.subresourceRange.layerCount = 1;

						barrier.srcAccessMask = 0;
						barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

						vkCmdPipelineBarrier(buffer
							, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0
							, 0, nullptr, 0, nullptr, 1, &barrier);
					}
					});

				const size_t maxFramesInFlight = min(MAX_FRAMES_IN_FLIGHT, m_swapChain->ImageCount());
				while (m_imageAvailableSemaphores.size() < maxFramesInFlight) {
					m_imageAvailableSemaphores.push_back(VulkanSemaphore(m_commandPool.Device()));
					m_renderFinishedSemaphores.push_back(VulkanSemaphore(m_commandPool.Device()));
				}
				m_imageAvailableSemaphores.resize(maxFramesInFlight);
				m_renderFinishedSemaphores.resize(maxFramesInFlight);

				while (m_inFlightFences.size() < m_swapChain->ImageCount())
					m_inFlightFences.push_back(VulkanFence(m_commandPool.Device(), true));

				m_commandPool.DestroyCommandBuffers(m_mainCommandBuffers);
				m_mainCommandBuffers = m_commandPool.CreateCommandBuffers(m_swapChain->ImageCount());

				// __TODO__: Notify underlying renderers that we've got a new swap chain
			}

			void VulkanSurfaceRenderEngine::SurfaceSizeChanged(VulkanWindowSurface*) {
				m_shouldRecreateComponents = true;
			}
		}
	}
}
