#include "TriangleRenderer.h"
#include <sstream>
#include <math.h>


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Test {
			namespace {
				class EnvironmentDescriptor
					: public virtual PipelineDescriptor
					, public virtual PipelineDescriptor::BindingSetDescriptor {
				public:
					inline EnvironmentDescriptor() {}

					inline virtual size_t BindingSetCount()const override { return 1; }

					inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
						return (PipelineDescriptor::BindingSetDescriptor*)this;
					}

					virtual bool SetByEnvironment()const override { return true; }

					virtual size_t ConstantBufferCount()const override { return 1; }
					virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { StageMask(PipelineStage::VERTEX), 0 }; }
					virtual Reference<Buffer> ConstantBuffer(size_t)const override { return nullptr; }

					virtual size_t StructuredBufferCount()const override { return 1; }
					virtual BindingInfo StructuredBufferInfo(size_t index)const override { return { StageMask(PipelineStage::FRAGMENT), 1 }; }
					virtual Reference<ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }

					virtual size_t TextureSamplerCount()const override { return 0; }
					virtual BindingInfo TextureSamplerInfo(size_t)const { return BindingInfo(); }
					virtual Reference<TextureSampler> Sampler(size_t)const { return nullptr; }

					static EnvironmentDescriptor* Instance() { 
						static EnvironmentDescriptor INSTANCE; 
						return &INSTANCE; 
					}
				};

				class MeshRendererData : public virtual Object {
				private:
					const Reference<TriMesh> m_mesh;
					const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
					
					ArrayBufferReference<MeshVertex> m_vertices;
					ArrayBufferReference<uint32_t> m_indices;
					
					Reference<TextureSampler> m_sampler;
					Reference<ShaderCache> m_shaderCache;
					Reference<RenderPass> m_rendererPass;
					Reference<GraphicsPipeline> m_renderPipeline;

					struct Descriptor
						: public virtual GraphicsPipeline::Descriptor
						, public virtual PipelineDescriptor::BindingSetDescriptor
						, public virtual Graphics::VertexBuffer {
						MeshRendererData* m_data;

						inline Descriptor(MeshRendererData* data) : m_data(data) {}

						// PipelineDescriptor:
						inline virtual size_t BindingSetCount()const override { return 2; }

						inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
							return index <= 0 
								? ((PipelineDescriptor::BindingSetDescriptor*)EnvironmentDescriptor::Instance()) 
								: ((PipelineDescriptor::BindingSetDescriptor*)this);
						}


						// BindingSetDecriptor:
						virtual bool SetByEnvironment()const override { return false; }

						virtual size_t ConstantBufferCount()const override { return 0; }
						virtual BindingInfo ConstantBufferInfo(size_t index)const override { return BindingInfo(); }
						virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return nullptr; }

						virtual size_t StructuredBufferCount()const override { return 0; }
						virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo(); }
						virtual Reference<ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }

						virtual size_t TextureSamplerCount()const override { return 1; }
						virtual BindingInfo TextureSamplerInfo(size_t)const override { return { StageMask(PipelineStage::FRAGMENT), 0 }; }
						virtual Reference<TextureSampler> Sampler(size_t)const override { return m_data->m_sampler; }


						// Graphics pipeline:
						inline virtual Reference<Shader> VertexShader() override { return m_data->m_shaderCache->GetShader("Shaders/SampleMeshShader.vert.spv", false); }
						inline virtual Reference<Shader> FragmentShader() override { return m_data->m_shaderCache->GetShader("Shaders/SampleMeshShader.frag.spv", false); }

						inline virtual size_t VertexBufferCount() override { return 1; }
						inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override { return (Graphics::VertexBuffer*)this; }

						inline virtual size_t InstanceBufferCount() override { return 0; }
						inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t) override { return nullptr; }

						inline virtual ArrayBufferReference<uint32_t> IndexBuffer() override { return m_data->m_indices; }

						inline virtual size_t IndexCount() override {  return m_data->m_indices->ObjectCount(); }
						inline virtual size_t InstanceCount() override { return 1; }


						// Vertex buffer:
						inline virtual size_t AttributeCount()const override { return 3; }

						inline virtual AttributeInfo Attribute(size_t index)const override {
							static const AttributeInfo INFOS[] = {
								{ AttributeInfo::Type::FLOAT3, 0, offsetof(MeshVertex, position) },
								{ AttributeInfo::Type::FLOAT3, 1, offsetof(MeshVertex, normal) },
								{ AttributeInfo::Type::FLOAT2, 2, offsetof(MeshVertex, uv) },
							};
							return INFOS[index];
						}

						inline virtual size_t BufferElemSize()const override { return sizeof(MeshVertex); }

						inline virtual Reference<ArrayBuffer> Buffer() override { return m_data->m_vertices; }
					} m_descriptor;

					inline void OnMeshDirty(Graphics::GraphicsMesh*) {
						PipelineDescriptor::WriteLock lock(m_descriptor);
						m_graphicsMesh->GetBuffers(m_vertices, m_indices);
					}

				public:
					inline MeshRendererData(TriMesh* mesh, ShaderCache* shaderCache, RenderPass* renderPass, size_t maxInFlightCommandBuffers, TriangleRenderer* renderer)
						: m_mesh(mesh), m_graphicsMesh(renderer->GraphicsMeshCache()->GetMesh(mesh, false)), m_shaderCache(shaderCache), m_rendererPass(renderPass), m_descriptor(this) {
						m_sampler = TriMesh::Reader(m_mesh).Name() == "bear"
							? renderer->BearTexture()->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler()
							: Reference<TextureSampler>(renderer->Sampler());
						m_renderPipeline = m_rendererPass->CreateGraphicsPipeline(&m_descriptor, maxInFlightCommandBuffers);
						m_graphicsMesh->OnInvalidate() += Callback<Graphics::GraphicsMesh*>(&MeshRendererData::OnMeshDirty, this);
						m_graphicsMesh->GetBuffers(m_vertices, m_indices);
					}

					inline ~MeshRendererData() {
						m_graphicsMesh->OnInvalidate() -= Callback<Graphics::GraphicsMesh*>(&MeshRendererData::OnMeshDirty, this);
					}


					// Pipeline getter for usage:
					inline GraphicsPipeline* Pipeline()const { return m_renderPipeline; }
				};

				class TriangleRendererData : public Object {
				private:
					const Reference<TriangleRenderer> m_renderer;
					const Reference<RenderEngineInfo> m_engineInfo;
					Reference<RenderPass> m_renderPass;
					std::vector<Reference<FrameBuffer>> m_frameBuffers;
					ArrayBufferReference<uint32_t> m_indexBuffer;
					Reference<Pipeline> m_environmentPipeline;
					Reference<GraphicsPipeline> m_renderPipeline;

					std::vector<Reference<MeshRendererData>> m_meshRenderers;

					class Environment : public virtual EnvironmentDescriptor {
					private:
						TriangleRendererData* m_data;
					public:
						inline Environment(TriangleRendererData* data) : m_data(data) {}
						virtual bool SetByEnvironment()const override { return false; }
						virtual Reference<Buffer> ConstantBuffer(size_t index)const override { return m_data->m_renderer->CameraTransform(); }
						virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override{ return m_data->m_renderer->Lights(); }
					} m_environmentDescriptor;

					class Descriptor 
						: public virtual GraphicsPipeline::Descriptor
						, public virtual PipelineDescriptor::BindingSetDescriptor {
					private:
						TriangleRendererData* m_data;

					public:
						inline Descriptor(TriangleRendererData* data) : m_data(data) {}

						inline virtual size_t BindingSetCount()const override {
							return 2;
						}

						inline virtual PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override {
							return ((index > 0)
								? (PipelineDescriptor::BindingSetDescriptor*)this
								: (PipelineDescriptor::BindingSetDescriptor*)EnvironmentDescriptor::Instance());
						}

						virtual bool SetByEnvironment()const override {
							return false;
						}


						virtual size_t ConstantBufferCount()const override {
							return 1;
						}

						virtual BindingInfo ConstantBufferInfo(size_t index)const {
							return { StageMask(PipelineStage::VERTEX), 1 };
						}

						virtual Reference<Buffer> ConstantBuffer(size_t index)const override {
							return m_data->m_renderer->ConstantBuffer();
						}


						virtual size_t StructuredBufferCount()const override {
							return 0;
						}

						virtual BindingInfo StructuredBufferInfo(size_t index)const override {
							return BindingInfo();
						}

						virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override {
							return nullptr;
						}


						virtual size_t TextureSamplerCount()const override {
							return 1;
						}

						virtual BindingInfo TextureSamplerInfo(size_t index)const override {
							return { StageMask(PipelineStage::FRAGMENT), 0 };
						}

						virtual Reference<TextureSampler> Sampler(size_t index)const override {
							return m_data->m_renderer->Sampler();
						}


						inline virtual Reference<Shader> VertexShader() override {
							return m_data->m_renderer->ShaderCache()->GetShader("Shaders/TriangleRenderer.vert.spv", false);
						}

						inline virtual Reference<Shader> FragmentShader() override {
							return m_data->m_renderer->ShaderCache()->GetShader("Shaders/TriangleRenderer.frag.spv", true);
						}

						inline virtual size_t VertexBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override {
							return m_data->m_renderer->PositionBuffer();
						}

						inline virtual size_t InstanceBufferCount() override {
							return 1;
						}

						inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override {
							return m_data->m_renderer->InstanceOffsetBuffer();
						}

						inline virtual ArrayBufferReference<uint32_t> IndexBuffer() override {
							return m_data->m_indexBuffer;
						}

						inline virtual size_t IndexCount() override {
							return m_data->m_renderer->PositionBuffer()->Buffer()->ObjectCount();
						}

						inline virtual size_t InstanceCount() override {
							return m_data->m_renderer->InstanceOffsetBuffer()->Buffer()->ObjectCount();
						}
					} m_pipelineDescriptor;


				public:
					inline TriangleRendererData(TriangleRenderer* renderer, RenderEngineInfo* engineInfo) 
						: m_renderer(renderer), m_engineInfo(engineInfo), m_renderPass(nullptr)
						, m_environmentDescriptor(this), m_pipelineDescriptor(this) {

						Texture::PixelFormat pixelFormat = engineInfo->ImageFormat();
						
						Reference<TextureView> colorAttachment = engineInfo->Device()->CreateMultisampledTexture(
							Texture::TextureType::TEXTURE_2D, pixelFormat, Size3(engineInfo->ImageSize(), 1), 1, Texture::Multisampling::MAX_AVAILABLE)
							->CreateView(TextureView::ViewType::VIEW_2D);

						Reference<TextureView> depthAttachment = engineInfo->Device()->CreateMultisampledTexture(
							Texture::TextureType::TEXTURE_2D, engineInfo->Device()->GetDepthFormat()
							, colorAttachment->TargetTexture()->Size(), 1, colorAttachment->TargetTexture()->SampleCount())
							->CreateView(TextureView::ViewType::VIEW_2D);

						m_renderPass = engineInfo->Device()->CreateRenderPass(
							colorAttachment->TargetTexture()->SampleCount(), 1, &pixelFormat, depthAttachment->TargetTexture()->ImageFormat(), true);

						for (size_t i = 0; i < engineInfo->ImageCount(); i++) {
							Reference<TextureView> resolveView = engineInfo->Image(i)->CreateView(TextureView::ViewType::VIEW_2D);
							m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveView));
						}

						m_environmentPipeline = engineInfo->Device()->CreateEnvironmentPipeline(&m_environmentDescriptor, engineInfo->ImageCount());

						m_renderPipeline = m_renderPass->CreateGraphicsPipeline(&m_pipelineDescriptor, engineInfo->ImageCount());

						for (size_t i = 0; i < renderer->Meshes().size(); i++)
							m_meshRenderers.push_back(Object::Instantiate<MeshRendererData>(renderer->Meshes()[i], renderer->ShaderCache(), m_renderPass, m_frameBuffers.size(), renderer));
						for (size_t i = 0; i < renderer->Meshes().size(); i++)
							if (m_meshRenderers[i]->RefCount() != 1)
								engineInfo->Device()->Log()->Error([&]() {
								std::stringstream stream;
								stream << "m_meshRenderers[" << i << "]->RefCount() = " << m_meshRenderers[i]->RefCount();
								return stream.str();
									}());
					}

					inline virtual ~TriangleRendererData() {
						m_environmentPipeline = nullptr;
						m_renderPipeline = nullptr;
						m_meshRenderers.clear();
					}

					inline RenderPass* GetRenderPass()const { return m_renderPass; }

					inline FrameBuffer* GetFrameBuffer(size_t imageId)const { return m_frameBuffers[imageId]; }

					inline Pipeline* Environment()const { return m_environmentPipeline; }

					inline GraphicsPipeline* TrianglePipeline()const { return m_renderPipeline; }

					inline GraphicsPipeline* MeshPipeline(size_t index)const { return m_meshRenderers[index]->Pipeline(); }

					inline RenderEngineInfo* EngineInfo()const { return m_engineInfo; }
				};
			}
			
			namespace {
				inline static void TextureUpdateThread(
					BufferReference<float> scale, 
					ImageTexture* texture, 
					ArrayBufferReference<Vector2> offsetBuffer,
					Reference<TriMesh> meshToDeform,
					volatile bool* alive) {
					
					const TriMesh baseMesh(*dynamic_cast<Mesh<MeshVertex, TriangleFace>*>(meshToDeform.operator->()));

					Stopwatch stopwatch;
					while (*alive) {
						const float time = stopwatch.Elapsed();

						scale.Map() = sin(0.15f * time) + 0.0f;
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

						{
							TriMesh::Writer meshWriter(meshToDeform);
							TriMesh::Reader baseMeshReader(baseMesh);
							for (uint32_t vert = 0; vert < baseMeshReader.VertCount(); vert++) {
								const Vector3 basePosition = baseMeshReader.Vert(vert).position;
								const Vector3 baseOffset = Vector3(
									cos(time + 8.0f * basePosition.y),
									cos(time + 4.25f * basePosition.x) * sin(time + 4.25f * basePosition.z),
									sin(time + 8.0f * basePosition.y));
								const float offsetMultiplier = 0.1f * sin(time + basePosition.x + basePosition.z);
								meshWriter.Verts()[vert].position = basePosition + offsetMultiplier * baseOffset;
							}
						}

						std::this_thread::sleep_for(std::chrono::milliseconds(8));
					}
				}
			}

			TriangleRenderer::TriangleRenderer(GraphicsDevice* device)
				: m_device(device), m_shaderCache(device->CreateShaderCache())
				, m_positionBuffer(device), m_instanceOffsetBuffer(device), m_rendererAlive(true)
				, m_graphicsMeshCache(Object::Instantiate<Graphics::GraphicsMeshCache>(device))
				, m_meshes(TriMesh::FromOBJ("Assets/Meshes/Bear/ursus_proximus.obj", device->Log())) {
				
				m_meshes.push_back(TriMesh::Box(Vector3(-1.5f, 0.25f, -0.25f), Vector3(-1.0f, 0.75f, 0.25f), "bear"));
				m_meshes.push_back(TriMesh::Sphere(Vector3(-1.25f, 1.15f, 0.0f), 0.25f, 16, 8, "bear"));
				m_meshes.push_back(TriMesh::Box(Vector3(-1.5f, 1.5f, -0.25f), Vector3(-1.0f, 2.0f, 0.25f), ""));
				m_meshes.push_back(TriMesh::Sphere(Vector3(-1.25f, 2.65f, 0.0f), 0.25f, 8, 4, ""));

				m_meshes.push_back(TriMesh::Box(Vector3(1.5f, 0.75f, 0.25f), Vector3(1.0f, 0.25f, -0.25f), "bear"));
				m_meshes.push_back(TriMesh::Sphere(Vector3(1.25f, 1.15f, 0.0f), -0.25f, 8, 4, "bear"));
				m_meshes.push_back(TriMesh::Box(Vector3(1.5f, 2.0f, 0.25f), Vector3(1.0f, 1.5f, -0.25f), ""));
				m_meshes.push_back(TriMesh::Sphere(Vector3(1.25f, 2.65f, 0.0f), -0.25f, 16, 8, ""));

				{
					const Reference<TriMesh> smoothSphere = TriMesh::Sphere(Vector3(0.0f, 2.25f, 0.0f), 0.45f, 16, 8, "");
					m_meshes.push_back(TriMesh::ShadeFlat(smoothSphere));
				}

				m_meshes.push_back(TriMesh::Sphere(Vector3(0.0f, 1.25f, -1.5f), 0.75f, 32, 16, ""));


				m_cameraTransform = m_device->CreateConstantBuffer<Matrix4>();

				m_lights = m_device->CreateArrayBuffer<Light>(5);
				{
					Light* lights = m_lights.Map();
					lights[0].position = Vector3(0.0f, 4.0f, 0.0f);
					lights[0].color = Vector3(2.0f, 2.0f, 2.0f);

					lights[1].position = Vector3(2.0f, 1.0f, 2.0f);
					lights[1].color = Vector3(4.0f, 0.0f, 0.0f);

					lights[2].position = Vector3(-2.0f, 1.0f, 2.0f);
					lights[2].color = Vector3(0.0f, 4.0f, 0.0f);

					lights[3].position = Vector3(2.0f, 1.0f, -2.0f);
					lights[3].color = Vector3(0.0f, 0.0f, 4.0f);

					lights[4].position = Vector3(-2.0f, 1.0f, -2.0f);
					lights[4].color = Vector3(2.0f, 0.0f, 2.0f);
					m_lights->Unmap(true);
				}

				m_cbuffer = m_device->CreateConstantBuffer<float>();

				m_texture = m_device->CreateTexture(
					Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R8G8B8A8_UNORM, Size3(256, 256, 1), 1, true);
				
				if (m_texture == nullptr)
					m_device->Log()->Fatal("TriangleRenderer - Could not create the texture!");
				
				const Size3 TEXTURE_SIZE = m_texture->Size();
				m_texture->Map();
				m_texture->Unmap(true);
				m_sampler = m_texture->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();
				m_imageUpdateThread = std::thread(TextureUpdateThread, m_cbuffer, m_texture, m_instanceOffsetBuffer.Buffer(), m_meshes.back(), &m_rendererAlive);

				m_bearTexture = ImageTexture::LoadFromFile(m_device, "Assets/Meshes/Bear/bear_diffuse.png", true);
			}

			TriangleRenderer::~TriangleRenderer() {
				m_rendererAlive = false;
				m_imageUpdateThread.join();
			}

			Reference<Object> TriangleRenderer::CreateEngineData(RenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			Graphics::ShaderCache* TriangleRenderer::ShaderCache()const {
				return m_shaderCache;
			}

			void TriangleRenderer::Render(Object* engineData, Pipeline::CommandBufferInfo bufferInfo) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);

				// Update camera perspective
				{
					Size2 size = data->EngineInfo()->ImageSize();
					Matrix4 projection = glm::perspective(glm::radians(64.0f), (float)size.x / (float)size.y, 0.001f, 10000.0f);
					projection[1][1] *= -1.0f;
					float time = m_stopwatch.Elapsed();
					m_cameraTransform.Map() = projection 
						* glm::lookAt(glm::vec3(2.0f, 1.5f + 1.2f * cos(time * glm::radians(15.0f)), 2.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))
						* glm::rotate(glm::mat4(1.0f), time * glm::radians(5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					m_cameraTransform->Unmap(true);
				}

				// Begin render pass
				const Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);
				data->GetRenderPass()->BeginPass(bufferInfo.commandBuffer, data->GetFrameBuffer(bufferInfo.inFlightBufferId), &CLEAR_VALUE);

				// Update pipeline buffers if there's a need to
				data->Environment()->Execute(bufferInfo);

				// Draw geometry
				data->Environment()->Execute(bufferInfo);
				data->TrianglePipeline()->Execute(bufferInfo);
				for (size_t i = 0; i < m_meshes.size(); i++)
					data->MeshPipeline(i)->Execute(bufferInfo);

				// End render pass
				data->GetRenderPass()->EndPass(bufferInfo.commandBuffer);
			}

			VertexBuffer* TriangleRenderer::PositionBuffer() {
				return &m_positionBuffer;
			}

			InstanceBuffer* TriangleRenderer::InstanceOffsetBuffer() {
				return &m_instanceOffsetBuffer;
			}
			
			Graphics::GraphicsMeshCache* TriangleRenderer::GraphicsMeshCache()const {
				return m_graphicsMeshCache;
			}

			const std::vector<Reference<TriMesh>>& TriangleRenderer::Meshes()const {
				return m_meshes;
			}

			Texture* TriangleRenderer::BearTexture()const {
				return m_bearTexture;
			}


			TriangleRenderer::VertexPositionBuffer::VertexPositionBuffer(GraphicsDevice* device) {
				m_buffer = device->CreateArrayBuffer<Vector2>(6);
				Vector2* positions = m_buffer.Map();
				positions[0] = Vector2(-0.5f, -0.25f);
				positions[1] = Vector2(-0.25f, -0.75f);
				positions[2] = Vector2(-0.75f, -0.75f);
				positions[3] = Vector2(-0.5f, 0.25f);
				positions[4] = Vector2(-0.75f, 0.75f);
				positions[5] = Vector2(-0.25f, 0.75f);
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
