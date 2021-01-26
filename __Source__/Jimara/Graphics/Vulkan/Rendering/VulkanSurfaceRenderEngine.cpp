#include "VulkanSurfaceRenderEngine.h"
#pragma warning(disable: 26812)
#define MAX_FRAMES_IN_FLIGHT ((size_t)3)

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanSurfaceRenderEngine::VulkanSurfaceRenderEngine(VulkanDevice* device, VulkanWindowSurface* surface) 
				: VulkanRenderEngine(device)
				, m_engineInfo(this), m_commandPool(device)
				, m_windowSurface(surface)
				, m_semaphoreIndex(0)
				, m_shouldRecreateComponents(false) {
				RecreateComponents();
				m_windowSurface->OnSizeChanged() += Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
			}

			VulkanSurfaceRenderEngine::~VulkanSurfaceRenderEngine() {
				m_windowSurface->OnSizeChanged() -= Callback<VulkanWindowSurface*>(&VulkanSurfaceRenderEngine::SurfaceSizeChanged, this);
				vkDeviceWaitIdle(*Device());
				m_rendererIndexes.clear();
				m_rendererData.clear();
				m_commandPool.DestroyCommandBuffers(m_mainCommandBuffers);
			}

			void VulkanSurfaceRenderEngine::Update() {
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				std::unique_lock<std::mutex> resizeLock(m_windowSurface->ResizeLock());

				// Semaphores we will be using for frame synchronisation:
				VulkanSemaphore& imageAvailableSemaphore = m_imageAvailableSemaphores[m_semaphoreIndex];
				VulkanSemaphore& renderFinishedSemaphore = m_renderFinishedSemaphores[m_semaphoreIndex];

				// We need to get the target image:
				size_t imageId;
				VulkanImage* targetImage;
				while (true) {
					// If the surface size is 0, there's no need to render anything to it
					Size2 size = m_windowSurface->Size();
					if (size.x <= 0 || size.y <= 0) return;

					// We need to recreate components if our swap chain is no longer valid
					if (m_swapChain->AquireNextImage(imageAvailableSemaphore, imageId, targetImage)) break;
					else RecreateComponents();
				}

				// Image is squired, so we just need to wait for it finish being presented in case it's still being rendered on
				VulkanFence& inFlightFence = m_inFlightFences[imageId];
				inFlightFence.WaitAndReset();
				m_semaphoreIndex = (m_semaphoreIndex + 1) % m_imageAvailableSemaphores.size();

				// Prepare recorder:
				Recorder& recorder = m_commandRecorders[imageId];
				{
					recorder.dependencies.clear();

					recorder.semaphoresToWaitFor.clear();
					recorder.semaphoresToWaitFor.push_back(imageAvailableSemaphore);

					recorder.semaphoresToSignal.clear();
					recorder.semaphoresToSignal.push_back(renderFinishedSemaphore);
				}

				// Record command buffer:
				{
					// Reset buffer
					if (vkResetCommandBuffer(recorder.commandBuffer, 0) != VK_SUCCESS)
						Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to reset command buffer");

					// Begin command buffer
					{
						VkCommandBufferBeginInfo beginInfo = {};
						beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
						beginInfo.flags = 0; // Optional
						beginInfo.pInheritanceInfo = nullptr; // Optional
						if (vkBeginCommandBuffer(recorder.commandBuffer, &beginInfo) != VK_SUCCESS)
							Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to begin recording command buffer!");
					}

					// Let all underlying renderers record their commands
					for (size_t i = 0; i < m_rendererData.size(); i++) {
						VulkanImageRenderer::EngineData* rendererData = m_rendererData[i];
						rendererData->Render(&recorder);
						recorder.dependencies.push_back(rendererData);
					}

					// End command buffer
					if (vkEndCommandBuffer(recorder.commandBuffer) != VK_SUCCESS)
						Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to end command buffer!");
				}

				// Submit command buffer:
				{
					VkSubmitInfo submitInfo = {};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

					// Wait for image availability
					submitInfo.waitSemaphoreCount = static_cast<uint32_t>(recorder.semaphoresToWaitFor.size());
					submitInfo.pWaitSemaphores = recorder.semaphoresToWaitFor.data();
					
					// Wait for semaphores when we need to access color attachment
					VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
					submitInfo.pWaitDstStageMask = waitStages;

					// Command buffer to submit
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &recorder.commandBuffer;

					// Semaphores to signal
					submitInfo.signalSemaphoreCount = static_cast<uint32_t>(recorder.semaphoresToSignal.size());;
					submitInfo.pSignalSemaphores = recorder.semaphoresToSignal.data();

					if (vkQueueSubmit(m_commandPool.Queue(), 1, &submitInfo, inFlightFence) != VK_SUCCESS)
						Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Failed to submit draw command buffer!");
				}

				// Present rendered image
				if (!m_swapChain->Present(imageId, renderFinishedSemaphore)) m_shouldRecreateComponents = true;
				if (m_shouldRecreateComponents) RecreateComponents();
			}

			void VulkanSurfaceRenderEngine::AddRenderer(ImageRenderer* renderer) {
				VulkanImageRenderer* imageRenderer = dynamic_cast<VulkanImageRenderer*>(renderer);
				if (imageRenderer == nullptr) return;
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				if (m_rendererIndexes.find(imageRenderer) != m_rendererIndexes.end()) return;

				Reference<VulkanImageRenderer::EngineData> engineData = imageRenderer->CreateEngineData(&m_engineInfo);
				if (engineData == nullptr) Device()->Log()->Error("VulkanSurfaceRenderEngine - Renderer failed to provide render engine data!");
				else {
					m_rendererIndexes.insert(std::make_pair(imageRenderer, m_rendererData.size()));
					m_rendererData.push_back(engineData);
				}
			}

			void VulkanSurfaceRenderEngine::RemoveRenderer(ImageRenderer* renderer) {
				VulkanImageRenderer* imageRenderer = dynamic_cast<VulkanImageRenderer*>(renderer);
				if (imageRenderer == nullptr) return;
				std::unique_lock<std::recursive_mutex> rendererLock(m_rendererLock);
				
				std::unordered_map<VulkanImageRenderer*, size_t>::const_iterator it = m_rendererIndexes.find(imageRenderer);
				if (it == m_rendererIndexes.end()) return;

				size_t index = it->second;
				m_rendererIndexes.erase(it);

				size_t lastIndex = m_rendererData.size() - 1;
				if (it->second < lastIndex) {
					const Reference<VulkanImageRenderer::EngineData>& lastData = m_rendererData[lastIndex];
					m_rendererData[index] = lastData;
					m_rendererIndexes[m_rendererData[lastIndex]->Renderer()] = index;
				}
				m_rendererData.pop_back();
			}

			void VulkanSurfaceRenderEngine::RecreateComponents() {
				// Let us make sure no random data is leaked for some reason...
				vkDeviceWaitIdle(*Device());
				for (size_t i = 0; i < m_commandRecorders.size(); i++)
					m_commandRecorders[i].dependencies.clear();

				// "Notify" underlying renderers that swap chain got invalidated
				std::vector<Reference<VulkanImageRenderer>> renderers;
				for (size_t i = 0; i < m_rendererData.size(); i++) {
					Reference<VulkanImageRenderer::EngineData>& data = m_rendererData[i];
					renderers.push_back(data->Renderer());
					data = nullptr;
				}

				// Reacreate swap chain
				m_shouldRecreateComponents = false;
				m_swapChain = nullptr;
				m_swapChain = Object::Instantiate<VulkanSwapChain>(Device(), m_windowSurface);

				// Let us make sure the swap chain images have VK_IMAGE_LAYOUT_PRESENT_SRC_KHR layout in case no attached renderer bothers to make proper changes
				m_commandPool.SubmitSingleTimeCommandBuffer([&](VkCommandBuffer buffer) {
					static std::vector<VkImageMemoryBarrier> transitions;
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
				while (m_imageAvailableSemaphores.size() < maxFramesInFlight) {
					m_imageAvailableSemaphores.push_back(VulkanSemaphore(Device()));
					m_renderFinishedSemaphores.push_back(VulkanSemaphore(Device()));
				}
				m_imageAvailableSemaphores.resize(maxFramesInFlight);
				m_renderFinishedSemaphores.resize(maxFramesInFlight);

				while (m_inFlightFences.size() < m_swapChain->ImageCount())
					m_inFlightFences.push_back(VulkanFence(Device(), true));

				m_commandPool.DestroyCommandBuffers(m_mainCommandBuffers);
				m_mainCommandBuffers = m_commandPool.CreateCommandBuffers(m_swapChain->ImageCount());
				m_commandRecorders.resize(m_mainCommandBuffers.size());
				for (size_t i = 0; i < m_mainCommandBuffers.size(); i++) {
					Recorder& recorder = m_commandRecorders[i];
					recorder.imageIndex = i;
					recorder.image = m_swapChain->Image(i);
					recorder.commandBuffer = m_mainCommandBuffers[i];
					recorder.commandPool = &m_commandPool;
					recorder.dependencies.clear();
				}

				// Notify underlying renderers that we've got a new swap chain
				for (size_t i = 0; i < m_rendererData.size(); i++) {
					Reference<VulkanImageRenderer::EngineData> engineData = renderers[i]->CreateEngineData(&m_engineInfo);
					if (engineData == nullptr) Device()->Log()->Fatal("VulkanSurfaceRenderEngine - Renderer failed to recreate render engine data!");
					else m_rendererData[i] = engineData;
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

			Size2 VulkanSurfaceRenderEngine::EngineInfo::TargetSize()const {
				return m_engine->m_swapChain->Size(); 
			}

			size_t VulkanSurfaceRenderEngine::EngineInfo::ImageCount()const { 
				return m_engine->m_swapChain->ImageCount(); 
			}

			VulkanImage* VulkanSurfaceRenderEngine::EngineInfo::Image(size_t imageId)const {
				return m_engine->m_swapChain->Image(imageId);
			}

			VkFormat VulkanSurfaceRenderEngine::EngineInfo::ImageFormat()const {
				return m_engine->m_swapChain->Format().format;
			}

			VkSampleCountFlagBits VulkanSurfaceRenderEngine::EngineInfo::MSAASamples(GraphicsSettings::MSAA desired)const {
				const VkPhysicalDeviceProperties& properties = m_engine->Device()->PhysicalDeviceInfo()->DeviceProperties();
				VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
				if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_64 && ((counts & VK_SAMPLE_COUNT_64_BIT) != 0)) return VK_SAMPLE_COUNT_64_BIT;
				else if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_32 && ((counts & VK_SAMPLE_COUNT_32_BIT) != 0)) return VK_SAMPLE_COUNT_32_BIT;
				else if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_16 && ((counts & VK_SAMPLE_COUNT_16_BIT) != 0)) return VK_SAMPLE_COUNT_16_BIT;
				else if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_8 && ((counts & VK_SAMPLE_COUNT_8_BIT) != 0)) return VK_SAMPLE_COUNT_8_BIT;
				else if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_4 && ((counts & VK_SAMPLE_COUNT_4_BIT) != 0)) return VK_SAMPLE_COUNT_4_BIT;
				else if (desired >= GraphicsSettings::MSAA::SAMPLE_COUNT_2 && ((counts & VK_SAMPLE_COUNT_2_BIT) != 0)) return VK_SAMPLE_COUNT_2_BIT;
				else return VK_SAMPLE_COUNT_1_BIT;
			}
		}
	}
}

#pragma warning(default: 26812)
