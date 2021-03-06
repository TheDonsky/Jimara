#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/Logging/StreamLogger.h"
#include "Components/MeshRenderer.h"
#include "Environment/Scene.h"
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>


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

			std::mutex m_renderLock;
			EventInstance<Environment*, float> m_onAsynchUpdate;
			std::thread m_asynchUpdateThread;
			std::atomic<bool> m_dead;

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
				{
					std::unique_lock<std::mutex> lock(m_renderLock);
					m_surfaceRenderEngine->Update();
				}
			}

			inline void WindowResized(OS::Window*) { 
				if (sizeChangeCount > 0)
					sizeChangeCount--;
			}

			inline static void AsynchUpdateThread(Environment* self) {
				Stopwatch stopwatch;
				float lastTime = stopwatch.Elapsed();
				float deltaTime = 0.0f;
				while (true) {
					{
						std::unique_lock<std::mutex> lock(self->m_renderLock);
						if (self->m_dead) break;
						float now = stopwatch.Elapsed();
						deltaTime = (now - lastTime);
						lastTime = now;
						self->m_onAsynchUpdate(self, deltaTime);
					}
					const uint64_t DESIRED_DELTA_MICROSECONDS = 10000u;
					uint64_t deltaMicroseconds = static_cast<uint64_t>(static_cast<double>(deltaTime) * 1000000.0);
					if (DESIRED_DELTA_MICROSECONDS > deltaMicroseconds) {
						uint64_t sleepTime = (DESIRED_DELTA_MICROSECONDS - deltaMicroseconds);
						std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
					}
				}
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
					m_window->OnUpdate() += Callback<OS::Window*>(&Environment::OnUpdate, this);
					m_window->OnSizeChanged() += Callback<OS::Window*>(&Environment::WindowResized, this);
				}
				else if (graphicsInstance->PhysicalDeviceCount() > 0)
					graphicsDevice = graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();
				if (graphicsDevice != nullptr) {
					Reference<AppContext> appContext = Object::Instantiate<AppContext>(graphicsDevice);
					m_scene = Object::Instantiate<Scene>(appContext);
				}
				else logger->Fatal("Environment could not be set up due to the insufficient hardware!");
				m_asynchUpdateThread = std::thread(&Environment::AsynchUpdateThread, this);
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

			inline float FrameTime()const { return m_deltaTime; }

			inline float SmoothFrameTime()const { return m_smoothDeltaTime; }

			inline Event<Environment*, float>& OnAsynchUpdate() { return m_onAsynchUpdate; }

			inline Component* RootObject()const { return m_scene->RootObject(); }

			inline Graphics::RenderEngine* RenderEngine()const { return m_surfaceRenderEngine; }
		};



		class EnvironmentBinding : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
		public:
			inline virtual bool SetByEnvironment()const override { return true; }
			
			inline virtual size_t ConstantBufferCount()const override { return 1; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::VERTEX), 0 }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return nullptr; }

			inline virtual size_t StructuredBufferCount()const override { return 1; }
			inline BindingInfo StructuredBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::FRAGMENT), 1 }; }
			inline Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

			inline size_t TextureSamplerCount()const override { return 0; }
			inline BindingInfo TextureSamplerInfo(size_t index)const override { return BindingInfo(); }
			inline Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return nullptr; }

			static EnvironmentBinding* Instance() { static EnvironmentBinding binding; return &binding; }
		};

		class TestMaterial : public virtual Material {
		private:
			const Reference<Graphics::Shader> m_vertexShader;
			const Reference<Graphics::Shader> m_fragmentShader;
			const Reference<Graphics::TextureSampler> m_sampler;

		public:
			inline TestMaterial(Graphics::ShaderCache* cache, Graphics::Texture* texture)
				: m_vertexShader(cache->GetShader("Shaders/SampleMeshShader.vert.spv"))
				, m_fragmentShader(cache->GetShader("Shaders/SampleMeshShader.frag.spv"))
				, m_sampler(texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler()) {}

			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor* EnvironmentDescriptor()const override { return EnvironmentBinding::Instance(); }

			inline virtual Reference<Graphics::Shader> VertexShader()const override { return m_vertexShader; }
			inline virtual Reference<Graphics::Shader> FragmentShader()const override { return m_fragmentShader; }

			inline virtual bool SetByEnvironment()const override { return false; }

			inline virtual size_t ConstantBufferCount()const override { return 0; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return BindingInfo(); }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return nullptr; }

			inline virtual size_t StructuredBufferCount()const override { return 0; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo(); }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

			inline virtual size_t TextureSamplerCount()const override { return 1; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::FRAGMENT), 0 }; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return m_sampler; }
		};

		struct Light {
			alignas(16) Vector3 position;
			alignas(16) Vector3 color;
		};

		class TestRenderer : public virtual Graphics::ImageRenderer {
		private:

			class EnvironmentPipeline : public virtual Graphics::PipelineDescriptor, public virtual EnvironmentBinding {
			private:
				const Reference<Graphics::GraphicsDevice> m_device;

				Graphics::BufferReference<Matrix4> m_cameraTransform;
				Graphics::ArrayBufferReference<Light> m_lightBuffer;
				Stopwatch m_stopwatch;

			public:
				inline EnvironmentPipeline(Graphics::GraphicsDevice* device)
					: m_device(device)
					, m_cameraTransform(device->CreateConstantBuffer<Matrix4>())
					, m_lightBuffer(device->CreateArrayBuffer<Light>(0)) {}

				inline virtual bool SetByEnvironment()const override { return false; }
				inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return m_cameraTransform; }
				inline Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return m_lightBuffer; }

				inline virtual size_t BindingSetCount()const override { return 1; }
				inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
					return (const Graphics::PipelineDescriptor::BindingSetDescriptor*)this;
				}

				inline void UpdateCamera(Size2 imageSize) {
					Matrix4 projection = glm::perspective(glm::radians(64.0f), (float)imageSize.x / (float)imageSize.y, 0.001f, 10000.0f);
					projection[2] *= -1.0f;
					float time = m_stopwatch.Elapsed();
					const Vector3 position = Vector3(1.5f, 1.0f + 0.8f * cos(time * glm::radians(15.0f)), 1.5f);
					const Vector3 target = Vector3(0.0f, 0.25f, 0.0f);
					m_cameraTransform.Map() = (projection * Math::Inverse(Math::LookAt(position, target)) * Math::MatrixFromEulerAngles(Vector3(0.0f, time * 10.0f, 0.0f)));
					m_cameraTransform->Unmap(true);
				}

				inline void SetLights(const std::vector<Light>& lights) {
					Graphics::ArrayBufferReference<Light> lightBuffer = m_device->CreateArrayBuffer<Light>(lights.size());
					memcpy(lightBuffer.Map(), lights.data(), sizeof(Light) * lights.size());
					lightBuffer->Unmap(true);
					Graphics::PipelineDescriptor::WriteLock lock(this);
					m_lightBuffer = lightBuffer;
				}
			};

			class Data : public virtual Object {
			private:
				const Reference<SceneContext> m_context;
				const Reference<Graphics::RenderEngineInfo> m_engineInfo;
				const Reference<EnvironmentPipeline> m_environmentDescriptor;
				
				Reference<Graphics::RenderPass> m_renderPass;
				std::vector<Reference<Graphics::FrameBuffer>> m_frameBuffers;

				Reference<Graphics::Pipeline> m_environmentPipeline;
				Reference<Graphics::GraphicsPipelineSet> m_pipelineSet;

				void AddGraphicsPipelines(const Reference<Graphics::GraphicsPipeline::Descriptor>* descriptors, size_t count, Graphics::GraphicsObjectSet*) {
					m_pipelineSet->AddPipelines(descriptors, count);
				};

				void RemoveGraphicsPipelines(const Reference<Graphics::GraphicsPipeline::Descriptor>* descriptors, size_t count, Graphics::GraphicsObjectSet*) {
					m_pipelineSet->RemovePipelines(descriptors, count);
				};

			public:
				inline Data(SceneContext* context, Graphics::RenderEngineInfo* engineInfo, EnvironmentPipeline* environmentDescriptor)
					: m_context(context), m_engineInfo(engineInfo), m_environmentDescriptor(environmentDescriptor) {

					Graphics::Texture::PixelFormat pixelFormat = engineInfo->ImageFormat();

					Reference<Graphics::TextureView> colorAttachment = engineInfo->Device()->CreateMultisampledTexture(
						Graphics::Texture::TextureType::TEXTURE_2D, pixelFormat, Size3(engineInfo->ImageSize(), 1), 1, Graphics::Texture::Multisampling::MAX_AVAILABLE)
						->CreateView(Graphics::TextureView::ViewType::VIEW_2D);

					Reference<Graphics::TextureView> depthAttachment = engineInfo->Device()->CreateMultisampledTexture(
						Graphics::Texture::TextureType::TEXTURE_2D, engineInfo->Device()->GetDepthFormat()
						, colorAttachment->TargetTexture()->Size(), 1, colorAttachment->TargetTexture()->SampleCount())
						->CreateView(Graphics::TextureView::ViewType::VIEW_2D);

					m_renderPass = m_context->GraphicsDevice()->CreateRenderPass(
						colorAttachment->TargetTexture()->SampleCount(), 1, &pixelFormat, depthAttachment->TargetTexture()->ImageFormat(), true);

					for (size_t i = 0; i < m_engineInfo->ImageCount(); i++) {
						Reference<Graphics::TextureView> resolveView = engineInfo->Image(i)->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveView));
					}

					m_environmentPipeline = m_context->GraphicsDevice()->CreateEnvironmentPipeline(m_environmentDescriptor, m_engineInfo->ImageCount());

					m_pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(m_context->GraphicsDevice()->GraphicsQueue(), m_renderPass, engineInfo->ImageCount());

					m_context->GraphicsPipelineSet()->AddChangeCallbacks(
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&Data::AddGraphicsPipelines, this), 
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&Data::RemoveGraphicsPipelines, this));
				}

				inline virtual ~Data() {
					m_context->GraphicsPipelineSet()->RemoveChangeCallbacks(
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&Data::AddGraphicsPipelines, this),
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&Data::RemoveGraphicsPipelines, this));
				}

				inline void Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo) {
					Graphics::PrimaryCommandBuffer* buffer = dynamic_cast<Graphics::PrimaryCommandBuffer*>(bufferInfo.commandBuffer);
					if (buffer == nullptr) {
						m_context->Log()->Fatal("MeshRenderTest - Renderer expected a primary command buffer, but did not get one!");
						return;
					}

					m_environmentDescriptor->UpdateCamera(m_engineInfo->ImageSize());

					Graphics::FrameBuffer* const frameBuffer = m_frameBuffers[bufferInfo.inFlightBufferId];
					const Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);

					m_renderPass->BeginPass(bufferInfo.commandBuffer, frameBuffer, &CLEAR_VALUE, true);
					m_pipelineSet->ExecutePipelines(buffer, bufferInfo.inFlightBufferId, frameBuffer, m_environmentPipeline);
					m_renderPass->EndPass(bufferInfo.commandBuffer);
				}
			};


		private:
			Reference<SceneContext> m_context;
			Reference<EnvironmentPipeline> m_environmentDescriptor;


		public:
			TestRenderer(SceneContext* context) : m_context(context), m_environmentDescriptor(Object::Instantiate<EnvironmentPipeline>(context->GraphicsDevice())) {}

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
				return Object::Instantiate<Data>(m_context, engineInfo, m_environmentDescriptor);
			}

			inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) override {
				dynamic_cast<Data*>(engineData)->Render(bufferInfo);
			}

			inline void SetLights(const std::vector<Light>& lights) { m_environmentDescriptor->SetLights(lights); }
		};
	}





	// Renders axis-facing cubes to make sure our coordinate system behaves
	TEST(MeshRendererTest, AxisTest) {
		Environment environment("AxisTest <X-red, Y-green, Z-blue>");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(1.0f, 1.0f, 1.0f), Vector3(2.5f, 2.5f, 2.5f) },
				{ Vector3(-1.0f, 1.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f) },
				{ Vector3(1.0f, 1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f) },
				{ Vector3(-1.0f, 1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f) }
			};
			renderer->SetLights(lights);
		}
		
		auto createMaterial = [&](uint32_t color) -> Reference<TestMaterial> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(0.0f, 0.25f, 0.0f),Vector3(2.0f, 2.0f, 2.0f) },
				{ Vector3(2.0f, 0.25f, 2.0f),Vector3(2.0f, 0.25f, 0.25f) },
				{ Vector3(2.0f, 0.25f, -2.0f),Vector3(0.25f, 2.0f, 0.25f) },
				{ Vector3(-2.0f, 0.25f, 2.0f),Vector3(0.25f, 0.25f, 2.0f) },
				{ Vector3(-2.0f, 0.25f, -2.0f),Vector3(2.0f, 4.0f, 1.0f) },
				{ Vector3(0.0f, 2.0f, 0.0f),Vector3(1.0f, 4.0f, 2.0f) }
			};
			renderer->SetLights(lights);
		}

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-4.0f, 4.0f);

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 16, 8);
		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		class TransformUpdater : public virtual Component {
		private:
			Environment* const m_environment;
			const Function<bool, const CapturedTransformState&, float, Environment*, Transform*> m_updateTransform;
			const CapturedTransformState m_initialTransform;
			const Stopwatch m_stopwatch;

			inline void OnUpdate(Environment*, float) {
				if (!m_updateTransform(m_initialTransform, m_stopwatch.Elapsed(), m_environment, GetTransfrom()))
					GetTransfrom()->Destroy();
			}

			inline void Unsubscribe(const Component*) {
				m_environment->OnAsynchUpdate() -= Callback<Environment*, float>(&TransformUpdater::OnUpdate, this);
			}

		public:
			inline TransformUpdater(Component* parent, const std::string& name, Environment* environment
				, Function<bool, const CapturedTransformState&, float, Environment*, Transform*> updateTransform)
				: Component(parent, name), m_environment(environment), m_updateTransform(updateTransform)
				, m_initialTransform(parent->GetTransfrom()) {
				m_environment->OnAsynchUpdate() += Callback<Environment*, float>(&TransformUpdater::OnUpdate, this);
				OnDestroyed() += Callback<const Component*>(&TransformUpdater::Unsubscribe, this);
			}

			inline virtual ~TransformUpdater() { Unsubscribe(this); }
		};

		// Moves objects "in orbit" around some point
		bool Swirl(const CapturedTransformState& initialState, float totalTime, Environment*, Transform* transform) {
			const float RADIUS = sqrt(Math::Dot(initialState.worldPosition, initialState.worldPosition));
			if (RADIUS <= 0.0f) return true;
			const Vector3 X = initialState.worldPosition / RADIUS;
			const Vector3 UP = Math::Normalize((Vector3(0.0f, 1.0f, 0.0f) - X * X.y));
			const Vector3 Y = Math::Cross(X, UP);

			auto getPosition = [&](float timePoint) {
				const float RELATIVE_TIME = timePoint / RADIUS;
				return (X * cos(RELATIVE_TIME) + Y * sin(RELATIVE_TIME)) * RADIUS + Vector3(0.0f, 0.25f, 0.0f);
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
		Environment environment("Moving Transforms");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(2.0f, 0.25f, 2.0f),Vector3(2.0f, 0.25f, 0.25f) },
				{ Vector3(2.0f, 0.25f, -2.0f),Vector3(0.25f, 2.0f, 0.25f) },
				{ Vector3(-2.0f, 0.25f, 2.0f),Vector3(0.25f, 0.25f, 2.0f) },
				{ Vector3(-2.0f, 0.25f, -2.0f),Vector3(2.0f, 4.0f, 1.0f) },
				{ Vector3(0.0f, 2.0f, 0.0f),Vector3(1.0f, 4.0f, 2.0f) }
			};
			renderer->SetLights(lights);
		}

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
				Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material);
			}
			{
				Transform* tail = Object::Instantiate<Transform>(parent, "Ball");
				tail->SetLocalPosition(Vector3(0.0f, 0.05f, -0.5f));
				tail->SetLocalScale(Vector3(0.025f, 0.025f, 0.5f));
				Object::Instantiate<MeshRenderer>(tail, "Tail_Renderer", cubeMesh, material);
			}
			Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
		};
	}





	// Creates geometry, applies "Swirl" movement to them and marks some of the renderers static to let us make sure, the rendered positions are not needlessly updated
	TEST(MeshRendererTest, StaticTransforms) {
		Environment environment("Static transforms (Tailless balls will be locked in place, even though their transforms are alted as well, moving only with camera)");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(2.0f, 0.25f, 2.0f),Vector3(2.0f, 0.25f, 0.25f) },
				{ Vector3(2.0f, 0.25f, -2.0f),Vector3(0.25f, 2.0f, 0.25f) },
				{ Vector3(-2.0f, 0.25f, 2.0f),Vector3(0.25f, 0.25f, 2.0f) },
				{ Vector3(-2.0f, 0.25f, -2.0f),Vector3(2.0f, 4.0f, 1.0f) },
				{ Vector3(0.0f, 2.0f, 0.0f),Vector3(1.0f, 4.0f, 2.0f) }
			};
			renderer->SetLights(lights);
		}

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		class MeshDeformer : public virtual Component {
		private:
			Environment* const m_environment;
			const Reference<TriMesh> m_mesh;
			const Stopwatch m_stopwatch;

			inline void OnUpdate(Environment* environemnt , float) {
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

			inline void Unsubscribe(const Component*) {
				m_environment->OnAsynchUpdate() -= Callback<Environment*, float>(&MeshDeformer::OnUpdate, this);
			}

		public:
			inline MeshDeformer(Component* parent, const std::string& name, Environment* env, TriMesh* mesh)
				: Component(parent, name), m_environment(env), m_mesh(mesh) {
				m_environment->OnAsynchUpdate() += Callback<Environment*, float>(&MeshDeformer::OnUpdate, this);
				OnDestroyed() += Callback<const Component*>(&MeshDeformer::Unsubscribe, this);
			}

			inline virtual ~MeshDeformer() { Unsubscribe(this); }
		};
	}





	// Creates a planar mesh and applies per-frame deformation
	TEST(MeshRendererTest, MeshDeformation) {
		Environment environment("Mesh Deformation");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = { { Vector3(0.0f, 1.0f, 0.0f),Vector3(1.0f, 1.0f, 1.0f) } };
			renderer->SetLights(lights);
		}

		Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		{
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = { { Vector3(0.0f, 1.0f, 0.0f),Vector3(1.0f, 1.0f, 1.0f) } };
			renderer->SetLights(lights);
		}

		Reference<TriMesh> planeMesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);

		Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
		{
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		class TextureGenerator : public virtual Component {
		private:
			Environment* const m_environment;
			const Reference<Graphics::ImageTexture> m_texture;
			const Stopwatch m_stopwatch;

			inline void OnUpdate(Environment* environemnt, float) {
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

			inline void Unsubscribe(const Component*) {
				m_environment->OnAsynchUpdate() -= Callback<Environment*, float>(&TextureGenerator::OnUpdate, this);
			}

		public:
			inline TextureGenerator(Component* parent, const std::string& name, Environment* env, Graphics::ImageTexture* texture)
				: Component(parent, name), m_environment(env), m_texture(texture) {
				m_environment->OnAsynchUpdate() += Callback<Environment*, float>(&TextureGenerator::OnUpdate, this);
				OnDestroyed() += Callback<const Component*>(&TextureGenerator::Unsubscribe, this);
			}

			inline virtual ~TextureGenerator() { Unsubscribe(this); }
		};
	}





	// Creates a planar mesh and applies a texture that changes each frame
	TEST(MeshRendererTest, DynamicTexture) {
		Environment environment("Dynamic Texture");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = { { Vector3(0.0f, 1.0f, 0.0f),Vector3(1.0f, 1.0f, 1.0f) } };
			renderer->SetLights(lights);
		}

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = { { Vector3(0.0f, 1.0f, 0.0f),Vector3(1.0f, 1.0f, 1.0f) } };
			renderer->SetLights(lights);
		}

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(4.0f, 4.0f, 4.0f),Vector3(8.0f, 8.0f, 8.0f) },
				{ Vector3(-4.0f, -4.0f, -4.0f),Vector3(2.0f, 4.0f, 8.0f) },
				{ Vector3(4.0f, 0.0f, -4.0f),Vector3(8.0f, 4.0f, 2.0f) },
				{ Vector3(-4.0f, 0.0f, 4.0f),Vector3(4.0f, 8.0f, 4.0f) }
			};
			renderer->SetLights(lights);
		}

		Reference<Graphics::ImageTexture> whiteTexture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
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
			environment.RootObject()->Context()->GraphicsDevice(), "Assets/Meshes/Bear/bear_diffuse.png", true);
		Reference<Material> bearMaterial = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), bearTexture);
		environment.SetWindowName("Applying texture...");

		for (size_t i = 0; i < geometry.size(); i++)
			if (TriMesh::Reader(geometry[i]).Name() == "bear")
				renderers[i]->SetMaterial(bearMaterial);

		environment.SetWindowName("Loaded scene");
	}
}
