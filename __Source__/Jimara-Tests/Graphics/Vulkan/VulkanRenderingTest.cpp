#include "../../GtestHeaders.h"
#include "Graphics/GraphicsInstance.h"
#include "Graphics/Vulkan/VulkanInstance.h"
#include "Graphics/Vulkan/VulkanPhysicalDevice.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/Rendering/VulkanRenderSurface.h"
#include "OS/Logging/StreamLogger.h"
#include "Core/Stopwatch.h"

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				// Waits for some amount of time before closing the window, or till it's closed manually after being resized
				inline static void WaitForWindow(OS::Window* window, glm::uvec2 initialSize, float waitTimeBeforeResize) {
					Stopwatch stopwatch;
					while (!window->Closed()) {
						if (initialSize != window->FrameBufferSize()) window->WaitTillClosed();
						else {
							std::this_thread::sleep_for(std::chrono::milliseconds(32));
							if (stopwatch.Elapsed() > waitTimeBeforeResize) break;
						}
					}
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
				
				glm::uvec2 size(1280, 720);
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

				RenderEngineUpdater updater(window, renderEngine);

				WaitForWindow(window, size, 5.0f);
			}
		}
	}
}
