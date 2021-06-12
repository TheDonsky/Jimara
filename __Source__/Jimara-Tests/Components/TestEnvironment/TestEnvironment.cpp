#include "TestEnvironment.h"
#include "Components/Camera.h"
#include "Components/Interfaces/Updatable.h"
#include "OS/Logging/StreamLogger.h"
#include "../../__Generated__/JIMARA_TEST_LIGHT_IDENTIFIERS.h"
#include <sstream>
#include <iomanip>


#pragma warning(disable: 26111)
#pragma warning(disable: 26115)
namespace Jimara {
	namespace Test {
		namespace {
			class TestCamera : public virtual Camera {
			private:
				Stopwatch m_stopwatch;
				Stopwatch m_deltaTime;
				float m_zoom = 0.0f;
				float m_rotationX = 0.0f;
				float m_rotationY = 0.0f;

				inline void UpdatePosition() {
					Reference<Transform> transform = GetTransfrom();
					if (transform == nullptr) return;
					{
						float deltaTime = m_deltaTime.Reset();
						const float SENSITIVITY = 128.0f;

						auto twoKeyCodeAxis = [&](OS::Input::KeyCode positive, OS::Input::KeyCode negative) {
							return (Context()->Input()->KeyPressed(negative) ? (-1.0f) : 0.0f) +
								(Context()->Input()->KeyPressed(positive) ? (1.0f) : 0.0f);
						};
						Vector2 delta = Vector2(
							twoKeyCodeAxis(OS::Input::KeyCode::W, OS::Input::KeyCode::S) + Context()->Input()->GetAxis(OS::Input::Axis::CONTROLLER_RIGHT_ANALOG_Y),
							twoKeyCodeAxis(OS::Input::KeyCode::D, OS::Input::KeyCode::A) + Context()->Input()->GetAxis(OS::Input::Axis::CONTROLLER_RIGHT_ANALOG_X));

						if (Context()->Input()->KeyPressed(OS::Input::KeyCode::MOUSE_LEFT_BUTTON))
							delta += Vector2(Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_Y), Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_X));

						m_rotationX = max(-80.0f, min(m_rotationX + deltaTime * SENSITIVITY * (delta.x), 80.0f));
						m_rotationY += deltaTime * SENSITIVITY * delta.y;

						m_zoom = max(-1.0f, min(m_zoom - 0.2f * Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_SCROLL_WHEEL), 4.0f));
					}

					float time = m_stopwatch.Elapsed();
					SetClearColor(Vector4(
						0.0625f * (1.0f + cos(time * Math::Radians(8.0f)) * sin(time * Math::Radians(10.0f))),
						0.125f * (1.0f + cos(time * Math::Radians(12.0f))),
						0.125f * (1.0f + sin(time * Math::Radians(14.0f))), 1.0f));
					SetFieldOfView(64.0f + 32.0f * cos(time * Math::Radians(16.0f)));
					transform->SetWorldEulerAngles(Vector3(m_rotationX, m_rotationY, 0.0f));
					transform->SetLocalPosition(Vector3(0.0f, 0.25f, 0.0f) - transform->Forward() / (float)tan(Math::Radians(FieldOfView() * 0.5f)) * (1.75f + m_zoom));
				}

			public:
				inline TestCamera(Component* parent, const std::string& name) : Component(parent, name), Camera(parent, name) {
					Context()->Graphics()->OnPostGraphicsSynch() += Callback<>(&TestCamera::UpdatePosition, this);
				}

				virtual ~TestCamera() {
					Context()->Graphics()->OnPostGraphicsSynch() -= Callback<>(&TestCamera::UpdatePosition, this);
				}
			};
		}

		TestEnvironment::TestEnvironment(const std::string_view& windowTitle) {
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("JimaraTest", Application::AppVersion(1, 0, 0));
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo);
			if (graphicsInstance == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Graphics instance creation failed!");
				return;
			}

			m_baseWindowName = std::string(windowTitle);
			m_windowName = m_baseWindowName;
			m_window = OS::Window::Create(logger, m_windowName);
			if (m_window == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Window creation failed!");
				return;
			}
			
			Reference<Graphics::RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(m_window);
			if (renderSurface == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Render surface creation failed!");
				return;
			}

			Graphics::PhysicalDevice* physicalDevice = renderSurface->PrefferedDevice();
			if (physicalDevice == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Render surface could not find a compatible physical device!");
				return;
			}

			Reference<Graphics::GraphicsDevice> graphicsDevice = physicalDevice->CreateLogicalDevice();
			if (graphicsDevice == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create a graphics device!");
				return;
			}

			Reference<AppContext> appContext = Object::Instantiate<AppContext>(graphicsDevice);
			Reference<ShaderLoader> loader = Object::Instantiate<ShaderDirectoryLoader>("Shaders/", logger);
			m_input = m_window->CreateInputModule();
			if (m_input == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create an input module!");
				return;
			}

			m_scene = Object::Instantiate<Scene>(appContext, loader, m_input,
				LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.typeIds, LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.perLightDataSize);

			m_renderEngine = graphicsDevice->CreateRenderEngine(renderSurface);
			if (m_renderEngine == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create a render engine!");
				return;
			}

			m_renderer = Object::Instantiate<TestCamera>(Object::Instantiate<Transform>(m_scene->RootObject(), "Camera Transform"), "Main Camera")->Renderer();
			if (m_renderer == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create test renderer!");
				return;
			}

			m_renderEngine->AddRenderer(m_renderer);
			m_window->OnUpdate() += Callback<OS::Window*>(&TestEnvironment::OnWindowUpdate, this);
			m_window->OnSizeChanged() += Callback<OS::Window*>(&TestEnvironment::OnWindowResized, this);
			m_asynchUpdate.thread = std::thread([&]() { AsynchUpdateThread(); });
		}

		TestEnvironment::~TestEnvironment() {
			if (m_renderEngine == nullptr) return;

			const Stopwatch stopwatch;
			while (!m_window->Closed()) {
				if (m_windowResized) {
					{
						std::unique_lock<std::mutex> lock(m_windowNameLock);
						m_windowName = m_baseWindowName;
					}
					m_window->WaitTillClosed();
				}
				else {
					float timeLeft = 5.0f - stopwatch.Elapsed();
					if (timeLeft <= 0.0f) break;
					{
						std::stringstream stream;
						stream << m_baseWindowName << std::fixed << std::setprecision(2) << " [Closing in " << timeLeft << " seconds, unless resized]";
						std::unique_lock<std::mutex> lock(m_windowNameLock);
						m_windowName = stream.str();
					}
					std::this_thread::sleep_for(std::chrono::microseconds(2));
				}
			}
			m_window->OnUpdate() -= Callback<OS::Window*>(&TestEnvironment::OnWindowUpdate, this);
			m_window->OnSizeChanged() -= Callback<OS::Window*>(&TestEnvironment::OnWindowResized, this);

			m_asynchUpdate.quit = true;
			m_asynchUpdate.startIteration.post();
			m_asynchUpdate.thread.join();
		}


		void TestEnvironment::SetWindowName(const std::string_view& newName) {
			std::unique_lock<std::mutex> lock(m_windowNameLock);
			m_baseWindowName = newName;
		}

		Component* TestEnvironment::RootObject()const { return m_scene->RootObject(); }

		void TestEnvironment::ExecuteOnUpdate(const Callback<TestEnvironment*>& callback) {
			std::unique_lock<std::mutex> lock(m_asynchUpdate.updateQueueLock);
			m_asynchUpdate.updateQueue[m_asynchUpdate.updateQueueBackBufferId].push(callback);
		}

		namespace {
			struct ExecuteOnUpdateTask {
				const Callback<TestEnvironment*>* callback = nullptr;
				Semaphore semaphore;

				void Execute(TestEnvironment* environment) {
					(*callback)(environment);
					semaphore.post();
				}
			};
		}

		void TestEnvironment::ExecuteOnUpdateNow(const Callback<TestEnvironment*>& callback) {
			ExecuteOnUpdateTask task;
			task.callback = &callback;
			task.semaphore.set(0);
			ExecuteOnUpdate(Callback(&ExecuteOnUpdateTask::Execute, task));
			task.semaphore.wait();
		}

		void TestEnvironment::OnWindowUpdate(OS::Window*) {
			{
				float deltaTime = m_fpsCounter.deltaTime.Reset();
				m_fpsCounter.smoothDeltaTime = deltaTime * 0.01f + m_fpsCounter.smoothDeltaTime * 0.99f;
				if (m_fpsCounter.timeSinceRefresh.Elapsed() > 0.25f) {
					std::stringstream stream;
					std::unique_lock<std::mutex> lock(m_windowNameLock);
					stream << std::fixed << std::setprecision(2)
						<< "[S_DT:" << (m_fpsCounter.smoothDeltaTime * 1000.0f) << "; S_FPS:" << (1.0f / m_fpsCounter.smoothDeltaTime)
						<< "; DT:" << (deltaTime * 0.001f) << "; FPS:" << (1.0f / deltaTime) << "] " << m_windowName;
					m_window->SetName(stream.str());
					m_fpsCounter.timeSinceRefresh.Reset();
				}
			}
			{
				if (m_asynchUpdate.stopwatch.Elapsed() >= 0.001f) {
					m_asynchUpdate.stopwatch.Reset();
					m_asynchUpdate.endIteration.wait();
					m_input->Update();
					m_asynchUpdate.startIteration.post();
				}
			}
			m_renderEngine->Update();
		}
		
		void TestEnvironment::OnWindowResized(OS::Window*) {
			m_windowResized = true;
		}

		void TestEnvironment::AsynchUpdateThread() {
			while (!m_asynchUpdate.quit) {
				m_asynchUpdate.endIteration.post();
				m_asynchUpdate.startIteration.wait();
				{
					std::queue<Callback<TestEnvironment*>>* updateQueue = nullptr;
					{
						std::unique_lock<std::mutex> lock(m_asynchUpdate.updateQueueLock);
						updateQueue = m_asynchUpdate.updateQueue + m_asynchUpdate.updateQueueBackBufferId;
						m_asynchUpdate.updateQueueBackBufferId = (m_asynchUpdate.updateQueueBackBufferId + 1) & 1;
					}
					while (!updateQueue->empty()) {
						updateQueue->front()(this);
						updateQueue->pop();
					}
				}
				m_scene->SynchGraphics();
				m_scene->Update();
			}
			m_asynchUpdate.endIteration.post();
		}
	}
}
#pragma warning(default: 26111)
#pragma warning(default: 26115)
