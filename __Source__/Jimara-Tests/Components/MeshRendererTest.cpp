#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/Logging/StreamLogger.h"
#include "Components/Camera.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Environment/Scene.h"
#include "Components/Interfaces/Updatable.h"
#include "../__Generated__/JIMARA_TEST_LIGHT_IDENTIFIERS.h"
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>
#include <cmath>


namespace Jimara {
	namespace {
		class Environment {
		private:
			std::mutex m_windowNameLock;
			std::string m_windowName;

			Reference<OS::Window> m_window;
			Reference<Graphics::RenderEngine> m_surfaceRenderEngine;
			Reference<Scene> m_scene;

			Stopwatch m_time;
			float m_lastTime;
			float m_deltaTime;
			float m_smoothDeltaTime;

			Stopwatch m_fpsUpdateTimer;
			std::atomic<uint64_t> sizeChangeCount;
			std::atomic<float> closingIn;

			std::thread m_asynchUpdateThread;
			std::atomic<bool> m_dead;

			inline void AsynchUpdateThread() {
				Stopwatch stopwatch;
				while (!m_dead) {
					float deltaTime = stopwatch.Reset();
					m_scene->SynchGraphics();
					m_scene->Update();
					const uint64_t DESIRED_DELTA_MICROSECONDS = 10000u;
					uint64_t deltaMicroseconds = static_cast<uint64_t>(static_cast<double>(deltaTime) * 1000000.0);
					if (DESIRED_DELTA_MICROSECONDS > deltaMicroseconds) {
						uint64_t sleepTime = (DESIRED_DELTA_MICROSECONDS - deltaMicroseconds);
						std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
					}
				}
			}

			inline void OnUpdate(OS::Window*) { 
				{
					float now = m_time.Elapsed();
					m_deltaTime = now - m_lastTime;
					m_smoothDeltaTime = m_deltaTime * 0.01f + m_smoothDeltaTime * 0.99f;
					m_lastTime = now;
				}
				if (m_fpsUpdateTimer.Elapsed() >= 0.25f) {
					std::unique_lock<std::mutex> lock(m_windowNameLock);
					std::stringstream stream;
					stream << std::fixed << std::setprecision(2)
						<< "[S_DT:" << (m_smoothDeltaTime * 1000.0f) << "; S_FPS:" << (1.0f / m_smoothDeltaTime)
						<< "; DT:" << (m_deltaTime * 0.001f) << "; FPS:" << (1.0f / m_deltaTime) << "] " << m_windowName;
					const float timeLeft = closingIn;
					if ((timeLeft >= 0.0f) && sizeChangeCount > 0)
						stream << " [Closing in " << timeLeft << " seconds, unless resized]";
					m_window->SetName(stream.str());
					m_fpsUpdateTimer.Reset();
				}
				m_surfaceRenderEngine->Update();
				std::this_thread::yield();
			}

			inline void WindowResized(OS::Window*) { 
				if (sizeChangeCount > 0)
					sizeChangeCount--;
			}

		public:
			inline Environment(const char* wndName = nullptr) 
				: m_windowName(wndName == nullptr ? "" : wndName)
				, m_lastTime(0.0f), m_deltaTime(0.0f), m_smoothDeltaTime(0.0f)
				, sizeChangeCount(1), closingIn(-1.0f), m_dead(false) {
				Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("JimaraTest", Application::AppVersion(1, 0, 0));
				Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
				Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInfo);
				Reference<Graphics::GraphicsDevice> graphicsDevice;
				if (wndName != nullptr) {
					m_window = OS::Window::Create(logger, m_windowName);
					Reference<Graphics::RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(m_window);
					graphicsDevice = renderSurface->PrefferedDevice()->CreateLogicalDevice();
					m_surfaceRenderEngine = graphicsDevice->CreateRenderEngine(renderSurface);
				}
				else if (graphicsInstance->PhysicalDeviceCount() > 0)
					graphicsDevice = graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();
				if (graphicsDevice != nullptr) {
					Reference<AppContext> appContext = Object::Instantiate<AppContext>(graphicsDevice);
					Reference<ShaderLoader> loader = Object::Instantiate<ShaderDirectoryLoader>("Shaders/", logger);
					m_scene = Object::Instantiate<Scene>(appContext, loader,
						LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.typeIds, LightRegistry::JIMARA_TEST_LIGHT_IDENTIFIERS.perLightDataSize);
				}
				else logger->Fatal("Environment could not be set up due to the insufficient hardware!");
				if (m_window != nullptr) {
					m_window->OnUpdate() += Callback<OS::Window*>(&Environment::OnUpdate, this);
					m_window->OnSizeChanged() += Callback<OS::Window*>(&Environment::WindowResized, this);
				}
				m_asynchUpdateThread = std::thread([&]() { AsynchUpdateThread(); });
			}

			inline ~Environment() {
				if (m_window != nullptr) {
					Stopwatch stopwatch;
					while (!m_window->Closed()) {
						if (sizeChangeCount > 0) {
							float timeLeft = 5.0f - stopwatch.Elapsed();
							if (timeLeft > 0.0f) {
								closingIn = timeLeft;
								std::this_thread::sleep_for(std::chrono::microseconds(2));
							}
							else break;
						}
						else m_window->WaitTillClosed();
					}
					m_window->OnUpdate() -= Callback<OS::Window*>(&Environment::OnUpdate, this);
					m_window->OnSizeChanged() -= Callback<OS::Window*>(&Environment::WindowResized, this);
				}
				m_dead = true;
				m_asynchUpdateThread.join();
				m_scene = nullptr;
			}

			inline void SetWindowName(const std::string& name) {
				std::unique_lock<std::mutex> lock(m_windowNameLock);
				m_windowName = name;
			}

			inline Component* RootObject()const { return m_scene->RootObject(); }

			inline Graphics::RenderEngine* RenderEngine()const { return m_surfaceRenderEngine; }
		};


		class TestMaterial : public virtual Material {
		private:
			const Reference<Graphics::TextureSampler> m_sampler;

			class ShaderClass : public virtual Graphics::ShaderClass {
			public:
				inline ShaderClass() : Graphics::ShaderClass("Jimara-Tests/Components/Shaders/Test_SampleDiffuseShader") {}

				inline static ShaderClass* Instance() {
					static ShaderClass instance;
					return &instance;
				}
			};

			static const std::string& LightingModelPath() {
				static const std::string path = "Shaders/Jimara/Environment/GraphicsContext/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm";
				return path;
			}

		public:
			inline TestMaterial(Graphics::ShaderCache* cache, Graphics::Texture* texture)
				: m_sampler(texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler()) {
				Material::Writer writer(this);
				writer.SetShader(ShaderClass::Instance());
				writer.SetTextureSampler("texSampler", m_sampler);
			}
		};


		class TestRenderer : public virtual Graphics::ImageRenderer {
		private:
			Stopwatch m_stopwatch;
			const Reference<Camera> m_camera;
			Reference<Graphics::ImageRenderer> m_renderer;

			inline void Update() {
				float time = m_stopwatch.Elapsed();
				m_camera->SetClearColor(Vector4(
					0.0625f * (1.0f + cos(time * Math::Radians(8.0f)) * sin(time * Math::Radians(10.0f))),
					0.125f * (1.0f + cos(time * Math::Radians(12.0f))),
					0.125f * (1.0f + sin(time * Math::Radians(14.0f))), 1.0f));
				m_camera->SetFieldOfView(64.0f + 32.0f * cos(time * Math::Radians(16.0f)));
				m_camera->GetTransfrom()->SetWorldPosition(
					Vector4(1.5f, 1.0f + 0.8f * cos(time * Math::Radians(15.0f)), 1.5f, 0.0f)
					* Math::MatrixFromEulerAngles(Vector3(0.0f, time * 10.0f, 0.0f))
					/ tan(Math::Radians(m_camera->FieldOfView() * 0.5f)) * 0.5f);
				m_camera->GetTransfrom()->LookAt(Vector3(0.0f, 0.25f, 0.0f));
			}

		public:
			inline TestRenderer(Component* rootObject)
				: m_camera(Object::Instantiate<Camera>(Object::Instantiate<Transform>(rootObject, "Camera Transform"), "Main Camera")) {
				m_renderer = m_camera->Renderer();
			}

			inline virtual ~TestRenderer() {}

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
				return m_renderer->CreateEngineData(engineInfo);
			}

			inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) override {
				Update();
				m_renderer->Render(engineData, bufferInfo);
			}
		};
	}





	// Renders axis-facing cubes to make sure our coordinate system behaves
	TEST(MeshRendererTest, AxisTest) {
		Environment environment("AxisTest <X-red, Y-green, Z-blue>");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, 1.0f)), "Light", Vector3(2.5f, 2.5f, 2.5f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, 1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
		}
		
		auto createMaterial = [&](uint32_t color) -> Reference<TestMaterial> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
		};
		
		Reference<TriMesh> box = TriMesh::Box(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));

		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
			Reference<TestMaterial> material = createMaterial(0xFF888888);
			Reference<TriMesh> sphere = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.1f, 32, 16);
			Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", sphere, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "X");
			transform->SetLocalPosition(Vector3(0.5f, 0.0f, 0.0f));
			transform->SetLocalScale(Vector3(1.0f, 0.075f, 0.075f));
			Reference<TestMaterial> material = createMaterial(0xFF0000FF);
			Object::Instantiate<MeshRenderer>(transform, "X_Renderer", box, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Y");
			transform->SetLocalPosition(Vector3(0.0f, 0.5f, 0.0f));
			transform->SetLocalScale(Vector3(0.075f, 1.0f, 0.075f));
			Reference<TestMaterial> material = createMaterial(0xFF00FF00);
			Object::Instantiate<MeshRenderer>(transform, "Y_Renderer", box, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Z");
			transform->SetLocalPosition(Vector3(0.0f, 0.0f, 0.5f));
			transform->SetLocalScale(Vector3(0.075f, 0.075f, 1.0f));
			Reference<TestMaterial> material = createMaterial(0xFFFF0000);
			Object::Instantiate<MeshRenderer>(transform, "Z_Renderer", box, material);
		}
	}





	// Creates a bounch of objects and makes them look at the center
	TEST(MeshRendererTest, CenterFacingInstances) {
		Environment environment("Center Facing Instances");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{

			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 0.25f, 0.0f)), "Light", Vector3(2.0f, 2.0f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
		}

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-4.0f, 4.0f);

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 16, 8);
		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
			texture->Unmap(true);
			return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
		}();

		{
			Reference<TriMesh> mesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 64, 32, "Center");
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
			transform->SetLocalScale(Vector3(0.35f));
			Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", mesh, material);
		}

		for (size_t i = 0; i < 2048; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			{
				parent->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
				parent->SetLocalScale(Vector3(0.125f));
				parent->LookAt(Vector3(0.0f));
			}
			{
				Transform* sphereChild = Object::Instantiate<Transform>(parent, "Sphere");
				Reference<MeshRenderer> sphereRenderer = Object::Instantiate<MeshRenderer>(sphereChild, "Sphere_Renderer", sphereMesh, material);
				sphereChild->SetLocalScale(Vector3(0.35f));
				sphereRenderer->MarkStatic(true);
			}
			{
				Transform* cubeChild = Object::Instantiate<Transform>(parent, "Cube");
				Reference<MeshRenderer> cubeRenderer = Object::Instantiate<MeshRenderer>(cubeChild, "Box_Renderer", cubeMesh, material);
				cubeChild->SetLocalPosition(Vector3(0.0f, 0.0f, -1.0f));
				cubeChild->SetLocalScale(Vector3(0.25f, 0.25f, 1.0f));
				cubeRenderer->MarkStatic(true);
			}
			{
				Transform* upIndicator = Object::Instantiate<Transform>(parent, "UpIndicator");
				Reference<MeshRenderer> upRenderer = Object::Instantiate<MeshRenderer>(upIndicator, "UpIndicator_Renderer", cubeMesh, material);
				upIndicator->SetLocalPosition(Vector3(0.0f, 0.5f, -0.5f));
				upIndicator->SetLocalScale(Vector3(0.0625f, 0.5f, 0.0625f));
				upRenderer->MarkStatic(true);
			}
		}
	}





	namespace {
		// Captures all transform fileds
		struct CapturedTransformState {
			Vector3 localPosition;
			Vector3 worldPosition;
			Vector3 localRotation;
			Vector3 worldRotation;
			Vector3 localScale;

			CapturedTransformState(Transform* transform)
				: localPosition(transform->LocalPosition()), worldPosition(transform->WorldPosition())
				, localRotation(transform->LocalEulerAngles()), worldRotation(transform->WorldEulerAngles())
				, localScale(transform->LocalScale()) {}
		};

		// Updates transform component each frame
		class TransformUpdater : public virtual Updatable, public virtual Component {
		private:
			Environment* const m_environment;
			const Function<bool, const CapturedTransformState&, float, Environment*, Transform*> m_updateTransform;
			const CapturedTransformState m_initialTransform;
			const Stopwatch m_stopwatch;

		public:
			inline TransformUpdater(Component* parent, const std::string& name, Environment* environment
				, Function<bool, const CapturedTransformState&, float, Environment*, Transform*> updateTransform)
				: Component(parent, name), m_environment(environment), m_updateTransform(updateTransform)
				, m_initialTransform(parent->GetTransfrom()) {
			}

			inline virtual void Update()override {
				if (!m_updateTransform(m_initialTransform, m_stopwatch.Elapsed(), m_environment, GetTransfrom()))
					GetTransfrom()->Destroy();
			}
		};

		// Moves objects "in orbit" around some point
		bool Swirl(const CapturedTransformState& initialState, float totalTime, Environment*, Transform* transform) {
			const float RADIUS = sqrt(Math::Dot(initialState.worldPosition, initialState.worldPosition));
			if (RADIUS <= 0.0f) return true;
			const Vector3 X = initialState.worldPosition / RADIUS;
			const Vector3 UP = Math::Normalize((Vector3(0.0f, 1.0f, 0.0f) - X * X.y));
			const Vector3 Y = Math::Cross(X, UP);

			auto getPosition = [&](float timePoint) -> Vector3 {
				const float RELATIVE_TIME = timePoint / RADIUS;
				return (X * (float)cos(RELATIVE_TIME) + Y * (float)sin(RELATIVE_TIME)) * RADIUS + Vector3(0.0f, 0.25f, 0.0f);
			};

			const float MOVE_TIME = totalTime * 2.0f;
			transform->SetWorldPosition(getPosition(MOVE_TIME));
			transform->LookAt(getPosition(MOVE_TIME + 0.1f));
			transform->SetLocalScale(
				Vector3((cos(totalTime + initialState.worldPosition.x + initialState.worldPosition.y + initialState.worldPosition.z) + 1.0f) * 0.15f + 0.15f));

			return true;
		};
	}





	// Creates a bounch of objects and moves them around using "Swirl"
	TEST(MeshRendererTest, MovingTransforms) {
#ifdef _WIN32
		Jimara::Test::Memory::MemorySnapshot snapshot;
		auto updateSnapshot = [&](){ snapshot = Jimara::Test::Memory::MemorySnapshot(); };
		auto compareSnapshot = [&]() -> bool { return snapshot.Compare(); };
#else
#ifndef NDEBUG
		size_t snapshot;
		auto updateSnapshot = [&](){ snapshot = Object::DEBUG_ActiveInstanceCount(); };
		auto compareSnapshot = [&]() -> bool { return snapshot == Object::DEBUG_ActiveInstanceCount(); };
#else
		auto updateSnapshot = [&](){};
		auto compareSnapshot = [&]() -> bool { return true; };
#endif 
#endif
		for (size_t i = 0; i < 2; i++) {
			updateSnapshot();
			std::stringstream stream;
			const bool instanced = (i == 1);
			stream << "Moving Transforms [Run " << i << " - " << (instanced ? "INSTANCED" : "NOT_INSTANCED") << "]";
			Environment environment(stream.str().c_str());
			Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
			environment.RenderEngine()->AddRenderer(renderer);

			{
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
			}

			Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
			Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
			}();

			std::mt19937 rng;
			std::uniform_real_distribution<float> disH(-1.5f, 1.5f);
			std::uniform_real_distribution<float> disV(0.0f, 2.0f);
			std::uniform_real_distribution<float> disAngle(-180.0f, 180.0f);

			for (size_t i = 0; i < 512; i++) {
				Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
				parent->SetLocalPosition(Vector3(disH(rng), disV(rng), disH(rng)));
				parent->SetLocalEulerAngles(Vector3(disAngle(rng), disAngle(rng), disAngle(rng)));
				{
					Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
					Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material, instanced);
				}
				{
					Transform* tail = Object::Instantiate<Transform>(parent, "Ball");
					tail->SetLocalPosition(Vector3(0.0f, 0.05f, -0.5f));
					tail->SetLocalScale(Vector3(0.025f, 0.025f, 0.5f));
					Object::Instantiate<MeshRenderer>(tail, "Tail_Renderer", cubeMesh, material, instanced);
				}
				Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
			}
		}
		EXPECT_TRUE(compareSnapshot());
	}





	// Creates geometry, applies "Swirl" movement to them and marks some of the renderers static to let us make sure, the rendered positions are not needlessly updated
	TEST(MeshRendererTest, StaticTransforms) {
		Environment environment("Static transforms (Tailless balls will be locked in place, even though their transforms are alted as well, moving only with camera)");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
		}

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = 0xFFAAAAAA;
			texture->Unmap(true);
			return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
		}();

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

		for (size_t i = 0; i < 128; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			parent->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			{
				Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
				Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material);
			}
			{
				Transform* tail = Object::Instantiate<Transform>(parent, "Ball");
				tail->SetLocalPosition(Vector3(0.0f, 0.05f, -0.5f));
				tail->SetLocalScale(Vector3(0.025f, 0.025f, 0.5f));
				Object::Instantiate<MeshRenderer>(tail, "Tail_Renderer", cubeMesh, material);
			}
			Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
		}
		for (size_t i = 0; i < 128; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			parent->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
			parent->SetLocalScale(Vector3(0.35f));
			{
				Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
				Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material)->MarkStatic(true);
			}
			Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
		}
	}





	namespace {
		// Deforms a planar mesh each frame, generating "moving waves"
		class MeshDeformer : public virtual Component, public virtual Updatable {
		private:
			Environment* const m_environment;
			const Reference<TriMesh> m_mesh;
			const Stopwatch m_stopwatch;

		public:
			inline MeshDeformer(Component* parent, const std::string& name, Environment* env, TriMesh* mesh)
				: Component(parent, name), m_environment(env), m_mesh(mesh) {}

			inline virtual void Update() override {
				float time = m_stopwatch.Elapsed();
				TriMesh::Writer writer(m_mesh);
				for (size_t i = 0; i < writer.Verts().size(); i++) {
					MeshVertex& vertex = writer.Verts()[i];
					auto getY = [&](float x, float z) { return cos((time + ((x * x) + (z * z))) * 10.0f) * 0.05f; };
					vertex.position.y = getY(vertex.position.x, vertex.position.z);
					Vector3 dx = Vector3(vertex.position.x + 0.01f, 0.0f, vertex.position.z);
					dx.y = getY(dx.x, dx.z);
					Vector3 dz = Vector3(vertex.position.x, 0.0f, vertex.position.z + 0.01f);
					dz.y = getY(dz.x, dz.z);
					vertex.normal = Math::Normalize(Math::Cross(dz - vertex.position, dx - vertex.position));
				}
			}
		};
	}





	// Creates a planar mesh and applies per-frame deformation
	TEST(MeshRendererTest, MeshDeformation) {
		Environment environment("Mesh Deformation");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
		}

		Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		{
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
			}();

			Object::Instantiate<MeshRenderer>(Object::Instantiate<Transform>(environment.RootObject(), "Transform"), "MeshRenderer", planeMesh, material)->MarkStatic(true);
		}

		Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);
	}





	// Creates a planar mesh, applies per-frame deformation and moves the thing around
	TEST(MeshRendererTest, MeshDeformationAndTransform) {
		Environment environment("Mesh Deformation And Transform");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
		}

		Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);

		Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
		{
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
			}();

			Object::Instantiate<MeshRenderer>(transform, "MeshRenderer", planeMesh, material);
		}

		auto move = [](const CapturedTransformState&, float totalTime, Environment*, Transform* transform) -> bool {
			transform->SetLocalPosition(Vector3(cos(totalTime), 0.0f, sin(totalTime)));
			transform->SetLocalScale(Vector3((cos(totalTime * 0.5f) + 1.0f) * 0.5f + 0.15f));
			return true;
		};

		Object::Instantiate<TransformUpdater>(transform, "TransformUpdater", &environment,
			Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
	}





	namespace {
		// Generates texture contents each frame
		class TextureGenerator : public virtual Component, public virtual Updatable {
		private:
			Environment* const m_environment;
			const Reference<Graphics::ImageTexture> m_texture;
			const Stopwatch m_stopwatch;

		public:
			inline TextureGenerator(Component* parent, const std::string& name, Environment* env, Graphics::ImageTexture* texture)
				: Component(parent, name), m_environment(env), m_texture(texture) {
			}

			inline virtual void Update() override {
				float time = m_stopwatch.Elapsed();
				const Size3 TEXTURE_SIZE = m_texture->Size();
				uint32_t* data = static_cast<uint32_t*>(m_texture->Map());
				uint32_t timeOffsetX = (static_cast<uint32_t>(time * 16));
				uint32_t timeOffsetY = (static_cast<uint32_t>(time * 48));
				uint32_t timeOffsetZ = (static_cast<uint32_t>(time * 32));
				for (uint32_t y = 0; y < TEXTURE_SIZE.y; y++) {
					for (uint32_t x = 0; x < TEXTURE_SIZE.x; x++) {
						uint8_t red = static_cast<uint8_t>(x + timeOffsetX);
						uint8_t green = static_cast<uint8_t>(y - timeOffsetY);
						uint8_t blue = static_cast<uint8_t>((x + timeOffsetZ) ^ y);
						uint8_t alpha = static_cast<uint8_t>(255u);
						data[x] = (red << 24) + (green << 16) + (blue << 8) + alpha;
					}
					data += TEXTURE_SIZE.x;
				}
				m_texture->Unmap(true);
			}
		};
	}





	// Creates a planar mesh and applies a texture that changes each frame
	TEST(MeshRendererTest, DynamicTexture) {
		Environment environment("Dynamic Texture");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
		}

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(128, 128, 1), 1, true);
		{
			texture->Map();
			texture->Unmap(true);
			Object::Instantiate<TextureGenerator>(environment.RootObject(), "TextureGenerator", &environment, texture);
		}

		{
			Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f));
			Reference<Material> material = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
			Object::Instantiate<MeshRenderer>(Object::Instantiate<Transform>(environment.RootObject(), "Transform"), "MeshRenderer", planeMesh, material);
		}
	}





	// Creates a planar mesh, applies per-frame deformation, a texture that changes each frame and moves the thing around
	TEST(MeshRendererTest, DynamicTextureWithMovementAndDeformation) {
		Environment environment("Dynamic Texture With Movement And Mesh Deformation");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
		}

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(128, 128, 1), 1, true);
		{
			texture->Map();
			texture->Unmap(true);
			Object::Instantiate<TextureGenerator>(environment.RootObject(), "TextureGenerator", &environment, texture);
		}

		Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");

		Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		{
			Reference<Material> material = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), texture);
			Object::Instantiate<MeshRenderer>(transform, "MeshRenderer", planeMesh, material);
		}

		Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);

		auto move = [](const CapturedTransformState&, float totalTime, Environment*, Transform* transform) -> bool {
			transform->SetLocalPosition(Vector3(cos(totalTime), 0.0f, sin(totalTime)));
			return true;
		};

		Object::Instantiate<TransformUpdater>(transform, "TransformUpdater", &environment,
			Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
	}





	// Loads sample scene from .obj file
	TEST(MeshRendererTest, LoadedGeometry) {
		Environment environment("Loading Geometry...");
		Reference<Graphics::ImageRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			static auto baseMove = [](const CapturedTransformState&, float totalTime, Environment*, Transform* transform) -> bool {
				transform->SetLocalPosition(Vector3(cos(totalTime) * 4.0f, 1.0f, sin(totalTime) * 4.0f));
				return true;
			};
			static const float rotationSpeed = -1.25f;
			{
				auto move = [](const CapturedTransformState& state, float totalTime, Environment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((sin(totalTime * 4.0f) + 1.0f) * 4.0f, cos(totalTime * 2.0f) + 1.0f, 2.0f));
					return baseMove(state, totalTime * rotationSpeed, env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(4.0f, 1.0f, 4.0f)), "Light", Vector3(8.0f, 2.0f, 2.0f)), 
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
			}
			{
				auto move = [](const CapturedTransformState& state, float totalTime, Environment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3(2.0f, (sin(totalTime * 2.0f) + 1.0f) * 4.0f, (cos(totalTime * 4.0f) + 1.0f) * 2.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(90.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-4.0f, 1.0f, -4.0f)), "Light", Vector3(2.0f, 8.0f, 2.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
			}
			{
				auto move = [](const CapturedTransformState& state, float totalTime, Environment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((cos(totalTime * 3.0f) + 1.0f) * 1.0f, 2.0f, (sin(totalTime * 2.5f) + 1.0f) * 4.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(180.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(4.0f, 1.0f, -4.0f)), "Light", Vector3(2.0f, 2.0f, 8.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
			}
			{
				auto move = [](const CapturedTransformState& state, float totalTime, Environment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((sin(totalTime * 4.25f) + 1.0f) * 4.0f, 2.0f, (cos(totalTime * 7.5f) + 1.0f) * 4.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(270.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-4.0f, 1.0f, 4.0f)), "Light", Vector3(4.0f, 2.0f, 4.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Environment*, Transform*>(move));
			}
			Object::Instantiate<DirectionalLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, -2.0f, 0.0f)), "Light", Vector3(1.5f, 0.0f, 0.0f))
				->GetTransfrom()->LookAt(Vector3(0.0f, 0.0f, 0.0f));
			Object::Instantiate<DirectionalLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 2.0f, 2.0f)), "Light", Vector3(0.0f, 0.125f, 0.125f))
				->GetTransfrom()->LookAt(Vector3(0.0f, 0.0f, 0.0f));
		}

		Reference<Graphics::ImageTexture> whiteTexture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
		{
			(*static_cast<uint32_t*>(whiteTexture->Map())) = 0xFFFFFFFF;
			whiteTexture->Unmap(true);
		}
		Reference<Material> whiteMaterial = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), whiteTexture);

		std::vector<Reference<TriMesh>> geometry = TriMesh::FromOBJ("Assets/Meshes/Bear/ursus_proximus.obj");
		std::vector<MeshRenderer*> renderers;

		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
			transform->SetLocalPosition(Vector3(0.0f, -0.5f, 0.0f));
			transform->SetLocalScale(Vector3(0.75f));
			for (size_t i = 0; i < geometry.size(); i++) {
				const TriMesh* mesh = geometry[i];
				renderers.push_back(Object::Instantiate<MeshRenderer>(transform, TriMesh::Reader(mesh).Name(), mesh, whiteMaterial));
			}
			environment.SetWindowName("Loading texture...");
		}

		Reference<Graphics::ImageTexture> bearTexture = Graphics::ImageTexture::LoadFromFile(
			environment.RootObject()->Context()->Graphics()->Device(), "Assets/Meshes/Bear/bear_diffuse.png", true);
		Reference<Material> bearMaterial = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), bearTexture);
		environment.SetWindowName("Applying texture...");

		for (size_t i = 0; i < geometry.size(); i++)
			if (TriMesh::Reader(geometry[i]).Name() == "bear")
				renderers[i]->SetMaterial(bearMaterial);

		environment.SetWindowName("Loaded scene");
	}
}
