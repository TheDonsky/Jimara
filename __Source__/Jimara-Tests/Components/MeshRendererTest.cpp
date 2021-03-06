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
					stream << std::fixed << std::setprecision(6)
						<< "[S_DT:" << (m_smoothDeltaTime * 1000.0f) << "; S_FPS:" << (1.0f / m_smoothDeltaTime)
						<< "; DT:" << (m_deltaTime * 0.001f) << "; FPS:" << (1.0f / m_deltaTime) << " " << m_windowName;
					const float timeLeft = closingIn;
					if ((timeLeft >= 0.0f) && sizeChangeCount > 0)
						stream << std::setprecision(2) << " [Closing in " << timeLeft << " seconds, unless resized]";
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
					const uint64_t DESIRED_DELTA_MICROSECONDS = 8000u;
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
					m_window = OS::Window::Create(logger, appInfo->ApplicationName());
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
					const Vector3 position = Vector3(2.0f, 1.5f + 1.2f * cos(time * glm::radians(15.0f)), 2.0f);
					const Vector3 target = Vector3(0.0f, 0.5f, 0.0f);
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

	TEST(MeshRendererTest, BasicScene) {
		Environment environment("BasicScene");
		Reference<TestRenderer> renderer = Object::Instantiate<TestRenderer>(environment.RootObject()->Context());
		environment.RenderEngine()->AddRenderer(renderer);

		{
			std::vector<Light> lights = {
				{ Vector3(0.0f, 0.25f, 0.0f),Vector3(24.0f, 24.0f, 24.0f) },
				{ Vector3(4.0f, 4.0f, 4.0f),Vector3(8.0f, 8.0f, 8.0f) },
				{ Vector3(-4.0f, -4.0f, -4.0f),Vector3(2.0f, 4.0f, 8.0f) },
				{ Vector3(4.0f, 0.0f, -4.0f),Vector3(8.0f, 4.0f, 2.0f) },
				{ Vector3(-4.0f, 0.0f, 4.0f),Vector3(4.0f, 8.0f, 4.0f) }
			};
			renderer->SetLights(lights);
		}

		Reference<Graphics::ImageTexture> whiteTexture = environment.RootObject()->Context()->GraphicsDevice()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(16, 16, 1), 1, true);
		{
			const Size3 TEXTURE_SIZE = whiteTexture->Size();
			const uint32_t COLOR = 0xFFFFFFFF;
			uint32_t* data = static_cast<uint32_t*>(whiteTexture->Map());
			for (uint32_t i = 0; i < TEXTURE_SIZE.x * TEXTURE_SIZE.y; i++)
				data[i] = COLOR;
			whiteTexture->Unmap(true);
		}

		Reference<Material> whiteMaterial = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), whiteTexture);

		Reference<Graphics::ImageTexture> bearTexture = Graphics::ImageTexture::LoadFromFile(
			environment.RootObject()->Context()->GraphicsDevice(), "Assets/Meshes/Bear/bear_diffuse.png", true);
		Reference<Material> bearMaterial = Object::Instantiate<TestMaterial>(environment.RootObject()->Context()->Context()->ShaderCache(), bearTexture);

		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Plane");
			Reference<TriMesh> mesh = TriMesh::Plane(Vector3(0.0f, 0.0f, 0.0f));
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(transform, "Plane_Renderer", mesh, bearMaterial);
		}

		Reference<TriMesh> sphereMesh = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 32, 16);
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Sphere_z");
			transform->SetLocalPosition(Vector3(0.0f, 0.0f, 1.0f));
			transform->SetLocalScale(Vector3(0.25f));
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(transform, "Sphere_Renderer", sphereMesh, bearMaterial);
		}

		Reference<TriMesh> cubeMesh = TriMesh::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Box_x");
			transform->SetLocalPosition(Vector3(1.0f, 0.0f, 0.0f));
			transform->SetLocalScale(Vector3(0.25f));
			Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(transform, "Box_Renderer", cubeMesh, bearMaterial);
		}

		std::mt19937 rng;
		std::uniform_real_distribution<float> dis(-8.0f, 8.0f);

		for (size_t i = 0; i < 8192; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			{
				parent->SetLocalPosition(Vector3(dis(rng), dis(rng), dis(rng)));
				parent->SetLocalScale(Vector3(0.125f));
				parent->LookAt(Vector3(0.0f));
			}
			{
				Transform* sphereChild = Object::Instantiate<Transform>(parent, "Sphere");
				Reference<MeshRenderer> sphereRenderer = Object::Instantiate<MeshRenderer>(sphereChild, "Sphere_Renderer", sphereMesh, whiteMaterial);
				sphereChild->SetLocalScale(Vector3(0.35f));
				sphereRenderer->MarkStatic(true);
			}
			{
				Transform* cubeChild = Object::Instantiate<Transform>(parent, "Cube");
				Reference<MeshRenderer> cubeRenderer = Object::Instantiate<MeshRenderer>(cubeChild, "Box_Renderer", cubeMesh, whiteMaterial);
				cubeChild->SetLocalPosition(Vector3(0.0f, 0.0f, -1.0f));
				cubeChild->SetLocalScale(Vector3(0.25f, 0.25f, 1.0f));
				cubeRenderer->MarkStatic(true);
			}
			{
				Transform* upIndicator = Object::Instantiate<Transform>(parent, "UpIndicator");
				Reference<MeshRenderer> upRenderer = Object::Instantiate<MeshRenderer>(upIndicator, "UpIndicator_Renderer", cubeMesh, whiteMaterial);
				upIndicator->SetLocalPosition(Vector3(0.0f, 0.5f, -0.5f));
				upIndicator->SetLocalScale(Vector3(0.0625f, 0.5f, 0.0625f));
				upRenderer->MarkStatic(true);
			}
		}
	}
}
