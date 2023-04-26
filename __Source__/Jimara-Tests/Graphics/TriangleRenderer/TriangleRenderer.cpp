#include "TriangleRenderer.h"
#include "Core/Stopwatch.h"
#include <math.h>


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Test {
			namespace {
				class TriangleRendererData : public Object {
				private:
					const Reference<TriangleRenderer> m_renderer;
					const Reference<RenderEngineInfo> m_engineInfo;
					Reference<RenderPass> m_renderPass;
					std::vector<Reference<FrameBuffer>> m_frameBuffers;
					Reference<Graphics::GraphicsPipeline> m_pipeline;
					Reference<Graphics::BindingSet> m_bindingSet;
					Reference<Graphics::VertexInput> m_vertexInput;


				public:
					inline TriangleRendererData(TriangleRenderer* renderer, RenderEngineInfo* engineInfo) 
						: m_renderer(renderer), m_engineInfo(engineInfo) {

						Texture::PixelFormat pixelFormat = engineInfo->ImageFormat();
						
						Reference<TextureView> colorAttachment = engineInfo->Device()->CreateMultisampledTexture(
							Texture::TextureType::TEXTURE_2D, pixelFormat, Size3(engineInfo->ImageSize(), 1), 1, Texture::Multisampling::MAX_AVAILABLE)
							->CreateView(TextureView::ViewType::VIEW_2D);

						m_renderPass = engineInfo->Device()->GetRenderPass(
							colorAttachment->TargetTexture()->SampleCount(), 1, &pixelFormat, Texture::PixelFormat::FORMAT_COUNT,
							Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH | Graphics::RenderPass::Flags::RESOLVE_COLOR);

						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							Reference<TextureView> resolveView = engineInfo->Image(i)->CreateView(TextureView::ViewType::VIEW_2D);
							m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, nullptr, &resolveView, nullptr));
						}

						{
							Graphics::GraphicsPipeline::Descriptor desc = {};
							desc.vertexShader = Graphics::SPIRV_Binary::FromSPVCached(
								"Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/TriangleRenderer/TriangleRenderer.vert.spv",
								engineInfo->Device()->Log());
							desc.fragmentShader = Graphics::SPIRV_Binary::FromSPVCached(
								"Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/TriangleRenderer/TriangleRenderer.frag.spv",
								engineInfo->Device()->Log());
							{
								Graphics::GraphicsPipeline::VertexInputInfo vertexBuffer = {};
								vertexBuffer.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
								vertexBuffer.bufferElementSize = sizeof(Vector2);
								Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo locationInfo = {};
								locationInfo.location = 0u;
								locationInfo.bufferElementOffset = 0u;
								vertexBuffer.locations.Push(locationInfo);
								desc.vertexInput.Push(vertexBuffer);
							}
							{
								Graphics::GraphicsPipeline::VertexInputInfo instanceBuffer = {};
								instanceBuffer.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
								instanceBuffer.bufferElementSize = sizeof(Vector2);
								Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo locationInfo = {};
								locationInfo.name = "vertOffset";
								locationInfo.bufferElementOffset = 0u;
								instanceBuffer.locations.Push(locationInfo);
								desc.vertexInput.Push(instanceBuffer);
							}
							m_pipeline = m_renderPass->GetGraphicsPipeline(desc);
						}

						{
							const Reference<Graphics::BindingPool> bindingPool = engineInfo->Device()->CreateBindingPool(engineInfo->ImageCount());
							Graphics::BindingSet::Descriptor desc = {};
							desc.pipeline = m_pipeline;
							desc.bindingSetId = 0u;

							const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> constantBuffer =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(renderer->ConstantBuffer());
							auto findConstantBuffer = [&](const auto&) { return constantBuffer; };
							desc.find.constantBuffer = &findConstantBuffer;

							const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> sampler =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(renderer->Sampler());
							auto findSampler = [&](const auto&) { return sampler; };
							desc.find.textureSampler = &findSampler;

							m_bindingSet = bindingPool->AllocateBindingSet(desc);
						}

						{
							const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> vertexBufferBinding =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(renderer->PositionBuffer());
							const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBufferBinding =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(renderer->InstanceOffsetBuffer());
							const Graphics::ResourceBinding<Graphics::ArrayBuffer>* vertexInputs[] = {
								vertexBufferBinding, indexBufferBinding
							};
							m_vertexInput = m_pipeline->CreateVertexInput(vertexInputs + 0u, nullptr);
						}
					}

					inline virtual ~TriangleRendererData() { }

					inline void Render(const InFlightBufferInfo bufferInfo)const {
						// Begin render pass
						const Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);
						m_renderPass->BeginPass(bufferInfo.commandBuffer, m_frameBuffers[bufferInfo.inFlightBufferId], &CLEAR_VALUE);

						// Draw geometry
						m_bindingSet->Update(bufferInfo);
						m_bindingSet->Bind(bufferInfo);
						m_vertexInput->Bind(bufferInfo);
						m_pipeline->Draw(bufferInfo,
							m_renderer->PositionBuffer()->ObjectCount(),
							m_renderer->InstanceOffsetBuffer()->ObjectCount());

						// End render pass
						m_renderPass->EndPass(bufferInfo.commandBuffer);
					}
				};
			}
			
			namespace {
				inline static void UpdateThread(
					BufferReference<float> scale, 
					ImageTexture* texture, 
					ArrayBufferReference<Vector2> offsetBuffer,
					volatile bool* alive) {
					
					Stopwatch stopwatch;
					while (*alive) {
						const float time = stopwatch.Elapsed();

						scale.Map() = (sin(time * 0.25f) + 1.25f) * 0.5f;
						scale->Unmap(true);

						Vector2* offsets = offsetBuffer.Map();
						offsets[0] = Vector2(cos(time), sin(time)) * 0.05f;
						offsets[1] = Vector2(1.0f, 0.15f) + Vector2(sin(time), cos(time)) * 0.1f;
						offsetBuffer->Unmap(true);

						const Size3 TEXTURE_SIZE = texture->Size();
						uint32_t* data = static_cast<uint32_t*>(texture->Map());
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
						texture->Unmap(true);

						std::this_thread::sleep_for(std::chrono::milliseconds(8));
					}
				}
			}

			TriangleRenderer::TriangleRenderer(GraphicsDevice* device)
				: m_device(device), m_shaderCache(ShaderCache::ForDevice(device)), m_rendererAlive(true) {

				Reference<ImageTexture> texture = m_device->CreateTexture(
					Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R8G8B8A8_UNORM, Size3(256, 256, 1), 1, true);

				if (texture == nullptr)
					m_device->Log()->Fatal("TriangleRenderer - Could not create the texture!");

				m_cbuffer = m_device->CreateConstantBuffer<float>();

				{
					m_positionBuffer = device->CreateArrayBuffer<Vector2>(6);
					Vector2* positions = m_positionBuffer.Map();
					positions[0] = Vector2(-0.5f, -0.25f);
					positions[1] = Vector2(-0.75f, -0.75f);
					positions[2] = Vector2(-0.25f, -0.75f);
					positions[3] = Vector2(-0.5f, 0.25f);
					positions[4] = Vector2(-0.25f, 0.75f);
					positions[5] = Vector2(-0.75f, 0.75f);
					m_positionBuffer->Unmap(true);
				}
				{
					m_instanceOffsetBuffer = device->CreateArrayBuffer<Vector2>(2);
					Vector2* positions = m_instanceOffsetBuffer.Map();
					positions[0] = Vector2(0.0f, 0.0f);
					positions[1] = Vector2(1.0f, 0.15f);
					m_instanceOffsetBuffer->Unmap(true);
				}
				
				const Size3 TEXTURE_SIZE = texture->Size();
				texture->Map();
				texture->Unmap(true);
				m_sampler = texture->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();

				m_imageUpdateThread = std::thread(UpdateThread, m_cbuffer, texture, m_instanceOffsetBuffer, &m_rendererAlive);
			}

			TriangleRenderer::~TriangleRenderer() {
				m_rendererAlive = false;
				m_imageUpdateThread.join();
			}

			Reference<Object> TriangleRenderer::CreateEngineData(RenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			ShaderCache* TriangleRenderer::GetShaderCache()const { return m_shaderCache; }

			void TriangleRenderer::Render(Object* engineData, InFlightBufferInfo bufferInfo) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);
				data->Render(bufferInfo);
			}

			Buffer* TriangleRenderer::ConstantBuffer()const { return m_cbuffer; }

			TextureSampler* TriangleRenderer::Sampler()const { return m_sampler; }

			ArrayBuffer* TriangleRenderer::PositionBuffer() { return m_positionBuffer; }

			ArrayBuffer* TriangleRenderer::InstanceOffsetBuffer() { return m_instanceOffsetBuffer; }
		}
	}
}
#pragma warning(default: 26812)
