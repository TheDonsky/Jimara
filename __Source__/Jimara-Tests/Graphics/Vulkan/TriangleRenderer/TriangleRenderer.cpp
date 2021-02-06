#include "TriangleRenderer.h"
#include "Graphics/Vulkan/Pipeline/VulkanGraphicsPipeline.h"
#include "Graphics/Vulkan/Pipeline/VulkanFrameBuffer.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				class EnvironmentPipeline : public VulkanPipeline {
				public:
					inline EnvironmentPipeline(VulkanDevice* device, PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers)
						: VulkanPipeline(device, descriptor, maxInFlightCommandBuffers) {}

					inline void UpdateBindings(VulkanCommandRecorder* commandRecorder) { UpdateDescriptors(commandRecorder); }
					inline void SetBindings(VulkanCommandRecorder* commandRecorder) { SetDescriptors(commandRecorder, VK_PIPELINE_BIND_POINT_GRAPHICS); }
				};

				class TriangleRendererData : public VulkanImageRenderer::EngineData {
				private:
					Reference<VulkanRenderPass> m_renderPass;
					std::vector<Reference<VulkanFrameBuffer>> m_frameBuffers;
					ArrayBufferReference<uint32_t> m_indexBuffer;
					Reference<EnvironmentPipeline> m_environmentPipeline;
					Reference<VulkanGraphicsPipeline> m_renderPipeline;

					TriangleRenderer* GetRenderer()const { return dynamic_cast<TriangleRenderer*>(Renderer()); }

					class EnvironmentDescriptor 
						: public virtual PipelineDescriptor
						, public virtual PipelineDescriptor::BindingSetDescriptor {
					private:
						TriangleRendererData* m_data;

					public:
						inline EnvironmentDescriptor(TriangleRendererData* data) : m_data(data) {}

						inline virtual size_t BindingSetCount()const override { return 1; }

						inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
							return (PipelineDescriptor::BindingSetDescriptor*)this;
						}

						virtual bool SetByEnvironment()const override {
							return false;
						}

						virtual size_t ConstantBufferCount()const {
							return 1;
						}

						virtual BindingInfo ConstantBufferInfo(size_t index)const {
							return { StageMask(PipelineStage::VERTEX), 0 };
						}

						virtual Reference<Buffer> ConstantBuffer(size_t index) {
							return m_data->GetRenderer()->CameraTransform();
						}


						virtual size_t StructuredBufferCount()const override {
							return 1;
						}

						virtual BindingInfo StructuredBufferInfo(size_t index)const {
							return { StageMask(PipelineStage::FRAGMENT), 1 };
						}

						virtual Reference<ArrayBuffer> StructuredBuffer(size_t index) {
							return m_data->GetRenderer()->Lights();
						}


						virtual size_t TextureSamplerCount()const {
							return 0;
						}

						virtual BindingInfo TextureSamplerInfo(size_t index)const {
							return BindingInfo();
						}

						virtual Reference<TextureSampler> Sampler(size_t index) {
							return nullptr;
						}

					} m_environmentDescriptor;

					class Descriptor 
						: public virtual GraphicsPipeline::Descriptor
						, public virtual PipelineDescriptor::BindingSetDescriptor
						, public virtual VulkanGraphicsPipeline::RendererContext {
					private:
						TriangleRendererData* m_data;


						class ExternalEnvironmentDescriptor : public virtual EnvironmentDescriptor {
						public:
							ExternalEnvironmentDescriptor(TriangleRendererData* data) : EnvironmentDescriptor(data) {}

							virtual bool SetByEnvironment()const override { return true; }
						} m_environmentDescriptor;

					public:
						inline Descriptor(TriangleRendererData* data) : m_data(data), m_environmentDescriptor(data) {}

						inline virtual size_t BindingSetCount()const override {
							return 2;
						}

						inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
							return ((index > 0)
								? (PipelineDescriptor::BindingSetDescriptor*)this
								: (PipelineDescriptor::BindingSetDescriptor*)&m_environmentDescriptor);
						}

						virtual bool SetByEnvironment()const override {
							return false;
						}


						virtual size_t ConstantBufferCount()const {
							return 1;
						}

						virtual BindingInfo ConstantBufferInfo(size_t index)const {
							return { StageMask(PipelineStage::VERTEX), 1 };
						}

						virtual Reference<Buffer> ConstantBuffer(size_t index) {
							return m_data->GetRenderer()->ConstantBuffer();
						}


						virtual size_t StructuredBufferCount()const override {
							return 0;
						}

						virtual BindingInfo StructuredBufferInfo(size_t index)const {
							return BindingInfo();
						}

						virtual Reference<ArrayBuffer> StructuredBuffer(size_t index) {
							return nullptr;
						}


						virtual size_t TextureSamplerCount()const {
							return 1;
						}

						virtual BindingInfo TextureSamplerInfo(size_t index)const {
							return { StageMask(PipelineStage::FRAGMENT), 0 };
						}

						virtual Reference<TextureSampler> Sampler(size_t index) {
							return m_data->GetRenderer()->Sampler();
						}


						inline virtual Reference<Shader> VertexShader() override {
							return m_data->GetRenderer()->ShaderCache()->GetShader("Shaders/TriangleRenderer.vert.spv", false);
						}

						inline virtual Reference<Shader> FragmentShader() override {
							return m_data->GetRenderer()->ShaderCache()->GetShader("Shaders/TriangleRenderer.frag.spv", true);
						}

						inline virtual size_t VertexBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override {
							return m_data->GetRenderer()->PositionBuffer();
						}

						inline virtual size_t InstanceBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override {
							return m_data->GetRenderer()->InstanceOffsetBuffer();
						}

						inline virtual ArrayBufferReference<uint32_t> IndexBuffer() override {
							return m_data->m_indexBuffer;
						}

						inline virtual size_t IndexCount() override {
							return m_data->GetRenderer()->PositionBuffer()->Buffer()->ObjectCount();
						}

						inline virtual size_t InstanceCount() override {
							return m_data->GetRenderer()->InstanceOffsetBuffer()->Buffer()->ObjectCount();
						}


						inline virtual VulkanRenderPass* RenderPass() override {
							return m_data->m_renderPass;
						}

						inline virtual Size2 TargetSize() override {
							return m_data->EngineInfo()->TargetSize();
						}

						virtual size_t TargetCount() override {
							return m_data->EngineInfo()->ImageCount();
						}
					} m_pipelineDescriptor;


				public:
					inline TriangleRendererData(TriangleRenderer* renderer, VulkanRenderEngineInfo* engineInfo) 
						: VulkanImageRenderer::EngineData(renderer, engineInfo), m_renderPass(VK_NULL_HANDLE)
						, m_environmentDescriptor(this), m_pipelineDescriptor(this) {

						Texture::PixelFormat pixelFormat = VulkanImage::PixelFormatFromNativeFormat(EngineInfo()->ImageFormat());
						m_renderPass = Object::Instantiate<VulkanRenderPass>(EngineInfo()->VulkanDevice(), GraphicsSettings::MSAA::SAMPLE_COUNT_1
							, 1, &pixelFormat, Texture::PixelFormat::OTHER, false);
						
						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							VulkanImage* image = engineInfo->Image(i);
							Reference<VulkanStaticImageView> view = image->CreateView(TextureView::ViewType::VIEW_2D);
							if (image == nullptr)
								engineInfo->Log()->Fatal("TriangleRenderer - Image is null");
							else if (view == nullptr)
								engineInfo->Log()->Fatal("TriangleRenderer - Could not create image view!");
							else m_frameBuffers.push_back(Object::Instantiate<VulkanFrameBuffer>(m_renderPass, &view, nullptr, nullptr));
						}

						m_environmentPipeline = Object::Instantiate<EnvironmentPipeline>(
							m_pipelineDescriptor.RenderPass()->Device(), &m_environmentDescriptor, EngineInfo()->ImageCount());

						m_renderPipeline = Object::Instantiate<VulkanGraphicsPipeline>(&m_pipelineDescriptor, &m_pipelineDescriptor);
					}

					inline virtual ~TriangleRendererData() {
						m_environmentPipeline = nullptr;
						m_renderPipeline = nullptr;
					}

					inline VkRenderPass RenderPass()const { return *m_renderPass; }

					inline VulkanFrameBuffer* FrameBuffer(size_t imageId)const { return m_frameBuffers[imageId]; }

					inline EnvironmentPipeline* Environment()const { return m_environmentPipeline; }

					inline VulkanGraphicsPipeline* Pipeline()const { return m_renderPipeline; }
				};
			}
			
			namespace {
				inline static void TextureUpdateThread(BufferReference<float> scale, ImageTexture* texture, ArrayBufferReference<Vector2> offsetBuffer, volatile bool* alive) {
					Stopwatch stopwatch;
					while (*alive) {
						const float time = stopwatch.Elapsed();

						scale.Map() = sin(time) + 0.0f;
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

			TriangleRenderer::TriangleRenderer(VulkanDevice* device)
				: m_device(device), m_shaderCache(device->CreateShaderCache())
				, m_positionBuffer(device), m_instanceOffsetBuffer(device), m_rendererAlive(true) {
				
				m_cameraTransform = (dynamic_cast<GraphicsDevice*>(m_device.operator->()))->CreateConstantBuffer<Matrix4>();

				m_lights = (dynamic_cast<GraphicsDevice*>(m_device.operator->()))->CreateArrayBuffer<Light>(5);
				{
					Light* lights = m_lights.Map();
					lights[0].position = Vector3(0.0f, 0.0f, 1.0f);
					lights[0].color = Vector3(0.15f, 0.15f, 0.15f);

					lights[1].position = Vector3(1.0f, 1.0f, 1.0f);
					lights[1].color = Vector3(1.0f, 0.0f, 0.0f);

					lights[2].position = Vector3(-1.0f, 1.0f, 1.0f);
					lights[2].color = Vector3(0.0f, 1.0f, 0.0f);

					lights[3].position = Vector3(1.0f, -1.0f, 1.0f);
					lights[3].color = Vector3(0.0f, 0.0f, 1.0f);

					lights[4].position = Vector3(-1.0f, -1.0f, 1.0f);
					lights[4].color = Vector3(0.5f, 0.0f, 0.5f);
					m_lights->Unmap(true);
				}

				m_cbuffer = (dynamic_cast<GraphicsDevice*>(m_device.operator->()))->CreateConstantBuffer<float>();

				m_texture = (dynamic_cast<GraphicsDevice*>(m_device.operator->()))->CreateTexture(
					Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R8G8B8A8_UNORM, Size3(256, 256, 1), 1, true);
				
				if (m_texture == nullptr)
					m_device->Log()->Fatal("TriangleRenderer - Could not create the texture!");
				
				const Size3 TEXTURE_SIZE = m_texture->Size();
				m_texture->Map();
				m_texture->Unmap(true);
				m_sampler = m_texture->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();
				m_imageUpdateThread = std::thread(TextureUpdateThread, m_cbuffer, m_texture, m_instanceOffsetBuffer.Buffer(), &m_rendererAlive);
			}

			TriangleRenderer::~TriangleRenderer() {
				m_rendererAlive = false;
				m_imageUpdateThread.join();
			}

			Reference<VulkanImageRenderer::EngineData> TriangleRenderer::CreateEngineData(VulkanRenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			Graphics::ShaderCache* TriangleRenderer::ShaderCache()const {
				return m_shaderCache;
			}

			void TriangleRenderer::Render(EngineData* engineData, VulkanCommandRecorder* commandRecorder) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);
				VkCommandBuffer commandBuffer = commandRecorder->CommandBuffer();
				if (commandBuffer == VK_NULL_HANDLE)
					engineData->EngineInfo()->Log()->Fatal("TriangleRenderer - Command buffer not provided");

				// Update camera perspective
				{
					Size2 size = engineData->EngineInfo()->TargetSize();
					Matrix4 projection = glm::perspective(glm::radians(45.0f), (float)size.x / (float)size.y, 0.001f, 10000.0f);
					projection[1][1] *= -1.0f;

					m_cameraTransform.Map() = projection 
						* glm::lookAt(glm::vec3(2.0f, 2.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f))
						* glm::rotate(glm::mat4(1.0f), m_stopwatch.Elapsed() * glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					m_cameraTransform->Unmap(true);
				}

				// Update pipeline buffers if there's a need to
				data->Environment()->UpdateBindings(commandRecorder);
				data->Pipeline()->UpdateBindings(commandRecorder);

				// Begin render pass
				{
					VkRenderPassBeginInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					info.renderPass = data->RenderPass();
					info.framebuffer = *data->FrameBuffer(commandRecorder->CommandBufferIndex());
					info.renderArea.offset = { 0, 0 };
					Size2 size = engineData->EngineInfo()->TargetSize();
					info.renderArea.extent = { size.x, size.y };
					VkClearValue clearValue = {};
					clearValue.color = { 0.0f, 0.25f, 0.25f, 1.0f };
					info.clearValueCount = 1;
					info.pClearValues = &clearValue;
					vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
				}

				// Draw triangle
				data->Environment()->SetBindings(commandRecorder);
				data->Pipeline()->Render(commandRecorder);

				// End render pass
				vkCmdEndRenderPass(commandBuffer);
			}

			VertexBuffer* TriangleRenderer::PositionBuffer() {
				return &m_positionBuffer;
			}

			InstanceBuffer* TriangleRenderer::InstanceOffsetBuffer() {
				return &m_instanceOffsetBuffer;
			}


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

			Buffer* TriangleRenderer::CameraTransform()const {
				return m_cameraTransform;
			}

			ArrayBufferReference<TriangleRenderer::Light> TriangleRenderer::Lights()const {
				return m_lights;
			}

			Buffer* TriangleRenderer::ConstantBuffer()const {
				return m_cbuffer;
			}

			TextureSampler* TriangleRenderer::Sampler()const {
				return m_sampler;
			}

			Reference<ArrayBuffer> TriangleRenderer::VertexPositionBuffer::Buffer() {
				return m_buffer;
			}

			size_t TriangleRenderer::VertexPositionBuffer::AttributeCount()const {
				return 1;
			}

			VertexBuffer::AttributeInfo TriangleRenderer::VertexPositionBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 0;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}

			size_t TriangleRenderer::VertexPositionBuffer::BufferElemSize()const {
				return sizeof(Vector2);
			}


			TriangleRenderer::InstanceOffsetBuffer::InstanceOffsetBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(2);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(0.0f, 0.0f);
				positions[1] = Vector2(1.0f, 0.15f);
				m_buffer->Unmap(true);
			}

			Reference<ArrayBuffer> TriangleRenderer::InstanceOffsetBuffer::Buffer() {
				return m_buffer;
			}

			size_t TriangleRenderer::InstanceOffsetBuffer::AttributeCount()const {
				return 1;
			}

			VertexBuffer::AttributeInfo TriangleRenderer::InstanceOffsetBuffer::Attribute(size_t index)const {
				AttributeInfo info = {};
				{
					info.location = 1;
					info.offset = 0;
					info.type = AttributeInfo::Type::FLOAT2;
				}
				return info;
			}

			size_t TriangleRenderer::InstanceOffsetBuffer::BufferElemSize()const {
				return sizeof(Vector2);
			}
		}
	}
}
#pragma warning(default: 26812)
