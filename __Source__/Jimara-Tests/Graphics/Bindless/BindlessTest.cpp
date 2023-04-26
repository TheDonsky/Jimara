#include "../../GtestHeaders.h"
#include "OS/Logging/StreamLogger.h"
#include "Graphics/GraphicsDevice.h"
#include "OS/Window/Window.h"
#include "Core/Stopwatch.h"
#include <thread>


namespace Jimara {
	namespace Graphics {
		namespace {
			class BindlessRenderer : public virtual ImageRenderer {
			public:
				struct ObjectBinding {
					Reference<Graphics::BindingSet> set;
					Reference<Graphics::Experimental::VertexInput> vertexInput;
					size_t indexCount = 0u;
				};

				class ObjectDescriptor : public virtual Object {
				public:
					virtual ObjectBinding CreateBinding(
						Graphics::Experimental::GraphicsPipeline* pipeline,
						Graphics::BindingPool* bindingPool) = 0;
				};

			private:
				const Reference<GraphicsDevice> m_device;
				const Reference<BindlessSet<TextureSampler>> m_textureSamplers;
				const Reference<BindlessSet<ArrayBuffer>> m_arrayBuffers;
				std::mutex m_objectSetLock;
				std::vector<Reference<ObjectDescriptor>> m_objects;
				Stopwatch m_stopwatch;
				std::atomic<float> m_frameTime = 1.0f;

				struct EngineData : public virtual Object {
					const Reference<BindlessSet<TextureSampler>::Instance> textureSamplers;
					const Reference<BindlessSet<ArrayBuffer>::Instance> arrayBuffers;
					const Reference<RenderPass> renderPass;
					std::vector<Reference<FrameBuffer>> frameBuffers;
					Reference<Graphics::Experimental::GraphicsPipeline> pipeline;
					Reference<Graphics::BindingPool> bindingPool;
					Reference<Graphics::BindingSet> bindlessTextureBinding;
					Reference<Graphics::BindingSet> bindlessBufferBinding;
					std::vector<ObjectBinding> objectBindings;

					inline EngineData(BindlessRenderer* renderer, RenderEngineInfo* engineInfo)
						: textureSamplers(renderer->m_textureSamplers->CreateInstance(engineInfo->ImageCount()))
						, arrayBuffers(renderer->m_arrayBuffers->CreateInstance(engineInfo->ImageCount()))
						, renderPass([&]() {
						const Texture::PixelFormat format = engineInfo->ImageFormat();
						return renderer->m_device->GetRenderPass(
							Texture::Multisampling::SAMPLE_COUNT_1, 1, &format, Texture::PixelFormat::OTHER, RenderPass::Flags::CLEAR_COLOR);
							}()) {
						if (textureSamplers == nullptr) 
							renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create texture sampler set instance!");
						if (arrayBuffers == nullptr)
							renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create array buffer set instance!");
						if (renderPass == nullptr)
							renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create render pass!");
						else for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							Reference<TextureView> view = engineInfo->Image(i)->CreateView(TextureView::ViewType::VIEW_2D);
							if (view == nullptr)
								renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create texture view for image ", i, "!");
							else {
								Reference<FrameBuffer> frameBuffer = renderPass->CreateFrameBuffer(&view, nullptr, nullptr, nullptr);
								if (frameBuffer == nullptr)
									renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create frame buffer for image ", i, "!");
								else frameBuffers.push_back(frameBuffer);
							}
						}
						
						// Get pipeline:
						{
							auto getShader = [&](const char* stage) {
								static const std::string path = "Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/Bindless/BindlessRenderer.";
								Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPVCached(path + stage + ".spv", renderer->m_device->Log());
								if (binary == nullptr)
									renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to load ", stage, " shader!");
								return binary;
							};
							Graphics::Experimental::GraphicsPipeline::Descriptor desc = {};
							desc.vertexShader = getShader("vert");
							desc.fragmentShader = getShader("frag");
							pipeline = renderPass->GetGraphicsPipeline(desc);
							if (pipeline == nullptr)
								renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to get/create pipeline!");
						}

						// Create binding pool:
						{
							bindingPool = renderer->m_device->CreateBindingPool(engineInfo->ImageCount());
							if (bindingPool == nullptr)
								renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create binding pool!");
						}

						// Create bindlessTextureBinding set
						{
							Graphics::BindingSet::Descriptor desc = {};
							desc.pipeline = pipeline;
							desc.bindingSetId = 0u;
							const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> bindings =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(textureSamplers);
							auto findBinding = [&](const auto&) { return bindings; };
							desc.find.bindlessTextureSamplers = &findBinding;
							bindlessTextureBinding = bindingPool->AllocateBindingSet(desc);
							if (bindlessTextureBinding == nullptr)
								renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create binding set for texture samplers!");
						}

						// Create bindlessBufferBinding set:
						{
							Graphics::BindingSet::Descriptor desc = {};
							desc.pipeline = pipeline;
							desc.bindingSetId = 1u;
							const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> bindings =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(arrayBuffers);
							auto findBinding = [&](const auto&) { return bindings; };
							desc.find.bindlessStructuredBuffers = &findBinding;
							bindlessBufferBinding = bindingPool->AllocateBindingSet(desc);
							if (bindlessBufferBinding == nullptr)
								renderer->m_device->Log()->Fatal("BindlessRenderer::EngineData - Failed to create binding set for styructured buffers!");
						}
					}
				};

			public:
				inline BindlessRenderer(GraphicsDevice* device, BindlessSet<TextureSampler>* textureSamplers, BindlessSet<ArrayBuffer>* arrayBuffers) 
					: m_device(device), m_textureSamplers(textureSamplers), m_arrayBuffers(arrayBuffers) {}

				inline void AddObject(const Reference<ObjectDescriptor>& object) {
					if (object == nullptr) return;
					std::unique_lock<std::mutex> lock(m_objectSetLock);
					m_objects.push_back(object);
				}

				virtual Reference<Object> CreateEngineData(RenderEngineInfo* engineInfo) override {
					return Object::Instantiate<EngineData>(this, engineInfo);
				}

				virtual void Render(Object* engineData, InFlightBufferInfo bufferInfo) override {
					EngineData* data = dynamic_cast<EngineData*>(engineData);
					{
						std::unique_lock<std::mutex> lock(m_objectSetLock);
						for (size_t i = data->objectBindings.size(); i < m_objects.size(); i++) {
							ObjectBinding binding = m_objects[i]->CreateBinding(data->pipeline, data->bindingPool);
							if (binding.set == nullptr || binding.vertexInput == nullptr)
								m_device->Log()->Fatal("BindlessRenderer::Render - Failed to create binding set!");
							data->objectBindings.push_back(binding);
						}
					}
					const Vector4 clearColor(1.0f, 0.0f, 0.0f, 1.0f);
					data->renderPass->BeginPass(bufferInfo.commandBuffer, data->frameBuffers[bufferInfo.inFlightBufferId], &clearColor);
					auto updateAndBind = [&](Graphics::BindingSet* set) {
						set->Update(bufferInfo);
						set->Bind(bufferInfo);
					};
					updateAndBind(data->bindlessTextureBinding);
					updateAndBind(data->bindlessBufferBinding);
					for (size_t i = 0u; i < data->objectBindings.size(); i++) {
						const auto& binding = data->objectBindings[i];
						updateAndBind(binding.set);
						binding.vertexInput->Bind(bufferInfo);
						data->pipeline->Draw(bufferInfo, binding.indexCount, 1u);
					}
					data->renderPass->EndPass(bufferInfo.commandBuffer);
					m_frameTime = m_stopwatch.Reset();
				}

				inline float FrameTime()const { return m_frameTime; }
			};

			class BindlessShape : public virtual BindlessRenderer::ObjectDescriptor {
			private:
				struct Vertex {
					Vector2 position;
					Vector2 uv;
				};

				struct VertexInfo {
					Vector2 offset = Vector2(0.0f);
					float scale = 1.0f;
					uint32_t vertexBufferIndex = 0u;
				};

				const Reference<GraphicsDevice> m_device;
				const Reference<BindlessSet<TextureSampler>::Binding> m_textureBinding;
				const Reference<BindlessSet<ArrayBuffer>::Binding> m_vertexBufferBinding;
				const BufferReference<uint32_t> m_textureIndexBuffer;
				const BufferReference<VertexInfo> m_vertexBufferInfo;
				const ArrayBufferReference<uint32_t> m_indexBuffer;

			public:
				inline static Reference<BindlessSet<TextureSampler>::Binding> CreateTexture(
					Size2 size, GraphicsDevice* device, BindlessSet<TextureSampler>* samplers) {
					const Reference<ImageTexture> texture = device->CreateTexture(
						Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::B8G8R8A8_SRGB, Size3(size, 1), 1, true);
					if (texture == nullptr) {
						device->Log()->Fatal("ColoredTriangle::CreateTexture - Failed to create a texture!");
						return nullptr;
					}
					const Reference<TextureView> view = texture->CreateView(TextureView::ViewType::VIEW_2D);
					if (view == nullptr) {
						device->Log()->Fatal("ColoredTriangle::CreateTexture - Failed to create a texture veiw!");
						return nullptr;
					}
					const Reference<TextureSampler> sampler = view->CreateSampler();
					if (sampler == nullptr) {
						device->Log()->Fatal("ColoredTriangle::CreateTexture - Failed to create a texture sampler!");
						return nullptr;
					}
					const Reference<BindlessSet<TextureSampler>::Binding> binding = samplers->GetBinding(sampler);
					if (binding == nullptr)
						device->Log()->Fatal("ColoredTriangle::CreateTexture - Failed to get bindless index!");
					return binding;
				}

				inline static Reference<BindlessSet<ArrayBuffer>::Binding> CreateVertices(size_t count, GraphicsDevice* device, BindlessSet<ArrayBuffer>* samplers) {
					const ArrayBufferReference<Vertex> buffer = device->CreateArrayBuffer<Vertex>(count);
					if (buffer == nullptr) {
						device->Log()->Fatal("ColoredTriangle::CreateVertices - Failed to create a buffer!");
						return nullptr;
					}
					const Reference<BindlessSet<ArrayBuffer>::Binding> binding = samplers->GetBinding(buffer);
					if (binding == nullptr)
						device->Log()->Fatal("ColoredTriangle::CreateVertices - Failed to get bindless index!");
					return binding;
				}

				template<typename ActionType>
				inline void MapTexture(const ActionType& action) {
					ImageTexture* texture = dynamic_cast<ImageTexture*>(m_textureBinding->BoundObject()->TargetView()->TargetTexture());
					texture->Unmap(action(reinterpret_cast<uint32_t*>(texture->Map()), texture->Size()));
				}

				template<typename ActionType>
				inline void MapVertices(const ActionType& action) {
					const ArrayBufferReference<Vertex> buffer = m_vertexBufferBinding->BoundObject();
					buffer->Unmap(action(buffer.Map(), buffer->ObjectCount()));
				}

				template<typename ActionType>
				inline void MapIndices(const ActionType& action) {
					m_indexBuffer->Unmap(action(m_indexBuffer.Map(), m_indexBuffer->ObjectCount()));
				}

				inline void SetPositionAndScale(const Vector2& position, float size) {
					VertexInfo& info = m_vertexBufferInfo.Map();
					info.offset = position;
					info.scale = size;
					info.vertexBufferIndex = m_vertexBufferBinding->Index();
					m_vertexBufferInfo->Unmap(true);
				}

				inline BindlessShape(
					GraphicsDevice* device, 
					BindlessSet<TextureSampler>* samplers, BindlessSet<ArrayBuffer>* buffers, 
					size_t vertexCount, Size2 textureSize)
					: m_device(device)
					, m_textureBinding(CreateTexture(textureSize, device, samplers))
					, m_vertexBufferBinding(CreateVertices(Math::Max(vertexCount, (size_t)3u), device, buffers))
					, m_textureIndexBuffer(device->CreateConstantBuffer<uint32_t>())
					, m_vertexBufferInfo(device->CreateConstantBuffer<VertexInfo>())
					, m_indexBuffer(device->CreateArrayBuffer<uint32_t>((Math::Max(vertexCount, (size_t)3u) - 2u) * 3u)) {
					SetPositionAndScale(Vector2(0.0f), 1.0f);
					{
						m_textureIndexBuffer.Map() = m_textureBinding->Index();
						m_textureIndexBuffer->Unmap(true);
					}
					MapVertices([&](Vertex* verts, size_t vertexCount) {
						const float angleStep = Math::Radians(360.0f / static_cast<float>(vertexCount));
						for (size_t i = 0; i < vertexCount; i++) {
							const float angle = angleStep * static_cast<float>(i);
							Vertex& vertex = verts[i];
							vertex.position = Vector2(std::cos(angle), std::sin(angle));
							vertex.uv = (vertex.position * 0.5f) + 0.5f;
						}
						MapIndices([&](uint32_t* indices, size_t) {
							for (uint32_t i = 2; i < vertexCount; i++) {
								indices[0] = 0;
								indices[1] = i - 1;
								indices[2] = i;
								indices += 3;
							}
							return true;
							});
						return true;
						});
				}

				virtual BindlessRenderer::ObjectBinding CreateBinding(
					Graphics::Experimental::GraphicsPipeline* pipeline, Graphics::BindingPool* bindingPool) override {
					BindlessRenderer::ObjectBinding result = {};
					{
						Graphics::BindingSet::Descriptor desc = {};
						desc.pipeline = pipeline;
						desc.bindingSetId = 2u;
						const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> vertexInfoBuffer =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(m_vertexBufferInfo);
						const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> textureIndexBuffer =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(m_textureIndexBuffer);
						auto findConstantBuffer = [&](const Graphics::BindingSet::BindingDescriptor& info) {
							return
								(info.binding == 0u) ? textureIndexBuffer :
								(info.binding == 1u) ? vertexInfoBuffer : nullptr;
						};
						desc.find.constantBuffer = &findConstantBuffer;
						result.set = bindingPool->AllocateBindingSet(desc);
					}
					{
						const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBuffer =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(m_indexBuffer);
						result.vertexInput = pipeline->CreateVertexInput(nullptr, indexBuffer);
						result.indexCount = m_indexBuffer->ObjectCount();
					}
					return result;
				}
			};
		}

		TEST(BindlessTest, Rendering) {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			ASSERT_NE(logger, nullptr);

			const Reference<GraphicsInstance> graphicsInstance = [&]() {
				static const Application::AppInformation appInfo;
				return GraphicsInstance::Create(logger, &appInfo);
			}();
			ASSERT_NE(graphicsInstance, nullptr);

			const Reference<OS::Window> window = OS::Window::Create(logger, "BindlessTest");
			ASSERT_NE(window, nullptr);

			const Reference<RenderSurface> renderSurface = graphicsInstance->CreateRenderSurface(window);
			ASSERT_NE(renderSurface, nullptr);

			const Reference<GraphicsDevice> device = [&]() -> Reference<GraphicsDevice> {
				PhysicalDevice* physicalDevice = renderSurface->PrefferedDevice();
				if (physicalDevice == nullptr) return nullptr;
				else return physicalDevice->CreateLogicalDevice();
			}();
			ASSERT_NE(device, nullptr);

			const Reference<RenderEngine> renderEngine = device->CreateRenderEngine(renderSurface);
			ASSERT_NE(renderEngine, nullptr);

			const Reference<BindlessSet<TextureSampler>> textureSamplers = device->CreateTextureSamplerBindlessSet();
			ASSERT_NE(textureSamplers, nullptr);

			const Reference<BindlessSet<ArrayBuffer>> arrayBuffers = device->CreateArrayBufferBindlessSet();
			ASSERT_NE(arrayBuffers, nullptr);

			const Reference<BindlessRenderer> renderer = Object::Instantiate<BindlessRenderer>(device, textureSamplers, arrayBuffers);
			ASSERT_NE(renderer, nullptr);

			renderEngine->AddRenderer(renderer);
			std::vector<Reference<BindlessShape>> shapes = {
				Object::Instantiate<BindlessShape>(device, textureSamplers, arrayBuffers, 3, Size2(32))
			};
			for (size_t i = 0; i < shapes.size(); i++)
				renderer->AddObject(shapes[i]);

			{
				const Stopwatch stopwatch;
				auto onWindowUpdate = [&](OS::Window*) { 
					renderEngine->Update(); 
					const float elapsed = stopwatch.Elapsed();
					for (size_t i = 0; i < shapes.size(); i++) {
						BindlessShape* shape = shapes[i];
						shape->SetPositionAndScale(
							Vector2(std::cos(elapsed), std::sin(elapsed)) * 0.225f,
							0.125f * (std::sin(elapsed) + 1.0f) + 0.125f);
						shape->MapTexture([&](uint32_t* color, Size2 size) {
							for (uint32_t y = 0; y < size.y; y++)
								for (uint32_t x = 0; x < size.x; x++) {
									*color = (((uint32_t)(elapsed * 64)) + x) ^ (((uint32_t)(elapsed * 32)) + size.y - y);
									color++;
								}
							return true;
							});
					}
				};
				const Callback<OS::Window*> updateRenderEngine = Callback<OS::Window*>::FromCall(&onWindowUpdate);
				window->OnUpdate() += updateRenderEngine;
				const constexpr float timeout = 5.0f;
				std::optional<Size2> size = window->FrameBufferSize();
				while (true) {
					if (window->Closed()) break;
					{
						const float elapsed = stopwatch.Elapsed();
						std::stringstream nameStream;
						nameStream << "BindlessTest - ";
						if (size.has_value()) {
							Size2 curSize = window->FrameBufferSize();
							if (curSize != size.value()) size = std::optional<Size2>();
							else if (elapsed > timeout) break;
							nameStream << " Closing in " << (timeout - elapsed) << " seconds, unless resized ";
						}
						else nameStream << " Close window to exit test ";
						const float frameTime = renderer->FrameTime();
						nameStream << " [Frame time: " << (frameTime * 1000.0f) << "ms; FPS: " << (1.0f / frameTime) << "]";
						window->SetName(nameStream.str());
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(4));
				}
				renderEngine->RemoveRenderer(renderer);
				window->OnUpdate() -= updateRenderEngine;
			}
		}
	}
}
