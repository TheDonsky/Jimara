#include "../../GtestHeaders.h"
#include "Graphics/GraphicsInstance.h"
#include "Graphics/Vulkan/VulkanInstance.h"
#include "Graphics/Vulkan/VulkanPhysicalDevice.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/Rendering/VulkanRenderSurface.h"
#include "OS/Logging/StreamLogger.h"
#include "Core/Stopwatch.h"
#include <sstream>
#include <iomanip>
#include <thread>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				class FPSCounter {
				private:
					size_t m_frames;
					Stopwatch m_stopwatch;
					OS::Window* m_window;
					const char* m_baseTitle;
					Stopwatch m_totalExecutionTime;
					float m_autoCloseTime;


					void ForceUpdate(float elapsed) {
						std::stringstream stream;
						stream << m_baseTitle << std::setprecision(4) << " (FPS:" << (((float)m_frames) / elapsed);
						if (m_autoCloseTime > 0.0f)
							stream << std::setprecision(1) << "; Auto-close in " << (m_autoCloseTime - m_totalExecutionTime.Elapsed()) << " seconds";
						stream << ")";
						m_window->SetName(stream.str());
						m_frames = 0;
						m_stopwatch.Reset();
					}

				public:
					FPSCounter(OS::Window* window, const char* baseTitle, float autoCloseTime)
						: m_frames(0), m_window(window), m_baseTitle(baseTitle), m_autoCloseTime(autoCloseTime) {
						ForceUpdate(1.0f);
					}

					void Update() {
						m_frames++;
						float elapsed = m_stopwatch.Elapsed();
						if (elapsed >= (m_autoCloseTime > 0.0f ? 0.1f : 1.0f)) ForceUpdate(elapsed);
					}

					void ManualClose() { m_autoCloseTime = -1.0f; }

					void OnWindowUpdate(OS::Window*) { Update(); }
				};

				// Waits for some amount of time before closing the window, or till it's closed manually after being resized
				inline static void WaitForWindow(OS::Window* window, glm::uvec2 initialSize, float waitTimeBeforeResize, SurfaceRenderEngine* engine, const char* baseTitle) {
					Stopwatch stopwatch;
					bool autoClose = true;
					FPSCounter fpsCounter(window, baseTitle, waitTimeBeforeResize);
					if (engine == nullptr)
						window->OnUpdate() += Callback<OS::Window*>(&FPSCounter::OnWindowUpdate, fpsCounter);
					while (!window->Closed()) {
						if (engine != nullptr) {
							engine->Update();
							fpsCounter.Update();
						}
						else std::this_thread::sleep_for(std::chrono::milliseconds(16));
						if (autoClose) {
							if (initialSize != window->FrameBufferSize()) {
								autoClose = false;
								fpsCounter.ManualClose();
							}
							else if (stopwatch.Elapsed() > waitTimeBeforeResize) break;
						}
					}
					if (engine == nullptr)
						window->OnUpdate() -= Callback<OS::Window*>(&FPSCounter::OnWindowUpdate, fpsCounter);
				}

				class RenderEngineUpdater {
				private:
					Reference<OS::Window> m_window;
					Reference<SurfaceRenderEngine> m_renderEngine;

					inline void OnUpdate(OS::Window*) { m_renderEngine->Update(); }

				public:
					RenderEngineUpdater(OS::Window* wnd, SurfaceRenderEngine* eng) 
						: m_window(wnd), m_renderEngine(eng) {
						m_window->OnUpdate() += Callback<OS::Window*>(&RenderEngineUpdater::OnUpdate, this);
					}

					~RenderEngineUpdater() {
						m_window->OnUpdate() -= Callback<OS::Window*>(&RenderEngineUpdater::OnUpdate, this);
					}
				};
			}

			TEST(VulkanRenderingTest, Triangle) {
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("VulkanInstanceTest", Application::AppVersion(1, 0, 0));
				Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(logger, appInfo, GraphicsInstance::Backend::VULKAN);
				ASSERT_NE(graphicsInstance, nullptr);
				EXPECT_NE(Reference<VulkanInstance>(graphicsInstance), nullptr);
				
				static const glm::uvec2 size(1280, 720);
				Reference<OS::Window> window = OS::Window::Create(logger, "If you don't see a triangle here, something's going wrong...", size);
				ASSERT_NE(window, nullptr);
				Reference<RenderSurface> surface = graphicsInstance->CreateRenderSurface(window);
				ASSERT_NE(surface, nullptr);
				EXPECT_NE(Reference<VulkanWindowSurface>(surface), nullptr);

				PhysicalDevice* physicalDevice = surface->PrefferedDevice();
				ASSERT_NE(physicalDevice, nullptr);

				Reference<GraphicsDevice> graphicsDevice = physicalDevice->CreateLogicalDevice();
				ASSERT_NE(graphicsDevice, nullptr);

				Reference<SurfaceRenderEngine> renderEngine = graphicsDevice->CreateRenderEngine(surface);
				ASSERT_NE(renderEngine, nullptr);

				{
					static const char windowTitle[] = "[Rendering on window thread] You should see a triangle here...";
					RenderEngineUpdater updater(window, renderEngine);
					WaitForWindow(window, size, 5.0f, nullptr, windowTitle);
				}
				{
					static const char windowTitle[] = "[Rendering on non-window thread] You should see a triangle here...";
					void(*lambdaFn)(OS::Window*) = [](OS::Window*) -> void { std::this_thread::sleep_for(std::chrono::milliseconds(4)); };
					Callback<OS::Window*> callback = lambdaFn;
					window->OnUpdate() += callback;
					WaitForWindow(window, size, 5.0f, renderEngine, windowTitle);
				}
			}
		}
	}
}
