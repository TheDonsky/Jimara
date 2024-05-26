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
		TEST(RayTracingAPITest, AccelerationStructureBuild) {
			const Reference<Jimara::Test::CountingLogger> log = Object::Instantiate<Jimara::Test::CountingLogger>();
			const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("RayTracingAPITest");
			const Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(log, appInfo);
			ASSERT_NE(graphicsInstance, nullptr);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			const size_t warningCount = log->NumWarning();
			const size_t failureCount = log->Numfailures();

			bool deviceFound = false;
			for (size_t deviceId = 0u; deviceId < graphicsInstance->PhysicalDeviceCount(); deviceId++) {
				PhysicalDevice* const physDevice = graphicsInstance->GetPhysicalDevice(deviceId);
				if (!physDevice->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					continue;
				deviceFound = true;

				const Reference<GraphicsDevice> device = physDevice->CreateLogicalDevice();
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

			EXPECT_EQ(warningCount, log->NumWarning());
			EXPECT_EQ(failureCount, log->Numfailures());

			if (!deviceFound)
				log->Warning("No RT-Capable GPU was found!");
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
			const Reference<Jimara::Test::CountingLogger> log = Object::Instantiate<Jimara::Test::CountingLogger>();
			const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("RayTracingAPITest");

			const Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(log, appInfo);
			ASSERT_NE(graphicsInstance, nullptr);

			auto getShader = [&](const char* stage) {
				static const std::string path = "Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/RayTracing/InlineRayTracing.";
				Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPVCached(path + stage + ".spv", log);
				if (binary == nullptr)
					log->Fatal("BindlessRenderer::EngineData - Failed to load ", stage, " shader!");
				return binary;
			};
			const Reference<SPIRV_Binary> vertexShader = getShader("vert");
			ASSERT_NE(vertexShader, nullptr);
			const Reference<SPIRV_Binary> fragmentShader = getShader("frag");
			ASSERT_NE(fragmentShader, nullptr);

			const Reference<TriMesh> sphere = MeshConstants::Tri::Sphere();
			ASSERT_NE(sphere, nullptr);

			const size_t warningCount = log->NumWarning();
			const size_t failureCount = log->Numfailures();

			bool deviceFound = false;
			for (size_t deviceId = 0u; deviceId < graphicsInstance->PhysicalDeviceCount(); deviceId++) {
				// Filter device and create window:
				PhysicalDevice* const physDevice = graphicsInstance->GetPhysicalDevice(deviceId);
				if (!physDevice->HasFeatures(PhysicalDevice::DeviceFeatures::RAY_TRACING))
					continue;
				const Reference<OS::Window> window = OS::Window::Create(log, "InlineRayTracing_Fragment");
				ASSERT_NE(window, nullptr);
				const Reference<RenderSurface> surface = graphicsInstance->CreateRenderSurface(window);
				ASSERT_NE(surface, nullptr);
				if (!surface->DeviceCompatible(physDevice))
					continue;
				deviceFound = true;

				// Create device:
				const Reference<GraphicsDevice> device = physDevice->CreateLogicalDevice();
				ASSERT_NE(device, nullptr);

				// Prepare resources for BLAS:
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
				bool blasBuilt = false;

				// Prepare resources for TLAS:
				const ArrayBufferReference<AccelerationStructureInstanceDesc> instanceDesc =
					device->CreateArrayBuffer<AccelerationStructureInstanceDesc>(1u, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(instanceDesc, nullptr);

				TopLevelAccelerationStructure::Properties tlasProps;
				tlasProps.maxBottomLevelInstances = 1u;
				tlasProps.flags = AccelerationStructure::Flags::ALLOW_UPDATES | AccelerationStructure::Flags::PREFER_FAST_BUILD;
				const Reference<TopLevelAccelerationStructure> tlas = device->CreateTopLevelAccelerationStructure(tlasProps);
				ASSERT_NE(tlas, nullptr);

				// Create constant buffer:
				struct Settings {
					alignas(16) Vector3 right;
					alignas(16) Vector3 up;
					alignas(16) Vector3 forward;
					alignas(16) Vector3 position;
				};
				const BufferReference<Settings> settingsBuffer = device->CreateConstantBuffer<Settings>();
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
						const ArrayBufferReference<uint16_t> indexBuffer = device->CreateArrayBuffer<uint16_t>(6u);
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
						const Reference<BindingPool> bindingPool = device->CreateBindingPool(engineInfo->ImageCount());
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

				const Reference<RenderEngine> engine = RayTracingAPITest_CreateRenderEngine(device, surface,
					RayTracingAPITest_DataCreateFn<RendererData>::FromCall(&dataCreate),
					RayTracingAPITest_RenderFunction<RendererData>::FromCall(&renderImage));
				ASSERT_NE(engine, nullptr);
				
				{
					std::stringstream stream;
					stream << window->Name() << " - " << physDevice->Name();
					const std::string name = stream.str();
					RayTracingAPITest_RenderLoop(engine, window, name);
				}
			}

			EXPECT_EQ(warningCount, log->NumWarning());
			EXPECT_EQ(failureCount, log->Numfailures());

			if (!deviceFound)
				log->Warning("No RT-Capable GPU was found!");
		}
	}
}
