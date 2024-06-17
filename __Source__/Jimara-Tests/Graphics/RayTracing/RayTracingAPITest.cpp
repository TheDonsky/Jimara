#include "../../GtestHeaders.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Data/Geometry/MeshConstants.h"
#include "Data/Geometry/GraphicsMesh.h"
#include "../../CountingLogger.h"
#include "Core/Synch/Semaphore.h"
#include "Core/Stopwatch.h"
#include <queue>


namespace Jimara {
	namespace Graphics {
		namespace {
			class RayTracingAPITest_Context {
			private:
				Reference<Jimara::Test::CountingLogger> m_log = Object::Instantiate<Jimara::Test::CountingLogger>();
				Reference<Application::AppInformation> m_appInfo = Object::Instantiate<Application::AppInformation>("RayTracingAPITest");
				Reference<GraphicsInstance> m_graphicsInstance;
				size_t m_warningCount = {};
				size_t m_failureCount = {};

			public:
				inline static RayTracingAPITest_Context Create() {
					RayTracingAPITest_Context context;
					context.m_graphicsInstance = GraphicsInstance::Create(context.m_log, context.m_appInfo);
					context.m_warningCount = context.m_log->NumWarning();
					context.m_failureCount = context.m_log->Numfailures();
					return context;
				}

				inline Jimara::Test::CountingLogger* Log()const { return m_log; }

				inline operator bool()const { return m_graphicsInstance != nullptr; }

				inline bool AnythingFailed()const {
					return
						m_warningCount != m_log->NumWarning() ||
						m_failureCount != m_log->Numfailures();
				}

				inline bool RTDeviceFound() {
					for (auto it = begin(); it != end(); ++it)
						return true;
					return false;
				}

				inline Reference<SPIRV_Binary> LoadShader(const std::string_view& shaderName) {
					static const std::string path = "Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/RayTracing/";
					Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPVCached(path + std::string(shaderName) + ".spv", m_log);
					if (binary == nullptr)
						m_log->Fatal("BindlessRenderer::EngineData - Failed to load shader module for \'", shaderName, "\'!");
					return binary;
				};

				struct iterator {
					size_t index = ~size_t(0u);
					const RayTracingAPITest_Context* context = nullptr;

					inline bool operator==(const iterator& other)const { return index == other.index; }
					inline bool operator!=(const iterator& other)const { return !((*this) == other); }

					inline Graphics::PhysicalDevice* PhysicalDevice()const {
						if (context == nullptr || context->m_graphicsInstance == nullptr || index >= context->m_graphicsInstance->PhysicalDeviceCount())
							return nullptr;
						else return context->m_graphicsInstance->GetPhysicalDevice(index);
					}

					inline Reference<GraphicsDevice> CreateDevice()const {
						Graphics::PhysicalDevice* device = PhysicalDevice();
						if (device == nullptr)
							return nullptr;
						else return device->CreateLogicalDevice();
					}

					inline iterator& operator++() {
						while (true) {
							index++;
							Graphics::PhysicalDevice* device = PhysicalDevice();
							if (device == nullptr) {
								index = ~size_t(0u);
								break;
							}
							else if (device->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
								break;
						}
						return (*this);
					}
				};

				inline iterator begin()const { iterator it = { ~size_t(0u), this }; ++it; return it; }
				inline iterator end()const { return { ~size_t(0u), this }; }

				struct WindowContext {
					Reference<GraphicsDevice> device;
					Reference<RenderSurface> surface;
					Reference<OS::Window> window;

					inline WindowContext(const iterator& it, const std::string_view& name) {
						if (it.context == nullptr || it == it.context->end())
							return;
						window = OS::Window::Create(it.context->Log(), std::string(name));
						assert(window != nullptr);
						surface = it.context->m_graphicsInstance->CreateRenderSurface(window);
						assert(surface != nullptr);
						if (!surface->DeviceCompatible(it.PhysicalDevice())) {
							surface = nullptr;
							window = nullptr;
						}
						else {
							device = it.CreateDevice();
							assert(device != nullptr);
						}
					}

					inline operator bool()const {
						return
							device != nullptr &&
							surface != nullptr &&
							window != nullptr;
					}
				};
			};
		}

		TEST(RayTracingAPITest, AccelerationStructureBuild) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			for (auto it = context.begin(); it != context.end(); ++it) {
				const Reference<GraphicsDevice> device = it.CreateDevice();
				ASSERT_NE(device, nullptr);

				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);

				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_READ_WRITE);
				ASSERT_NE(instanceDesc, nullptr);
				{
					AccelerationStructureInstanceDesc& desc = *instanceDesc.Map();
					desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
					desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
					desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
					desc.instanceCustomIndex = 0u;
					desc.visibilityMask = ~uint8_t(0u);
					desc.shaderBindingTableRecordOffset = 0u;
					desc.instanceFlags = 0u;
					desc.blasDeviceAddress = blas->DeviceAddress();
					instanceDesc->Unmap(true);
				}

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				const Reference<TopLevelAccelerationStructure> tlas = device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);
				const Reference<PrimaryCommandBuffer> commands = commandPool->CreatePrimaryCommandBuffer();
				ASSERT_NE(commandPool, nullptr);

				commands->BeginRecording();
				blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
				tlas->Build(commands, instanceDesc);
				commands->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commands);
				commands->Wait();
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!context.RTDeviceFound())
				context.Log()->Warning("No RT-Capable GPU was found!");
		}



		namespace {
			template<typename DataType>
			using RayTracingAPITest_DataCreateFn = Function<Reference<DataType>, const RenderEngineInfo*>;
			template<typename DataType>
			using RayTracingAPITest_RenderFunction = Callback<DataType*, const InFlightBufferInfo&>;

			template<typename DataType>
			inline static Reference<RenderEngine> RayTracingAPITest_CreateRenderEngine(
				GraphicsDevice* device, RenderSurface* surface,
				const RayTracingAPITest_DataCreateFn<DataType>& createData,
				const RayTracingAPITest_RenderFunction<DataType>& render) {
				struct Renderer : public virtual ImageRenderer {
					const RayTracingAPITest_DataCreateFn<DataType> dataCreate;
					const RayTracingAPITest_RenderFunction<DataType> renderFn;
					inline Renderer(
						const RayTracingAPITest_DataCreateFn<DataType>& init, 
						const RayTracingAPITest_RenderFunction<DataType>& render)
						: dataCreate(init), renderFn(render) {}
					virtual Reference<Object> CreateEngineData(RenderEngineInfo* engineInfo) final override {
						return dataCreate(engineInfo);

					}
					virtual void Render(Object* engineData, InFlightBufferInfo bufferInfo) final override {
						renderFn(dynamic_cast<DataType*>(engineData), bufferInfo);
					}
				};
				const Reference<RenderEngine> engine = device->CreateRenderEngine(surface);
				assert(engine != nullptr);
				const Reference<Renderer> renderer = Object::Instantiate<Renderer>(createData, render);
				engine->AddRenderer(renderer);
				return engine;
			}

			inline static void RayTracingAPITest_RenderLoop(
				RenderEngine* engine, OS::Window* window,
				const std::string_view& windowName, float closeTime = 5.0f) {

				Stopwatch frameTimer;
				float frameTime = 1.0f;
				float smoothFrameTime = 1.0f;
				auto update = [&](OS::Window*) { 
					engine->Update();
					frameTime = frameTimer.Reset();
					smoothFrameTime = Math::Lerp(smoothFrameTime, frameTime, Math::Min(Math::Max(0.01f, frameTime * 60.0f), 1.0f));
				};
				Callback<OS::Window*> updateFn = Callback<OS::Window*>::FromCall(&update);
				window->OnUpdate() += updateFn;

				Stopwatch elapsed;
				std::optional<Size2> initialResolution = window->FrameBufferSize();
				while (!window->Closed()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
					{
						std::stringstream stream;
						stream << windowName << std::fixed << std::setprecision(1) <<
							" [" << (frameTime * 1000.0f) << "ms; sFPS:" << (1.0f / smoothFrameTime) << "]";
						if (initialResolution.has_value())
							stream << " (Window will automatically close in " 
							<< Math::Max(closeTime - elapsed.Elapsed(), 0.0f) << " seconds unless resized)";
						const std::string name = stream.str();
						window->SetName(name);
					}
					if (!initialResolution.has_value())
						continue;
					else if (initialResolution.value() != window->FrameBufferSize())
						initialResolution = std::nullopt;
					else if (elapsed.Elapsed() >= closeTime)
						break;
				}

				window->OnUpdate() -= updateFn;
			}
		}



		TEST(RayTracingAPITest, InlineRayTracing_Fragment) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			auto getShader = [&](const char* stage) {
				const std::string name = std::string("InlineRayTracing.") + stage;
				return context.LoadShader(name);
			};
			const Reference<SPIRV_Binary> vertexShader = getShader("vert");
			ASSERT_NE(vertexShader, nullptr);
			const Reference<SPIRV_Binary> fragmentShader = getShader("frag");
			ASSERT_NE(fragmentShader, nullptr);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "InlineRayTracing_Fragment");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);
				
				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Create renderer:
				struct RendererData : public virtual Object {
					float aspectRatio = 1.0f;
					Reference<RenderPass> renderPass;
					Reference<GraphicsPipeline> pipeline;
					Reference<VertexInput> vertInput;
					Reference<BindingSet> bindings;
					std::vector<Reference<FrameBuffer>> frameBuffers;
				};

				auto dataCreate = [&](const RenderEngineInfo* engineInfo) -> Reference<RendererData> {
					const Reference<RendererData> data = Object::Instantiate<RendererData>();
					data->aspectRatio = float(engineInfo->ImageSize().x) / float(Math::Max(engineInfo->ImageSize().y, 1u));
					{
						const Texture::PixelFormat format = engineInfo->ImageFormat();
						data->renderPass = engineInfo->Device()->GetRenderPass(
							Texture::Multisampling::SAMPLE_COUNT_1,
							1u, &format, Texture::PixelFormat::FORMAT_COUNT,
							RenderPass::Flags::CLEAR_COLOR);
						assert(data->renderPass != nullptr);
					}
					{
						GraphicsPipeline::Descriptor desc = {};
						desc.vertexShader = vertexShader;
						desc.fragmentShader = fragmentShader;
						data->pipeline = data->renderPass->GetGraphicsPipeline(desc);
						assert(data->pipeline != nullptr);
					}
					{
						const ArrayBufferReference<uint16_t> indexBuffer = ctx.device->CreateArrayBuffer<uint16_t>(6u);
						assert(indexBuffer != nullptr);
						uint16_t* indexData = indexBuffer.Map();
						for (uint16_t i = 0u; i < indexBuffer->ObjectCount(); i++)
							indexData[i] = i;
						indexBuffer->Unmap(true);
						const Reference<const ResourceBinding<ArrayBuffer>> indexBufferBinding = 
							Object::Instantiate<ResourceBinding<ArrayBuffer>>(indexBuffer);
						data->vertInput = data->pipeline->CreateVertexInput(nullptr, indexBufferBinding);
						assert(data->vertInput != nullptr);
					}
					{
						const Reference<BindingPool> bindingPool = ctx.device->CreateBindingPool(engineInfo->ImageCount());
						assert(bindingPool != nullptr);
						BindingSet::Descriptor desc = {};
						desc.pipeline = data->pipeline;

						const Reference<const ResourceBinding<Buffer>> cbufferBinding = 
							Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
						auto findConstantBuffer = [&](const auto&) { return cbufferBinding; };
						desc.find.constantBuffer = &findConstantBuffer;

						const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
							Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
						auto findTlas = [&](const auto&) { return tlasBinding; };
						desc.find.accelerationStructure = &findTlas;

						data->bindings = bindingPool->AllocateBindingSet(desc);
						assert(data->bindings != nullptr);
					}
					for (size_t i = 0u; i < engineInfo->ImageCount(); i++) {
						const Reference<TextureView> view = engineInfo->Image(i)->CreateView(TextureView::ViewType::VIEW_2D);
						assert(view != nullptr);
						const Reference<FrameBuffer> frameBuffer = data->renderPass->CreateFrameBuffer(&view, nullptr, nullptr, nullptr);
						assert(frameBuffer != nullptr);
						data->frameBuffers.push_back(frameBuffer);
					}
					return data;
				};

				const Stopwatch elapsed;
				auto renderImage = [&](RendererData* data, const InFlightBufferInfo& commands) {
					{
						AccelerationStructureInstanceDesc& desc = *instanceDesc.Map();
						desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
						desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(elapsed.Elapsed()));
						desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
						desc.instanceCustomIndex = 0u;
						desc.visibilityMask = ~uint8_t(0u);
						desc.shaderBindingTableRecordOffset = 0u;
						desc.instanceFlags = 0u;
						desc.blasDeviceAddress = blas->DeviceAddress();
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);
					
					{
						Settings& settings = settingsBuffer.Map();
						const float angle = elapsed.Elapsed() * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) * data->aspectRatio;
						settingsBuffer->Unmap(true);
					}

					data->bindings->Update(commands);

					const constexpr Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);
					data->renderPass->BeginPass(commands, data->frameBuffers[commands], &CLEAR_VALUE);

					data->bindings->Bind(commands);
					data->vertInput->Bind(commands);
					data->pipeline->Draw(commands, 6u, 1u);

					data->renderPass->EndPass(commands);
				};

				const Reference<RenderEngine> engine = RayTracingAPITest_CreateRenderEngine(ctx.device, ctx.surface,
					RayTracingAPITest_DataCreateFn<RendererData>::FromCall(&dataCreate),
					RayTracingAPITest_RenderFunction<RendererData>::FromCall(&renderImage));
				ASSERT_NE(engine, nullptr);
				
				{
					std::stringstream stream;
					stream << ctx.window->Name() << " - " << it.PhysicalDevice()->Name();
					const std::string name = stream.str();
					RayTracingAPITest_RenderLoop(engine, ctx.window, name);
				}
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable display GPU was found!");
		}



		TEST(RayTracingAPITest, InlineRayTracing_Compute) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			const Reference<SPIRV_Binary> shader = context.LoadShader("InlineRayTracing.comp");
			ASSERT_NE(shader, nullptr);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "InlineRayTracing_Compute");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Create renderer:
				struct RendererData : public virtual Object {
					float aspectRatio = 1.0f;
					Reference<ComputePipeline> pipeline;
					Reference<BindingSet> bindings;
					Reference<TextureView> frameBufer;
					Reference<const RenderEngineInfo> engineInfo;
				};

				auto dataCreate = [&](const RenderEngineInfo* engineInfo) -> Reference<RendererData> {
					const Reference<RendererData> data = Object::Instantiate<RendererData>();
					{
						data->aspectRatio = float(engineInfo->ImageSize().x) / float(Math::Max(engineInfo->ImageSize().y, 1u));
						data->engineInfo = engineInfo;
					}
					{
						data->pipeline = ctx.device->GetComputePipeline(shader);
						assert(data->pipeline != nullptr);
					}
					{
						data->frameBufer = ctx.device->CreateTexture(
							Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R16G16B16A16_SFLOAT,
							Size3(engineInfo->ImageSize(), 1u), 1u, false, ImageTexture::AccessFlags::SHADER_WRITE)
							->CreateView(TextureView::ViewType::VIEW_2D);
						assert(data->frameBufer != nullptr);
					}
					{
						const Reference<BindingPool> bindingPool = ctx.device->CreateBindingPool(engineInfo->ImageCount());
						assert(bindingPool != nullptr);
						BindingSet::Descriptor desc = {};
						desc.pipeline = data->pipeline;

						const Reference<const ResourceBinding<Buffer>> cbufferBinding =
							Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
						auto findConstantBuffer = [&](const auto&) { return cbufferBinding; };
						desc.find.constantBuffer = &findConstantBuffer;

						const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
							Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
						auto findTlas = [&](const auto&) { return tlasBinding; };
						desc.find.accelerationStructure = &findTlas;

						const Reference<const ResourceBinding<TextureView>> imageBinding = 
							Object::Instantiate<ResourceBinding<TextureView>>(data->frameBufer);
						auto findImage = [&](const auto&) { return imageBinding; };
						desc.find.textureView = &findImage;

						data->bindings = bindingPool->AllocateBindingSet(desc);
						assert(data->bindings != nullptr);
					}
					return data;
				};

				const Stopwatch elapsed;
				auto renderImage = [&](RendererData* data, const InFlightBufferInfo& commands) {
					{
						AccelerationStructureInstanceDesc& desc = *instanceDesc.Map();
						desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
						desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(elapsed.Elapsed()));
						desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
						desc.instanceCustomIndex = 0u;
						desc.visibilityMask = ~uint8_t(0u);
						desc.shaderBindingTableRecordOffset = 0u;
						desc.instanceFlags = 0u;
						desc.blasDeviceAddress = blas->DeviceAddress();
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = elapsed.Elapsed() * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) * data->aspectRatio;
						settingsBuffer->Unmap(true);
					}

					data->bindings->Update(commands);
					
					{
						const constexpr uint32_t BLOCK_SIZE = 16u;
						data->bindings->Bind(commands);
						data->pipeline->Dispatch(commands, Size3(
							(Size2(data->frameBufer->TargetTexture()->Size()) + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u));
					}

					data->engineInfo->Image(commands)->Blit(commands, data->frameBufer->TargetTexture());
				};

				const Reference<RenderEngine> engine = RayTracingAPITest_CreateRenderEngine(ctx.device, ctx.surface,
					RayTracingAPITest_DataCreateFn<RendererData>::FromCall(&dataCreate),
					RayTracingAPITest_RenderFunction<RendererData>::FromCall(&renderImage));
				ASSERT_NE(engine, nullptr);

				{
					std::stringstream stream;
					stream << ctx.window->Name() << " - " << it.PhysicalDevice()->Name();
					const std::string name = stream.str();
					RayTracingAPITest_RenderLoop(engine, ctx.window, name);
				}
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable display GPU was found!");
		}


		namespace {
			inline static void RayTracingAPITest_RTPipelineRenderLoop(
				const RayTracingAPITest_Context::WindowContext& ctx,
				const RayTracingPipeline::Descriptor& pipelineDesc,
				const BindingSet::BindingSearchFunctions& bindingSearchFunctions,
				Callback<InFlightBufferInfo> updateFn = Callback<InFlightBufferInfo>(Unused<InFlightBufferInfo>)) {
				ASSERT_TRUE(ctx.operator bool());

				const Reference<Graphics::RayTracingPipeline> pipeline = ctx.device->CreateRayTracingPipeline(pipelineDesc);
				ASSERT_NE(pipeline, nullptr);

				struct RendererData : public virtual Object {
					Reference<TextureView> frameBuffer;
					Reference<BindingSet> bindings;
					Reference<const RenderEngineInfo> engineInfo;
				};

				auto dataCreate = [&](const RenderEngineInfo* engineInfo) -> Reference<RendererData> {
					const Reference<RendererData> data = Object::Instantiate<RendererData>();
					{
						data->engineInfo = engineInfo;
					}
					{
						data->frameBuffer = ctx.device->CreateTexture(
							Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R16G16B16A16_SFLOAT,
							Size3(engineInfo->ImageSize(), 1u), 1u, false, ImageTexture::AccessFlags::SHADER_WRITE)
							->CreateView(TextureView::ViewType::VIEW_2D);
						assert(data->frameBuffer != nullptr);
					}
					{
						const Reference<BindingPool> bindingPool = ctx.device->CreateBindingPool(engineInfo->ImageCount());
						assert(bindingPool != nullptr);
						BindingSet::Descriptor desc = {};
						desc.pipeline = pipeline;

						desc.find = bindingSearchFunctions;

						const Reference<const ResourceBinding<TextureView>> imageBinding =
							Object::Instantiate<ResourceBinding<TextureView>>(data->frameBuffer);
						auto findImage = [&](const auto& info) -> Reference<const ResourceBinding<TextureView>> {
							const auto ref = bindingSearchFunctions.textureView(info);
							return (ref != nullptr) ? ref : imageBinding;
						};
						desc.find.textureView = &findImage;

						data->bindings = bindingPool->AllocateBindingSet(desc);
						assert(data->bindings != nullptr);
					}
					return data;
				};

				auto renderImage = [&](RendererData* data, const InFlightBufferInfo& commands) {
					updateFn(commands);
					data->bindings->Update(commands);
					data->bindings->Bind(commands);
					pipeline->TraceRays(commands, data->frameBuffer->TargetTexture()->Size());
					data->engineInfo->Image(commands)->Blit(commands, data->frameBuffer->TargetTexture());
				};

				const Reference<RenderEngine> engine = RayTracingAPITest_CreateRenderEngine(ctx.device, ctx.surface,
					RayTracingAPITest_DataCreateFn<RendererData>::FromCall(&dataCreate),
					RayTracingAPITest_RenderFunction<RendererData>::FromCall(&renderImage));
				ASSERT_NE(engine, nullptr);

				{
					std::stringstream stream;
					stream << ctx.window->Name() << " - " << ctx.device->PhysicalDevice()->Name();
					const std::string name = stream.str();
					RayTracingAPITest_RenderLoop(engine, ctx.window, name);
				}
			}
		}


		TEST(RayTracingAPITest, RTPipeline_SimpleRayGen) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("SimpleRayGen.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_SimpleRayGen");
				if (!ctx)
					continue;
				deviceFound = true;
				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, {});
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable display GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_SimpleMiss) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("SimpleMiss.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("SimpleMiss.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_SimpleMiss");
				if (!ctx)
					continue;
				deviceFound = true;

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlas =
					Object::Instantiate<const ResourceBinding<TopLevelAccelerationStructure>>(ctx.device->CreateTopLevelAccelerationStructure({}));
				{
					ASSERT_NE(tlas->BoundObject(), nullptr);
					const ArrayBufferReference<AccelerationStructureInstanceDesc> instances =
						ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(0u);
					ASSERT_NE(instances, nullptr);
					EXPECT_EQ(instances->ObjectCount(), 0u);
					const Reference<PrimaryCommandBuffer> commands =
						ctx.device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
					ASSERT_NE(commands, nullptr);
					commands->BeginRecording();
					tlas->BoundObject()->Build(commands, instances);
					commands->EndRecording();
					ctx.device->GraphicsQueue()->ExecuteCommandBuffer(commands);
					commands->Wait();
				}

				BindingSet::BindingSearchFunctions searchFns = {};
				auto findTlas = [&](const auto&) { return tlas; };
				searchFns.accelerationStructure = &findTlas;

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns);
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_MultiMiss) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("MultiMiss.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("SimpleMiss.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
				pipelineDesc.missShaders.push_back(context.LoadShader("MultiMiss.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[1u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[1u]->ShaderStages(), PipelineStage::RAY_MISS);
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_MultiMiss");
				if (!ctx)
					continue;
				deviceFound = true;

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlas =
					Object::Instantiate<const ResourceBinding<TopLevelAccelerationStructure>>(ctx.device->CreateTopLevelAccelerationStructure({}));
				{
					ASSERT_NE(tlas->BoundObject(), nullptr);
					const ArrayBufferReference<AccelerationStructureInstanceDesc> instances =
						ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(0u);
					ASSERT_NE(instances, nullptr);
					EXPECT_EQ(instances->ObjectCount(), 0u);
					const Reference<PrimaryCommandBuffer> commands =
						ctx.device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
					ASSERT_NE(commands, nullptr);
					commands->BeginRecording();
					tlas->BoundObject()->Build(commands, instances);
					commands->EndRecording();
					ctx.device->GraphicsQueue()->ExecuteCommandBuffer(commands);
					commands->Wait();
				}

				BindingSet::BindingSearchFunctions searchFns = {};
				auto findTlas = [&](const auto&) { return tlas; };
				searchFns.accelerationStructure = &findTlas;

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns);
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_SimpleClosestHit) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("SingleCast.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("SingleCast.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("SimpleClosestHit.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
				}
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_SimpleClosestHit");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
				ASSERT_NE(sphere, nullptr);
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Binding table:
				BindingSet::BindingSearchFunctions searchFns = {};
				
				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding = 
					Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
				auto findTlas = [&](const auto&) { return tlasBinding; };
				searchFns.accelerationStructure = &findTlas;

				const Reference<const ResourceBinding<Buffer>> settingsBinding = 
					Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
				auto findSettings = [&](const auto&) { return settingsBinding; };
				searchFns.constantBuffer = &findSettings;

				// Update function:
				const Stopwatch elapsed;
				auto update = [&](InFlightBufferInfo commands) {
					{
						AccelerationStructureInstanceDesc& desc = *instanceDesc.Map();
						desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
						desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(elapsed.Elapsed()));
						desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
						desc.instanceCustomIndex = 0u;
						desc.visibilityMask = ~uint8_t(0u);
						desc.shaderBindingTableRecordOffset = 0u;
						desc.instanceFlags = 0u;
						desc.blasDeviceAddress = blas->DeviceAddress();
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = elapsed.Elapsed() * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) *
							float(ctx.window->FrameBufferSize().x) / float(Math::Max(ctx.window->FrameBufferSize().y, 1u));
						settingsBuffer->Unmap(true);
					}
				};

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns, Callback<InFlightBufferInfo>::FromCall(&update));
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_MultiClosestHit) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("SingleCast.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("SingleCast.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("SimpleClosestHit.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
				}
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("DiffuseClosestHit.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
				}
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_MultiClosestHit");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
				ASSERT_NE(sphere, nullptr);
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(2u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 2u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Binding table:
				BindingSet::BindingSearchFunctions searchFns = {};

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
					Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
				auto findTlas = [&](const auto&) { return tlasBinding; };
				searchFns.accelerationStructure = &findTlas;

				const Reference<const ResourceBinding<Buffer>> settingsBinding =
					Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
				auto findSettings = [&](const auto&) { return settingsBinding; };
				searchFns.constantBuffer = &findSettings;

				const Reference<const ResourceBinding<ArrayBuffer>> vertexBufferBinding =
					Object::Instantiate<ResourceBinding<ArrayBuffer>>(vertexBuffer);
				const Reference<const ResourceBinding<ArrayBuffer>> indexBufferBinding =
					Object::Instantiate<ResourceBinding<ArrayBuffer>>(indexBuffer);
				auto findStructuredBuffer = [&](const BindingSet::BindingDescriptor& desc) {
					return
						(desc.name == "vertices") ? vertexBufferBinding :
						(desc.name == "indices") ? indexBufferBinding : nullptr;
				};
				searchFns.structuredBuffer = &findStructuredBuffer;

				// Update function:
				const Stopwatch elapsed;
				auto update = [&](InFlightBufferInfo commands) {
					const float time = elapsed.Elapsed();

					{
						const float phase = time * 0.25f;
						AccelerationStructureInstanceDesc* instances = instanceDesc.Map();
						{
							AccelerationStructureInstanceDesc& desc = instances[0u];
							desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, std::cos(phase));
							desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(time));
							desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, std::sin(phase));
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 0u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = blas->DeviceAddress();
						}
						{
							AccelerationStructureInstanceDesc& desc = instances[1u];
							desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, std::cos(phase + Math::Pi()));
							desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::cos(time));
							desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, std::sin(phase + Math::Pi()));
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 1u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = blas->DeviceAddress();
						}
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = time * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) *
							float(ctx.window->FrameBufferSize().x) / float(Math::Max(ctx.window->FrameBufferSize().y, 1u));
						settingsBuffer->Unmap(true);
					}
				};

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns, Callback<InFlightBufferInfo>::FromCall(&update));
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_AnyHit) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("SingleCast.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("SingleCast.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("SimpleClosestHit.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
				}
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("DiffuseClosestHit.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
					group.anyHit = context.LoadShader("SimpleAnyHit.rahit");
					ASSERT_NE(group.anyHit, nullptr);
					EXPECT_EQ(group.anyHit->ShaderStages(), PipelineStage::RAY_ANY_HIT);
				}
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_AnyHit");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
				ASSERT_NE(sphere, nullptr);
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(2u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 2u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Binding table:
				BindingSet::BindingSearchFunctions searchFns = {};

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
					Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
				auto findTlas = [&](const auto&) { return tlasBinding; };
				searchFns.accelerationStructure = &findTlas;

				const Reference<const ResourceBinding<Buffer>> settingsBinding =
					Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
				auto findSettings = [&](const auto&) { return settingsBinding; };
				searchFns.constantBuffer = &findSettings;

				const Reference<const ResourceBinding<ArrayBuffer>> vertexBufferBinding =
					Object::Instantiate<ResourceBinding<ArrayBuffer>>(vertexBuffer);
				const Reference<const ResourceBinding<ArrayBuffer>> indexBufferBinding =
					Object::Instantiate<ResourceBinding<ArrayBuffer>>(indexBuffer);
				auto findStructuredBuffer = [&](const BindingSet::BindingDescriptor& desc) {
					return
						(desc.name == "vertices") ? vertexBufferBinding :
						(desc.name == "indices") ? indexBufferBinding : nullptr;
					};
				searchFns.structuredBuffer = &findStructuredBuffer;

				// Update function:
				const Stopwatch elapsed;
				auto update = [&](InFlightBufferInfo commands) {
					const float time = elapsed.Elapsed();

					{
						const float phase = time * 0.25f;
						AccelerationStructureInstanceDesc* instances = instanceDesc.Map();
						{
							AccelerationStructureInstanceDesc& desc = instances[0u];
							desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, std::cos(phase));
							desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(time));
							desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, std::sin(phase));
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 0u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = blas->DeviceAddress();
						}
						{
							AccelerationStructureInstanceDesc& desc = instances[1u];
							desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, std::cos(phase + Math::Pi()));
							desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::cos(time));
							desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, std::sin(phase + Math::Pi()));
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 1u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = blas->DeviceAddress();
						}
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = time * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) *
							float(ctx.window->FrameBufferSize().x) / float(Math::Max(ctx.window->FrameBufferSize().y, 1u));
						settingsBuffer->Unmap(true);
					}
					};

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns, Callback<InFlightBufferInfo>::FromCall(&update));
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_Callables) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("Callables.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.callableShaders.push_back(context.LoadShader("CallableA.rcall"));
				ASSERT_NE(pipelineDesc.callableShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.callableShaders[0u]->ShaderStages(), PipelineStage::CALLABLE);
				pipelineDesc.callableShaders.push_back(context.LoadShader("CallableB.rcall"));
				ASSERT_NE(pipelineDesc.callableShaders[1u], nullptr);
				EXPECT_EQ(pipelineDesc.callableShaders[1u]->ShaderStages(), PipelineStage::CALLABLE);
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_Callables");
				if (!ctx)
					continue;
				deviceFound = true;
				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, {});
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_InlineRT) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("InlineRayTracing.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_InlineRT");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS:
				const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
				ASSERT_NE(sphere, nullptr);
				const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, sphere, GraphicsPipeline::IndexType::TRIANGLE);
				ASSERT_NE(graphicsMesh, nullptr);

				ArrayBufferReference<MeshVertex> vertexBuffer;
				ArrayBufferReference<uint32_t> indexBuffer;
				graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
				ASSERT_NE(vertexBuffer, nullptr);
				ASSERT_NE(indexBuffer, nullptr);

				BottomLevelAccelerationStructure::Properties blasProps;
				blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
				blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
				const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
				ASSERT_NE(blas, nullptr);
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Binding table:
				BindingSet::BindingSearchFunctions searchFns = {};

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
					Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
				auto findTlas = [&](const auto&) { return tlasBinding; };
				searchFns.accelerationStructure = &findTlas;

				const Reference<const ResourceBinding<Buffer>> settingsBinding =
					Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
				auto findSettings = [&](const auto&) { return settingsBinding; };
				searchFns.constantBuffer = &findSettings;

				// Update function:
				const Stopwatch elapsed;
				auto update = [&](InFlightBufferInfo commands) {
					{
						AccelerationStructureInstanceDesc& desc = *instanceDesc.Map();
						desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
						desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(elapsed.Elapsed()));
						desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
						desc.instanceCustomIndex = 0u;
						desc.visibilityMask = ~uint8_t(0u);
						desc.shaderBindingTableRecordOffset = 0u;
						desc.instanceFlags = 0u;
						desc.blasDeviceAddress = blas->DeviceAddress();
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						blas->Build(commands, vertexBuffer, sizeof(MeshVertex), offsetof(MeshVertex, position), indexBuffer);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = elapsed.Elapsed() * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) *
							float(ctx.window->FrameBufferSize().x) / float(Math::Max(ctx.window->FrameBufferSize().y, 1u));
						settingsBuffer->Unmap(true);
					}
				};

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns, Callback<InFlightBufferInfo>::FromCall(&update));
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_Reflections) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			RayTracingPipeline::Descriptor pipelineDesc = {};
			{
				pipelineDesc.raygenShader = context.LoadShader("Reflections.rgen");
				ASSERT_NE(pipelineDesc.raygenShader, nullptr);
				EXPECT_EQ(pipelineDesc.raygenShader->ShaderStages(), PipelineStage::RAY_GENERATION);
				pipelineDesc.missShaders.push_back(context.LoadShader("Reflections.rmiss"));
				ASSERT_NE(pipelineDesc.missShaders[0u], nullptr);
				EXPECT_EQ(pipelineDesc.missShaders[0u]->ShaderStages(), PipelineStage::RAY_MISS);
				{
					RayTracingPipeline::ShaderGroup& group = pipelineDesc.bindingTable.emplace_back();
					group.closestHit = context.LoadShader("Reflections.rchit");
					ASSERT_NE(group.closestHit, nullptr);
					EXPECT_EQ(group.closestHit->ShaderStages(), PipelineStage::RAY_CLOSEST_HIT);
				}
			}

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_Reflections");
				if (!ctx)
					continue;
				deviceFound = true;

				// Prepare resources for BLAS-es:
				auto getMeshBuffers = [&](const Reference<TriMesh>& mesh) 
					-> std::tuple<
					ArrayBufferReference<MeshVertex>, 
					ArrayBufferReference<uint32_t>,
					Reference<BottomLevelAccelerationStructure>> {
					assert(mesh != nullptr);
					const Reference<GraphicsMesh> graphicsMesh = GraphicsMesh::Cached(ctx.device, mesh, GraphicsPipeline::IndexType::TRIANGLE);
					assert(graphicsMesh != nullptr);
					ArrayBufferReference<MeshVertex> vertexBuffer;
					ArrayBufferReference<uint32_t> indexBuffer;
					graphicsMesh->GetBuffers(vertexBuffer, indexBuffer);
					assert(vertexBuffer != nullptr);
					assert(indexBuffer != nullptr);
					BottomLevelAccelerationStructure::Properties blasProps;
					blasProps.maxVertexCount = static_cast<uint32_t>(vertexBuffer->ObjectCount());
					blasProps.maxTriangleCount = static_cast<uint32_t>(indexBuffer->ObjectCount() / 3u);
					const Reference<BottomLevelAccelerationStructure> blas = ctx.device->CreateBottomLevelAccelerationStructure(blasProps);
					assert(blas != nullptr);
					return std::tuple<
						ArrayBufferReference<MeshVertex>,
						ArrayBufferReference<uint32_t>,
						Reference<BottomLevelAccelerationStructure>>(
							vertexBuffer, indexBuffer, blas);
				};
				auto [sphereVerts, sphereIndices, sphereBlas] = getMeshBuffers(MeshConstants::Tri::Sphere());
				auto [planeVerts, planeIndices, planeBlas] = getMeshBuffers(MeshConstants::Tri::Plane());
				bool blasBuilt = false;

				// Build Buffer-Ref buffer:
				const Graphics::ArrayBufferReference<uint64_t> bufferRefs = ctx.device->CreateArrayBuffer<uint64_t>(4u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				{
					ASSERT_NE(bufferRefs, nullptr);
					uint64_t* ref = bufferRefs.Map();
					ref[0u] = sphereVerts->DeviceAddress();
					ref[1u] = sphereIndices->DeviceAddress();
					ref[2u] = planeVerts->DeviceAddress();
					ref[3u] = planeIndices->DeviceAddress();
					bufferRefs->Unmap(true);
				}

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					ctx.device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(2u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 2u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = ctx.device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = ctx.device->CreateConstantBuffer<Settings>();
				ASSERT_NE(settingsBuffer, nullptr);

				// Binding table:
				BindingSet::BindingSearchFunctions searchFns = {};

				const Reference<const ResourceBinding<TopLevelAccelerationStructure>> tlasBinding =
					Object::Instantiate<ResourceBinding<TopLevelAccelerationStructure>>(tlas);
				auto findTlas = [&](const auto&) { return tlasBinding; };
				searchFns.accelerationStructure = &findTlas;

				const Reference<const ResourceBinding<Buffer>> settingsBinding =
					Object::Instantiate<ResourceBinding<Buffer>>(settingsBuffer);
				auto findSettings = [&](const auto&) { return settingsBinding; };
				searchFns.constantBuffer = &findSettings;

				const Reference<const ResourceBinding<ArrayBuffer>> bufferRefBinding =
					Object::Instantiate<ResourceBinding<ArrayBuffer>>(bufferRefs);
				auto findStructuredBuffer = [&](const auto&) { return bufferRefBinding; };
				searchFns.structuredBuffer = &findStructuredBuffer;

				// Update function:
				const Stopwatch elapsed;
				auto update = [&](InFlightBufferInfo commands) {
					const float time = elapsed.Elapsed();

					{
						const float phase = time * 0.25f;
						AccelerationStructureInstanceDesc* instances = instanceDesc.Map();
						{
							AccelerationStructureInstanceDesc& desc = instances[0u];
							desc.transform[0u] = Vector4(1.0f, 0.0f, 0.0f, 0.0);
							desc.transform[1u] = Vector4(0.0f, 1.0f, 0.0f, std::sin(time) + 0.5f);
							desc.transform[2u] = Vector4(0.0f, 0.0f, 1.0f, 0.0);
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 0u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = sphereBlas->DeviceAddress();
						}
						{
							AccelerationStructureInstanceDesc& desc = instances[1u];
							desc.transform[0u] = Vector4(8.0f, 0.0f, 0.0f, 0.0);
							desc.transform[1u] = Vector4(0.0f, 8.0f, 0.0f, -1.5);
							desc.transform[2u] = Vector4(0.0f, 0.0f, 8.0f, 0.0);
							desc.instanceCustomIndex = 0u;
							desc.visibilityMask = ~uint8_t(0u);
							desc.shaderBindingTableRecordOffset = 0u;
							desc.instanceFlags = 0u;
							desc.blasDeviceAddress = planeBlas->DeviceAddress();
						}
						instanceDesc->Unmap(true);
					}

					if (!blasBuilt) {
						sphereBlas->Build(commands, sphereVerts, sizeof(MeshVertex), offsetof(MeshVertex, position), sphereIndices);
						planeBlas->Build(commands, planeVerts, sizeof(MeshVertex), offsetof(MeshVertex, position), planeIndices);
						tlas->Build(commands, instanceDesc);
						blasBuilt = true;
					}
					else tlas->Build(commands, instanceDesc, tlas);

					{
						Settings& settings = settingsBuffer.Map();
						const float angle = time * 0.5f;
						settings.right = Math::Right();
						settings.position = (Math::Back() * std::cos(angle) + Math::Right() * std::sin(angle)) * 5.0f;
						settings.forward = Math::Normalize(-settings.position);
						settings.up = Math::Up();
						settings.right = Math::Normalize(Math::Cross(settings.up, settings.forward)) *
							float(ctx.window->FrameBufferSize().x) / float(Math::Max(ctx.window->FrameBufferSize().y, 1u));
						settingsBuffer->Unmap(true);
					}
				};

				RayTracingAPITest_RTPipelineRenderLoop(ctx, pipelineDesc, searchFns, Callback<InFlightBufferInfo>::FromCall(&update));
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}

		TEST(RayTracingAPITest, RTPipeline_Shadows) {
			RayTracingAPITest_Context context = RayTracingAPITest_Context::Create();
			ASSERT_TRUE(context);

			bool deviceFound = false;
			for (auto it = context.begin(); it != context.end(); ++it) {
				// Filter device and create window:
				RayTracingAPITest_Context::WindowContext ctx(it, "RTPipeline_Shadows");
				if (!ctx)
					continue;
				deviceFound = true;

				// __TODO__: Implement this crap!
				EXPECT_TRUE("Implemented" == nullptr);
			}

			EXPECT_FALSE(context.AnythingFailed());
			if (!deviceFound)
				context.Log()->Warning("No RT-Capable GPU was found!");
		}
	}
}
