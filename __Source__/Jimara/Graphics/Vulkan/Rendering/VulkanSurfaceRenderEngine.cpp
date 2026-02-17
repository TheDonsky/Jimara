#include "VulkanSurfaceRenderEngine.h"
#pragma warning(disable: 26812)
#define MAX_FRAMES_IN_FLIGHT ((size_t)3)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanSurfaceRenderEngine::VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface) 
				: VulkanRenderEngine(device)
				, m_engineInfo(this), m_commandPool(device->GraphicsQueue()->CreateCommandPool())
				, m_windowSurface(surface)
				, m_shouldRecreateComponents(false) {
				RecreateComponents();
				m_windowSurface->OnSizeChanged() += Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
			}

			VulkanSurfaceRenderEngine::~VulkanSurfaceRenderEngine() {
				m_windowSurface->OnSizeChanged() -= Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
				for (size_t i = 0; i < m_mainCommandBuffers.size(); i++)
					m_mainCommandBuffers[i].commandBuffer->Wait();
				m_swapChainImages.clear();
				m_swapChain = nullptr;
				m_mainCommandBuffers.clear();
				m_rendererIndexes.clear();
				m_rendererData.clear();
			}

			void VulkanSurfaceRenderEngine::Update() {
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				std::shared_lock<std::shared_mutex> resizeLock(m_windowSurface->ResizeLock());

				// If the surface size is 0, there's no need to render anything to it
				{
					Size2 size = m_windowSurface->Size();
					if (size.x <= 0 || size.y <= 0) 
						return;
				}

				// Semaphores we will be using for frame synchronisation:
				bool semaphoreIsNew = true;
				auto getFreeSemaphore = [&]() -> Reference<VulkanSemaphore> {
					if (m_freeSemaphores.size() <= 0u)
						return Object::Instantiate<VulkanSemaphore>(Device());
					const Reference<VulkanSemaphore> rv = m_freeSemaphores.back();
					m_freeSemaphores.pop_back();
					semaphoreIsNew = false;
					return rv;
				};
				const Reference<VulkanSemaphore> imageAvailableSemaphore = getFreeSemaphore();
				const Reference<VulkanSemaphore> renderFinishedSemaphore = getFreeSemaphore();

				// We need to get the target image:
				size_t imageId;
				VulkanImage* targetImage;
				while (true) {
					// We need to recreate components if our swap chain is no longer valid
					if (m_swapChain->AquireNextImage(*imageAvailableSemaphore, imageId, targetImage)) 
						break;
					RecreateComponents();
				}

				// Prepare recorder:
				SubmittedCommandBuffer& submission = m_mainCommandBuffers[imageId];
				VulkanPrimaryCommandBuffer* commandBuffer = Reference<VulkanPrimaryCommandBuffer>(submission.commandBuffer);
				{
					commandBuffer->Reset();
					if (submission.renderFinishedSemaphore != nullptr)
						m_freeSemaphores.push_back(submission.renderFinishedSemaphore);
					if (submission.imageAvailableSemaphore != nullptr)
						m_freeSemaphores.push_back(submission.imageAvailableSemaphore);
					commandBuffer->BeginRecording();
					commandBuffer->WaitForSemaphore(imageAvailableSemaphore,
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
						VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
						VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
						VK_PIPELINE_STAGE_TRANSFER_BIT |
						(Device()->PhysicalDevice()->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING) 
							? VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR : 0));
					commandBuffer->SignalSemaphore(renderFinishedSemaphore);
					submission.imageAvailableSemaphore = imageAvailableSemaphore;
					submission.renderFinishedSemaphore = renderFinishedSemaphore;
				}

				// Record command buffer:
				{
					// Transition to shader read only optimal layout
					VulkanImage* const image = m_swapChain->Image(imageId);
					image->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, image->ShaderAccessLayout(), 0, 1, 0, 1);

					// Let all underlying renderers record their commands
					const InFlightBufferInfo BUFFER_INFO(commandBuffer, imageId);
					for (size_t i = 0; i < m_rendererData.size(); i++) {
						std::pair<Reference<ImageRenderer>, Reference<Object>>& rendererData = m_rendererData[i];
						rendererData.first->Render(rendererData.second, BUFFER_INFO);
					}

					// Copy image content if swap chain image is not the same as the render target
					if (m_swapChainImages[imageId] != image)
						image->Copy(commandBuffer, m_swapChainImages[imageId]);

					// Transition to present layout
					image->TransitionLayout(commandBuffer, image->ShaderAccessLayout(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, 0, 1);

					// End command buffer
					commandBuffer->EndRecording();
				}

				// Submit command buffer:
				Device()->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);

				// Present rendered image
				if (!m_swapChain->Present(imageId, *renderFinishedSemaphore)) 
					m_shouldRecreateComponents = true;
				if (m_shouldRecreateComponents) 
					RecreateComponents();
			}

			void VulkanSurfaceRenderEngine::AddRenderer(ImageRenderer* renderer) {
				if (renderer == nullptr) return;
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				if (m_rendererIndexes.find(renderer) != m_rendererIndexes.end()) return;

				Reference<Object> engineData = renderer->CreateEngineData(&m_engineInfo);
				m_rendererIndexes.insert(std::make_pair(renderer, m_rendererData.size()));
				m_rendererData.push_back(std::pair<Reference<ImageRenderer>, Reference<Object>>(renderer, engineData));
			}

			void VulkanSurfaceRenderEngine::RemoveRenderer(ImageRenderer* renderer) {
				if (renderer == nullptr) return;
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				
				std::unordered_map<ImageRenderer*, size_t>::const_iterator it = m_rendererIndexes.find(renderer);
				if (it == m_rendererIndexes.end()) return;

				size_t index = it->second;

				size_t lastIndex = m_rendererData.size() - 1;
				if (it->second < lastIndex) {
					const std::pair<Reference<ImageRenderer>, Reference<Object>>& lastData = m_rendererData[lastIndex];
					m_rendererData[index] = lastData;
					m_rendererIndexes[m_rendererData[lastIndex].first] = index;
				}
				m_rendererData.pop_back();
				m_rendererIndexes.erase(it);
			}

			void VulkanSurfaceRenderEngine::RecreateComponents() {
				// Let us make sure no random data is leaked for some reason...
				Device()->WaitIdle();
				for (size_t i = 0; i < m_mainCommandBuffers.size(); i++)
					m_mainCommandBuffers[i].commandBuffer->Reset();

				// "Notify" underlying renderers that swap chain got invalidated
				for (size_t i = 0; i < m_rendererData.size(); i++)
					m_rendererData[i].second = nullptr;

				// Reacreate swap chain
				m_shouldRecreateComponents = false;
				m_swapChainImages.clear();
				m_swapChain = nullptr;
				m_swapChain = Object::Instantiate<VulkanSwapChain>(Device(), m_windowSurface);

				// Create custom images, if the swap chain does not allow view creation
				if (!m_swapChain->ViewCreationSupported()) {
					for (size_t i = 0u; i < m_swapChain->ImageCount(); i++)
						m_swapChainImages.push_back(Device()->CreateTexture(
							Texture::TextureType::TEXTURE_2D,
							VulkanImage::PixelFormatFromNativeFormat(m_swapChain->Format().format),
							Size3(m_swapChain->Size(), 1u),
							1u,
							false,
							ImageTexture::AccessFlags::SHADER_WRITE));
				}
				else for (size_t i = 0u; i < m_swapChain->ImageCount(); i++)
					m_swapChainImages.push_back(m_swapChain->Image(i));

				// Let us make sure the swap chain images have VK_IMAGE_LAYOUT_PRESENT_SRC_KHR layout in case no attached renderer bothers to make proper changes
				m_commandPool->SubmitSingleTimeCommandBuffer([&](VkCommandBuffer buffer) {
					static thread_local std::vector<VkImageMemoryBarrier> transitions;
					transitions.resize(m_swapChain->ImageCount());

					for (size_t i = 0; i < m_swapChain->ImageCount(); i++)
						transitions[i] = m_swapChain->Image(i)->LayoutTransitionBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 1, 0, 1);

					vkCmdPipelineBarrier(
						buffer,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
						0,
						0, nullptr,
						0, nullptr,
						static_cast<uint32_t>(transitions.size()), transitions.data()
					);
					});

				const size_t maxFramesInFlight = min(MAX_FRAMES_IN_FLIGHT, m_swapChain->ImageCount());
				m_freeSemaphores.clear();
				m_mainCommandBuffers.clear();
				std::vector<Reference<PrimaryCommandBuffer>> commandBuffers = m_commandPool->CreatePrimaryCommandBuffers(m_swapChain->ImageCount());
				m_mainCommandBuffers.resize(commandBuffers.size());
				for (size_t i = 0u; i < commandBuffers.size(); i++)
					m_mainCommandBuffers[i].commandBuffer = commandBuffers[i];
				
				// Notify underlying renderers that we've got a new swap chain
				for (size_t i = 0; i < m_rendererData.size(); i++) {
					std::pair<Reference<ImageRenderer>, Reference<Object>>& entry = m_rendererData[i];
					entry.second = entry.first->CreateEngineData(&m_engineInfo);
				}
			}

			void VulkanSurfaceRenderEngine::SurfaceSizeChanged(VulkanWindowSurface*) {
				m_shouldRecreateComponents = true;
			}



			VulkanSurfaceRenderEngine::EngineInfo::EngineInfo(VulkanSurfaceRenderEngine* engine)
				: m_engine(engine) {}

			GraphicsDevice* VulkanSurfaceRenderEngine::EngineInfo::Device()const {
				return m_engine->Device(); 
			}

			Size2 VulkanSurfaceRenderEngine::EngineInfo::ImageSize()const {
				return m_engine->m_swapChain->Size(); 
			}

			Texture::PixelFormat VulkanSurfaceRenderEngine::EngineInfo::ImageFormat()const {
				return VulkanImage::PixelFormatFromNativeFormat(m_engine->m_swapChain->Format().format);
			}

			size_t VulkanSurfaceRenderEngine::EngineInfo::ImageCount()const {
				return m_engine->m_swapChain->ImageCount();
			}

			Texture* VulkanSurfaceRenderEngine::EngineInfo::Image(size_t imageId)const {
				return m_engine->m_swapChainImages[imageId];
			}
		}
	}
}

#pragma warning(default: 26812)
