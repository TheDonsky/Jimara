#include "TestEnvironment.h"
#include "Components/Camera.h"
#include "OS/Logging/StreamLogger.h"
#include "Environment/Scene/SceneUpdateLoop.h"
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
				inline TestCamera(Component* parent, const std::string_view& name = "Test Camera") : Component(parent, name), Camera(parent, name) {
					Context()->Graphics()->OnGraphicsSynch() += Callback<>(&TestCamera::UpdatePosition, this);
				}

				virtual ~TestCamera() {
					Context()->Graphics()->OnGraphicsSynch() -= Callback<>(&TestCamera::UpdatePosition, this);
				}
			};

			class TestRenderer : public virtual Graphics::ImageRenderer {
			private:
				const Reference<Scene::LogicContext> m_context;
				const Reference<RenderStack> m_renderStack;
				std::condition_variable m_canPresentFrame;

				struct EngineData : public virtual Object {
					const Reference<Graphics::RenderEngineInfo> engineInfo;
					inline EngineData(Graphics::RenderEngineInfo* info) : engineInfo(info) {}
				};

				inline void OnFrameRendered() { m_canPresentFrame.notify_one(); }

			public:
				inline TestRenderer(Component* rootObject) 
					: m_context(rootObject->Context())
					, m_renderStack(RenderStack::Main(rootObject->Context())) {
					m_context->Graphics()->OnRenderFinished() += Callback(&TestRenderer::OnFrameRendered, this);
				}

				inline virtual ~TestRenderer() {
					m_context->Graphics()->OnRenderFinished() -= Callback(&TestRenderer::OnFrameRendered, this);
				}

				inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
					m_renderStack->SetResolution(engineInfo->ImageSize());
					return Object::Instantiate<EngineData>(engineInfo);
				}

				inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) override {
					Reference<RenderImages> images = m_renderStack->Images();
					Reference<Graphics::TextureView> targetTexture = images == nullptr ? nullptr : images->GetImage(RenderImages::MainColor())->Resolve();
					if (targetTexture == nullptr) return;
					EngineData* data = dynamic_cast<EngineData*>(engineData);
					if (data == nullptr) {
						m_context->Log()->Error("TestRenderer::CreateEngineData - Invalid engine data!");
						return;
					}
					static thread_local std::mutex mutex;
					std::unique_lock<std::mutex> lock(mutex);
					m_canPresentFrame.wait(lock);
					data->engineInfo->Image(bufferInfo.inFlightBufferId)->Blit(bufferInfo.commandBuffer, targetTexture->TargetTexture());
				}
			};

			class TestCameraSerializer : public virtual ComponentSerializer::Of<TestCamera> {
			public:
				inline TestCameraSerializer() : ItemSerializer("Test Camera", "Camera for Jimara tests") {}

				inline static const TestCameraSerializer* Instance() {
					static const TestCameraSerializer instance;
					return &instance;
				}

				virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, TestCamera* target)const final override {
					TypeId::Of<Camera>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				}
			};
		}
	}

	template<> inline void TypeIdDetails::GetTypeAttributesOf<Jimara::Test::TestCamera>(const Callback<const Object*>& report) {
		report(Jimara::Test::TestCameraSerializer::Instance());
	}

	namespace Test {

		TestEnvironment::TestEnvironment(const std::string_view& windowTitle, float windowTimeout) : m_windowTimeout(windowTimeout) {
			Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("JimaraTest", Application::AppVersion(1, 0, 0));
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo);
			if (graphicsInstance == nullptr) {
				logger->Fatal("TestEnvironment::TestEnvironment - Graphics instance creation failed!");
				return;
			}

			m_windowName = std::string(windowTitle);
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

			{
				Scene::CreateArgs args;
				{
					args.logic.input = m_window->CreateInputModule();
				}
				{
					args.graphics.graphicsDevice = graphicsDevice;
					args.graphics.shaderLoader = Object::Instantiate<Graphics::ShaderDirectoryLoader>("Shaders/", logger);
					args.graphics.lightSettings.lightTypeIds = &LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.typeIds;
					args.graphics.lightSettings.perLightDataSize = LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.perLightDataSize;
				}
				{
					args.createMode = Scene::CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_SUPRESS_WARNINGS;
				}
				m_scene = Scene::Create(args);
				{
					Reference<const Object> token = TypeId::Of<TestCamera>().Register();
					m_scene->Context()->StoreDataObject(token);
				}
				Object::Instantiate<TestCamera>(Object::Instantiate<Transform>(m_scene->Context()->RootObject(), "Camera Transform"), "Main Camera");
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

			m_sceneUpdateLoop = Object::Instantiate<SceneUpdateLoop>(m_scene);
			m_renderEngine->AddRenderer(m_renderer);
			m_window->OnUpdate() += Callback<OS::Window*>(&TestEnvironment::OnWindowUpdate, this);
			m_window->OnSizeChanged() += Callback<OS::Window*>(&TestEnvironment::OnWindowResized, this);
		}

		TestEnvironment::~TestEnvironment() {
			const Stopwatch stopwatch;
			while (!m_window->Closed()) {
				if (m_windowResized) {
					m_window->WaitTillClosed();
				}
				else {
					float timeLeft = m_windowTimeout - stopwatch.Elapsed();
					if (timeLeft <= 0.0f) break;
					{
						std::stringstream stream;
						stream << std::fixed << std::setprecision(2) << " [Closing in " << timeLeft << " seconds, unless resized]";
						std::unique_lock<std::mutex> lock(m_windowNameLock);
						m_windowSuffix = stream.str();
					}
					std::this_thread::sleep_for(std::chrono::microseconds(2));
				}
			}
			m_window->OnUpdate() -= Callback<OS::Window*>(&TestEnvironment::OnWindowUpdate, this);
			m_window->OnSizeChanged() -= Callback<OS::Window*>(&TestEnvironment::OnWindowResized, this);
			m_renderEngine->RemoveRenderer(m_renderer);

			m_renderer = nullptr;
			m_renderEngine = nullptr;
			m_sceneUpdateLoop = nullptr;
			m_scene = nullptr;
		}


		void TestEnvironment::SetWindowName(const std::string_view& newName) {
			std::unique_lock<std::mutex> lock(m_windowNameLock);
			m_windowName = newName;
		}

		Component* TestEnvironment::RootObject()const { return m_scene->RootObject(); }

		void TestEnvironment::ExecuteOnUpdate(const Callback<Object*>& callback, Object* userData) {
			m_scene->Context()->ExecuteAfterUpdate(callback, userData);
		}

		namespace {
			struct ExecuteOnUpdateTask {
				Semaphore semaphore = Semaphore(0);
				const Callback<Object*>* callback = nullptr;

				void Execute(Object* userData) {
					(*callback)(userData);
					semaphore.post();
				}
			};
		}

		void TestEnvironment::ExecuteOnUpdateNow(const Callback<Object*>& callback, Object* userData) {
			ExecuteOnUpdateTask task;
			task.callback = &callback;
			ExecuteOnUpdate(Callback(&ExecuteOnUpdateTask::Execute, task), userData);
			task.semaphore.wait();
		}

		void TestEnvironment::OnWindowUpdate(OS::Window*) {
			{
				float deltaTime = m_scene->Context()->Time()->UnscaledDeltaTime();
				m_fpsCounter.smoothDeltaTime = deltaTime * 0.01f + m_fpsCounter.smoothDeltaTime * 0.99f;
				if (m_fpsCounter.timeSinceRefresh.Elapsed() > 0.25f) {
					std::stringstream stream;
					std::unique_lock<std::mutex> lock(m_windowNameLock);
					stream << std::fixed << std::setprecision(2)
						<< "[S_DT:" << (m_fpsCounter.smoothDeltaTime * 1000.0f) << "; S_FPS:" << (1.0f / m_fpsCounter.smoothDeltaTime)
						<< "; DT:" << (deltaTime * 0.001f) << "; FPS:" << (1.0f / deltaTime) << "] " << m_windowName << m_windowSuffix;
					m_window->SetName(stream.str());
					m_fpsCounter.timeSinceRefresh.Reset();
				}
			}
			m_renderEngine->Update();
		}
		
		void TestEnvironment::OnWindowResized(OS::Window*) {
			m_windowResized = true;
		}
	}
}
#pragma warning(default: 26111)
#pragma warning(default: 26115)
