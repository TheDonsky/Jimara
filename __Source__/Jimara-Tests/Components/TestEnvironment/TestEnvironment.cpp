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
				float m_zoom = 0.0f;
				float m_rotationX = 0.0f;
				float m_rotationY = 0.0f;

				inline void UpdatePosition() {
					Reference<Transform> transform = GetTransfrom();
					if (transform == nullptr) return;
					{
						float deltaTime = Context()->Time()->UnscaledDeltaTime();
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

						m_zoom = max(-1.0f, min(
							m_zoom - 0.2f * Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_SCROLL_WHEEL) + 
							(deltaTime * (Context()->Input()->GetAxis(OS::Input::Axis::CONTROLLER_LEFT_TRIGGER) - Context()->Input()->GetAxis(OS::Input::Axis::CONTROLLER_RIGHT_TRIGGER)))
							, 8.0f));
					}

					float time = m_stopwatch.Elapsed();
					const Vector3 forwardColor = (GetTransfrom()->Forward() + 1.0f) * 0.5f;
					SetClearColor(Vector4(forwardColor * forwardColor, 1.0f));
					SetFieldOfView(64.0f + 32.0f * cos(time * Math::Radians(16.0f)));
					transform->SetWorldEulerAngles(Vector3(m_rotationX, m_rotationY, 0.0f));
					transform->SetLocalPosition(Vector3(0.0f, 0.25f, 0.0f) - transform->Forward() / (float)tan(Math::Radians(FieldOfView() * 0.5f)) * (1.75f + m_zoom));
				}

			public:
				inline TestCamera(Component* parent, const std::string& name) : Component(parent, name), Camera(parent, name) {
					Context()->Graphics()->OnGraphicsSynch() += Callback<>(&TestCamera::UpdatePosition, this);
				}

				virtual ~TestCamera() {
					Context()->Graphics()->OnGraphicsSynch() -= Callback<>(&TestCamera::UpdatePosition, this);
				}
			};

			class TestRenderer : public virtual Graphics::ImageRenderer {
			private:
				Reference<Camera> m_camera;
				Reference<Graphics::ImageRenderer> m_underlyingRenderer;

				std::mutex m_frameRenderLock;
				std::condition_variable m_frameRendered;
				Object* m_engineData = nullptr;
				Graphics::Pipeline::CommandBufferInfo m_bufferInfo;

				inline void OnCameraDestroyed(Component*) { 
					m_camera = nullptr; 
					m_underlyingRenderer = nullptr;
				}

				inline void OnSceneRenderFinished() {
					std::unique_lock<std::mutex> lock(m_frameRenderLock);
					if (m_engineData != nullptr) {
						if (m_underlyingRenderer != nullptr) m_underlyingRenderer->Render(m_engineData, m_bufferInfo);
						m_engineData = nullptr;
					}
					m_frameRendered.notify_one();
				}

			public:
				inline TestRenderer(Component* rootObject) 
					: m_camera(Object::Instantiate<TestCamera>(Object::Instantiate<Transform>(rootObject, "Camera Transform"), "Main Camera")) {
					m_camera->OnDestroyed() += Callback(&TestRenderer::OnCameraDestroyed, this);
					m_underlyingRenderer = m_camera->Renderer();
					if (m_underlyingRenderer == nullptr)
						rootObject->Context()->Log()->Fatal("TestEnvironment::TestRenderer - Failed to create underlying renderer!");
					m_camera->Context()->Graphics()->OnRenderFinished() += Callback(&TestRenderer::OnSceneRenderFinished, this);
				}

				inline virtual ~TestRenderer() {
					m_camera->Context()->Graphics()->OnRenderFinished() -= Callback(&TestRenderer::OnSceneRenderFinished, this);
					{
						std::unique_lock<std::mutex> lock(m_frameRenderLock);
						m_frameRendered.notify_all();
					}
					m_camera->OnDestroyed() -= Callback(&TestRenderer::OnCameraDestroyed, this);
				}

				inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
					return (m_underlyingRenderer == nullptr) ? nullptr : m_underlyingRenderer->CreateEngineData(engineInfo);
				}

				inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) override {
					std::unique_lock<std::mutex> lock(m_frameRenderLock);
					m_engineData = engineData;
					m_bufferInfo = bufferInfo;
					m_frameRendered.wait(lock);
				}
			};
		}

		TestEnvironment::TestEnvironment(const std::string_view& windowTitle, float windowTimeout) : m_windowTimeout(windowTimeout) {
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

			Reference<Physics::PhysicsInstance> physicsInstance = Physics::PhysicsInstance::Create(logger);
			if (physicsInstance == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create a physics instance!");
				return;
			}

			Reference<Audio::AudioInstance> audioInstance = Audio::AudioInstance::Create(logger);
			if (audioInstance == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create an audio instance!");
				return;
			}
			else if (audioInstance->DefaultDevice() == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Audio instance has no default device!");
				return;
			}
			
			Reference<Audio::AudioDevice> audioDevice = audioInstance->DefaultDevice()->CreateLogicalDevice();
			if (audioDevice == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create an audio device!");
				return;
			}

			Reference<Graphics::ShaderLoader> loader = Object::Instantiate<Graphics::ShaderDirectoryLoader>("Shaders/", logger);
			m_input = m_window->CreateInputModule();

			{
				Scene::GraphicsConstants graphics;
				{
					graphics.graphicsDevice = graphicsDevice;
					graphics.shaderLoader = loader;
					graphics.lightSettings.lightTypeIds = &LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.typeIds;
					graphics.lightSettings.perLightDataSize = LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.perLightDataSize;
				}
				m_scene = Scene::Create(m_input, &graphics, physicsInstance, audioDevice);
			}

			m_renderEngine = graphicsDevice->CreateRenderEngine(renderSurface);
			if (m_renderEngine == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create a render engine!");
				return;
			}

			m_renderer = Object::Instantiate<TestRenderer>(m_scene->RootObject());
			if (m_renderer == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Failed to create test renderer!");
				return;
			}

			m_renderEngine->AddRenderer(m_renderer);
			m_window->OnUpdate() += Callback<OS::Window*>(&TestEnvironment::OnWindowUpdate, this);
			m_window->OnSizeChanged() += Callback<OS::Window*>(&TestEnvironment::OnWindowResized, this);
			m_asynchUpdate.thread = std::thread([](TestEnvironment* self) { self->AsynchUpdateThread(); }, this);
		}

		TestEnvironment::~TestEnvironment() {
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
					float timeLeft = m_windowTimeout - stopwatch.Elapsed();
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
			m_renderEngine->RemoveRenderer(m_renderer);

			m_asynchUpdate.quit = true;
			m_asynchUpdate.thread.join();
			m_renderer = nullptr;
			m_scene = nullptr;
			m_renderEngine = nullptr;
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
			m_renderEngine->Update();
		}
		
		void TestEnvironment::OnWindowResized(OS::Window*) {
			m_windowResized = true;
		}

		void TestEnvironment::AsynchUpdateThread() {
			while (!m_asynchUpdate.quit) {
				if (m_asynchUpdate.stopwatch.Elapsed() >= 0.0001) {
					float updateTime = m_asynchUpdate.stopwatch.Reset();
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
					m_scene->Update(updateTime);

					{
						const size_t targetMsPerFrame = 0;
						const size_t millis = static_cast<size_t>(1000 * updateTime);
						if (millis < targetMsPerFrame)
							std::this_thread::sleep_for(std::chrono::milliseconds(targetMsPerFrame - millis));
					}
				}
				std::this_thread::yield();
			}
		}
	}
}
#pragma warning(default: 26111)
#pragma warning(default: 26115)
