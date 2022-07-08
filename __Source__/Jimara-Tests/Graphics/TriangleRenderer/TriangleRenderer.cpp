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
					Reference<GraphicsPipeline> m_renderPipeline;

					class Descriptor 
						: public virtual GraphicsPipeline::Descriptor
						, public virtual PipelineDescriptor::BindingSetDescriptor {
					private:
						TriangleRendererData* m_data;

					public:
						inline Descriptor(TriangleRendererData* data) : m_data(data) {}

						inline virtual size_t BindingSetCount()const override { return 1; }
						inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return (PipelineDescriptor::BindingSetDescriptor*)this; }

						virtual bool SetByEnvironment()const override { return false; }

						virtual size_t ConstantBufferCount()const override { return 1; }
						virtual BindingInfo ConstantBufferInfo(size_t index)const { return { StageMask(PipelineStage::VERTEX), 1 }; }
						virtual Reference<Buffer> ConstantBuffer(size_t index)const override { return m_data->m_renderer->ConstantBuffer(); }

						virtual size_t StructuredBufferCount()const override { return 0; }
						virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo(); }
						virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

						virtual size_t TextureSamplerCount()const override { return 1; }
						virtual BindingInfo TextureSamplerInfo(size_t index)const override { return { StageMask(PipelineStage::FRAGMENT), 0 }; }
						virtual Reference<TextureSampler> Sampler(size_t index)const override { return m_data->m_renderer->Sampler(); }


						inline virtual Reference<Shader> VertexShader()const override {
							return m_data->m_renderer->GetShaderCache()->GetShader(
								"Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/TriangleRenderer/TriangleRenderer.vert.spv", false);
						}

						inline virtual Reference<Shader> FragmentShader()const override {
							return m_data->m_renderer->GetShaderCache()->GetShader(
								"Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/TriangleRenderer/TriangleRenderer.frag.spv", true);
						}

						inline virtual size_t VertexBufferCount()const override { return 1; }
						inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override { return m_data->m_renderer->PositionBuffer(); }

						inline virtual size_t InstanceBufferCount()const override { return 1; }
						inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override { return m_data->m_renderer->InstanceOffsetBuffer(); }

						inline virtual ArrayBufferReference<uint32_t> IndexBuffer() override { return nullptr; }
						inline virtual GraphicsPipeline::IndexType GeometryType() override { return GraphicsPipeline::IndexType::TRIANGLE; }
						inline virtual size_t IndexCount() override { return m_data->m_renderer->PositionBuffer()->Buffer()->ObjectCount(); }
						inline virtual size_t InstanceCount() override { return m_data->m_renderer->InstanceOffsetBuffer()->Buffer()->ObjectCount(); }
					} m_pipelineDescriptor;


				public:
					inline TriangleRendererData(TriangleRenderer* renderer, RenderEngineInfo* engineInfo) 
						: m_renderer(renderer), m_engineInfo(engineInfo), m_pipelineDescriptor(this) {

						Texture::PixelFormat pixelFormat = engineInfo->ImageFormat();
						
						Reference<TextureView> colorAttachment = engineInfo->Device()->CreateMultisampledTexture(
							Texture::TextureType::TEXTURE_2D, pixelFormat, Size3(engineInfo->ImageSize(), 1), 1, Texture::Multisampling::MAX_AVAILABLE)
							->CreateView(TextureView::ViewType::VIEW_2D);

						m_renderPass = engineInfo->Device()->CreateRenderPass(
							colorAttachment->TargetTexture()->SampleCount(), 1, &pixelFormat, Texture::PixelFormat::FORMAT_COUNT,
							Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH | Graphics::RenderPass::Flags::RESOLVE_COLOR);

						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							Reference<TextureView> resolveView = engineInfo->Image(i)->CreateView(TextureView::ViewType::VIEW_2D);
							m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, nullptr, &resolveView, nullptr));
						}

						m_renderPipeline = m_renderPass->CreateGraphicsPipeline(&m_pipelineDescriptor, engineInfo->ImageCount());
					}

					inline virtual ~TriangleRendererData() { m_renderPipeline = nullptr; }

					inline void Render(const Pipeline::CommandBufferInfo bufferInfo)const { 
						// Begin render pass
						const Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);
						m_renderPass->BeginPass(bufferInfo.commandBuffer, m_frameBuffers[bufferInfo.inFlightBufferId], &CLEAR_VALUE);

						// Draw geometry
						m_renderPipeline->Execute(bufferInfo);

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
				: m_device(device), m_shaderCache(ShaderCache::ForDevice(device))
				, m_positionBuffer(device), m_instanceOffsetBuffer(device), m_rendererAlive(true) {

				Reference<ImageTexture> texture = m_device->CreateTexture(
					Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R8G8B8A8_UNORM, Size3(256, 256, 1), 1, true);

				if (texture == nullptr)
					m_device->Log()->Fatal("TriangleRenderer - Could not create the texture!");

				m_cbuffer = m_device->CreateConstantBuffer<float>();
				
				const Size3 TEXTURE_SIZE = texture->Size();
				texture->Map();
				texture->Unmap(true);
				m_sampler = texture->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();

				m_imageUpdateThread = std::thread(UpdateThread, m_cbuffer, texture, m_instanceOffsetBuffer.Buffer(), &m_rendererAlive);
			}

			TriangleRenderer::~TriangleRenderer() {
				m_rendererAlive = false;
				m_imageUpdateThread.join();
			}

			Reference<Object> TriangleRenderer::CreateEngineData(RenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			ShaderCache* TriangleRenderer::GetShaderCache()const { return m_shaderCache; }

			void TriangleRenderer::Render(Object* engineData, Pipeline::CommandBufferInfo bufferInfo) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);
				data->Render(bufferInfo);
			}

			VertexBuffer* TriangleRenderer::PositionBuffer() { return &m_positionBuffer; }

			InstanceBuffer* TriangleRenderer::InstanceOffsetBuffer() { return &m_instanceOffsetBuffer; }


			TriangleRenderer::VertexPositionBuffer::VertexPositionBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(6);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(-0.5f, -0.25f);
				positions[1] = Vector2(-0.75f, -0.75f);
				positions[2] = Vector2(-0.25f, -0.75f);
				positions[3] = Vector2(-0.5f, 0.25f);
				positions[4] = Vector2(-0.25f, 0.75f);
				positions[5] = Vector2(-0.75f, 0.75f);
				m_buffer->Unmap(true);
			}

			Buffer* TriangleRenderer::ConstantBuffer()const { return m_cbuffer; }

			TextureSampler* TriangleRenderer::Sampler()const { return m_sampler; }

			Reference<ArrayBuffer> TriangleRenderer::VertexPositionBuffer::Buffer() { return m_buffer; }

			size_t TriangleRenderer::VertexPositionBuffer::AttributeCount()const { return 1; }

			VertexBuffer::AttributeInfo TriangleRenderer::VertexPositionBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 0;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}

			size_t TriangleRenderer::VertexPositionBuffer::BufferElemSize()const { return sizeof(Vector2); }


			TriangleRenderer::InstanceOffsetBuffer::InstanceOffsetBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(2);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(0.0f, 0.0f);
				positions[1] = Vector2(1.0f, 0.15f);
				m_buffer->Unmap(true);
			}

			Reference<ArrayBuffer> TriangleRenderer::InstanceOffsetBuffer::Buffer() { return m_buffer; }

			size_t TriangleRenderer::InstanceOffsetBuffer::AttributeCount()const { return 1; }

			VertexBuffer::AttributeInfo TriangleRenderer::InstanceOffsetBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 1;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}

			size_t TriangleRenderer::InstanceOffsetBuffer::BufferElemSize()const { return sizeof(Vector2); }
		}
	}
}
#pragma warning(default: 26812)
