#include "TriangleRenderer.h"
#include "Components/Transform.h"
#include "Components/MeshRenderer.h"
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

				class TriangleRendererData : public Object {
				private:
					const Reference<TriangleRenderer> m_renderer;
					const Reference<RenderEngineInfo> m_engineInfo;
					Reference<RenderPass> m_renderPass;
					std::vector<Reference<FrameBuffer>> m_frameBuffers;
					ArrayBufferReference<uint32_t> m_indexBuffer;
					Reference<Pipeline> m_environmentPipeline;
					Reference<GraphicsPipeline> m_renderPipeline;

					struct PipelineRef {
						Reference<GraphicsPipeline::Descriptor> desc;
						Reference<GraphicsPipeline> pipeline;

						PipelineRef(GraphicsPipeline::Descriptor* d = nullptr, RenderPass* pass = nullptr, size_t commandBuffers = 0)
							: desc(d), pipeline((d == nullptr) ? nullptr : pass->CreateGraphicsPipeline(d, commandBuffers)) {}
					};
					std::unordered_map<GraphicsPipeline::Descriptor*, PipelineRef> m_scenePipelines;

					void AddGraphicsPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count, GraphicsObjectSet*) {
						for (size_t i = 0; i < count; i++) {
							std::unordered_map<GraphicsPipeline::Descriptor*, PipelineRef>::iterator it = m_scenePipelines.find(descriptors[i]);
							if (it != m_scenePipelines.end()) continue;
							m_scenePipelines[descriptors[i]] = PipelineRef(descriptors[i], m_renderPass, m_engineInfo->ImageCount());
						}
					};

					void RemoveGraphicsPipelines(const Reference<GraphicsPipeline::Descriptor>* descriptors, size_t count, GraphicsObjectSet*) {
						for (size_t i = 0; i < count; i++) {
							std::unordered_map<GraphicsPipeline::Descriptor*, PipelineRef>::iterator it = m_scenePipelines.find(descriptors[i]);
							if (it != m_scenePipelines.end()) m_scenePipelines.erase(it);
						}
					};

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
							return m_data->m_renderer->GetScene()->Context()->Context()->ShaderCache()->GetShader("Shaders/TriangleRenderer.vert.spv", false);
						}

						inline virtual Reference<Shader> FragmentShader() override {
							return m_data->m_renderer->GetScene()->Context()->Context()->ShaderCache()->GetShader("Shaders/TriangleRenderer.frag.spv", true);
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

						m_renderer->GetScene()->Context()->GraphicsPipelineSet()->AddChangeCallbacks(
							Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>(&TriangleRendererData::AddGraphicsPipelines, this),
							Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>(&TriangleRendererData::RemoveGraphicsPipelines, this));
					}

					inline virtual ~TriangleRendererData() {
						m_renderer->GetScene()->Context()->GraphicsPipelineSet()->RemoveChangeCallbacks(
							Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>(&TriangleRendererData::AddGraphicsPipelines, this),
							Callback<const Reference<GraphicsPipeline::Descriptor>*, size_t, GraphicsObjectSet*>(&TriangleRendererData::RemoveGraphicsPipelines, this));
						m_environmentPipeline = nullptr;
						m_renderPipeline = nullptr;
					}

					inline RenderPass* GetRenderPass()const { return m_renderPass; }

					inline FrameBuffer* GetFrameBuffer(size_t imageId)const { return m_frameBuffers[imageId]; }

					inline void Render(const Pipeline::CommandBufferInfo bufferInfo)const {
						m_environmentPipeline->Execute(bufferInfo);
						m_renderPipeline->Execute(bufferInfo);
						for (std::unordered_map<GraphicsPipeline::Descriptor*, PipelineRef>::const_iterator it = m_scenePipelines.begin(); it != m_scenePipelines.end(); ++it)
							it->second.pipeline->Execute(bufferInfo);
					}

					inline RenderEngineInfo* EngineInfo()const { return m_engineInfo; }
				};
			}
			
			namespace {
				inline static void TextureUpdateThread(
					BufferReference<float> scale, 
					ImageTexture* texture, 
					ArrayBufferReference<Vector2> offsetBuffer,
					Reference<TriMesh> meshToDeform,
					Reference<Transform> transformPlayWith,
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
								const float t = time + basePosition.y;
								const Vector3 baseOffset = Vector3(
									cos(time + 8.0f * basePosition.y),
									cos(time + 4.25f * basePosition.x) * sin(time + 4.25f * basePosition.z),
									sin(time + 8.0f * basePosition.y));
								const float offsetMultiplier = 0.1f * sin(time + basePosition.x + basePosition.z);
								meshWriter.Verts()[vert].position = basePosition + offsetMultiplier * baseOffset;
							}
						}

						{
							transformPlayWith->SetWorldPosition(Vector3(cos(time), 1.5f + 0.25f * sin(time), sin(time)));
							transformPlayWith->SetWorldEulerAngles(Vector3(0.0f, time * 90.0f, 0.0f));
							transformPlayWith->SetLocalScale(Vector3(cos(time * 0.5f)));
						}

						std::this_thread::sleep_for(std::chrono::milliseconds(8));
					}
				}


				class MeshMaterial : public virtual Material {
				private:
					const Reference<Graphics::Shader> m_vertexShader;
					const Reference<Graphics::Shader> m_fragmentShader;
					const Reference<Graphics::TextureSampler> m_sampler;

				public:
					inline MeshMaterial(Graphics::ShaderCache* cache, Graphics::Texture* texture)
						: m_vertexShader(cache->GetShader("Shaders/SampleMeshShader.vert.spv"))
						, m_fragmentShader(cache->GetShader("Shaders/SampleMeshShader.frag.spv"))
						, m_sampler(texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler()) {}

					inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor* EnvironmentDescriptor()const override { 
						return (PipelineDescriptor::BindingSetDescriptor*)EnvironmentDescriptor::Instance();
					}

					inline virtual Reference<Graphics::Shader> VertexShader()const override { return m_vertexShader; }
					inline virtual Reference<Graphics::Shader> FragmentShader()const override { return m_fragmentShader; }

					inline virtual bool SetByEnvironment()const override { return false; }

					inline virtual size_t ConstantBufferCount()const override { return 0; }
					inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return BindingInfo(); }
					inline virtual Reference<Buffer> ConstantBuffer(size_t index)const override { return nullptr; }

					inline virtual size_t StructuredBufferCount()const override { return 0; }
					inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo(); }
					inline virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

					inline virtual size_t TextureSamplerCount()const override { return 1; }
					inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return { StageMask(PipelineStage::FRAGMENT), 0 }; }
					inline virtual Reference<TextureSampler> Sampler(size_t index)const override { return m_sampler; }
				};
			}

			TriangleRenderer::TriangleRenderer(GraphicsDevice* device)
				: m_device(device), m_scene([&]() -> Reference<Scene> {
				Reference<AppContext> context = Object::Instantiate<AppContext>(device);
				return Object::Instantiate<Scene>(context); }())
				, m_positionBuffer(device), m_instanceOffsetBuffer(device), m_rendererAlive(true) {
				

				m_texture = m_device->CreateTexture(
					Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::R8G8B8A8_UNORM, Size3(256, 256, 1), 1, true);

				if (m_texture == nullptr)
					m_device->Log()->Fatal("TriangleRenderer - Could not create the texture!");

				Reference<Graphics::ImageTexture> bearTexture = ImageTexture::LoadFromFile(m_device, "Assets/Meshes/Bear/bear_diffuse.png", true);

				Reference<Material> bearMaterial = Object::Instantiate<MeshMaterial>(m_scene->Context()->Context()->ShaderCache(), bearTexture);
				Reference<Material> envMaterial = Object::Instantiate<MeshMaterial>(m_scene->Context()->Context()->ShaderCache(), m_texture);
				{
					std::vector<Reference<TriMesh>> meshes = TriMesh::FromOBJ("Assets/Meshes/Bear/ursus_proximus.obj", device->Log());
					meshes.push_back(TriMesh::Box(Vector3(-1.5f, 0.25f, -0.25f), Vector3(-1.0f, 0.75f, 0.25f), "bear"));
					meshes.push_back(TriMesh::Sphere(Vector3(-1.25f, 1.15f, 0.0f), 0.25f, 16, 8, "bear"));
					meshes.push_back(TriMesh::Box(Vector3(-1.5f, 1.5f, -0.25f), Vector3(-1.0f, 2.0f, 0.25f), ""));
					meshes.push_back(TriMesh::Sphere(Vector3(-1.25f, 2.65f, 0.0f), 0.25f, 8, 4, ""));

					meshes.push_back(TriMesh::Box(Vector3(1.5f, 0.75f, 0.25f), Vector3(1.0f, 0.25f, -0.25f), "bear"));
					meshes.push_back(TriMesh::Sphere(Vector3(1.25f, 1.15f, 0.0f), -0.25f, 8, 4, "bear"));
					meshes.push_back(TriMesh::Box(Vector3(1.5f, 2.0f, 0.25f), Vector3(1.0f, 1.5f, -0.25f), ""));
					meshes.push_back(TriMesh::Sphere(Vector3(1.25f, 2.65f, 0.0f), -0.25f, 16, 8, ""));

					const Reference<TriMesh> smoothSphere = TriMesh::Sphere(Vector3(0.0f, 2.25f, 0.0f), 0.45f, 16, 8, "");
					meshes.push_back(TriMesh::ShadeFlat(smoothSphere));

					Transform* ursusTransform = Object::Instantiate<Transform>(m_scene->RootObject(), "ursus_proximus");
					ursusTransform->SetLocalScale(Vector3(0.75f));
					for (size_t i = 0; i < meshes.size(); i++) {
						const std::string& name = TriMesh::Reader(meshes[i]).Name();
						MeshRenderer* renderer = Object::Instantiate<MeshRenderer>(ursusTransform, name, meshes[i], (name == "bear") ? bearMaterial : envMaterial);
					}
				}

				Transform* sphereTransform = Object::Instantiate<Transform>(m_scene->RootObject(), "Sphere");
				{
					sphereTransform->SetWorldPosition(Vector3(0.0f, -1.0f, 0.0f));
					Reference<TriMesh> sphere = TriMesh::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.5f, 32, 16, "Sphere");
					Object::Instantiate<MeshRenderer>(sphereTransform, "SphereRenderer_0", sphere, bearMaterial);
					Transform* otherSphereTransform = Object::Instantiate<Transform>(m_scene->RootObject(), "Sphere");
					otherSphereTransform->SetLocalPosition(Vector3(0.0f, 1.0f, -1.0f));
					Object::Instantiate<MeshRenderer>(otherSphereTransform, "SphereRenderer_1", sphere, bearMaterial);
				}

				Reference<TriMesh> deformedMesh = (TriMesh::Sphere(Vector3(0.0f), 0.75f, 32, 16, "Deformed"));
				{
					Transform* deformedTransform = Object::Instantiate<Transform>(m_scene->RootObject(), "Deformed");
					deformedTransform->SetWorldPosition(Vector3(0.0f, 1.0f, 1.5f));
					deformedTransform->SetLocalScale(Vector3(0.25f));
					Object::Instantiate<MeshRenderer>(deformedTransform, "DeformedMesh", deformedMesh, bearMaterial);
				}


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
				
				const Size3 TEXTURE_SIZE = m_texture->Size();
				m_texture->Map();
				m_texture->Unmap(true);
				m_sampler = m_texture->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler();
				m_imageUpdateThread = std::thread(TextureUpdateThread, m_cbuffer, m_texture, m_instanceOffsetBuffer.Buffer(), deformedMesh, sphereTransform, &m_rendererAlive);
			}

			TriangleRenderer::~TriangleRenderer() {
				m_rendererAlive = false;
				m_imageUpdateThread.join();
			}

			Reference<Object> TriangleRenderer::CreateEngineData(RenderEngineInfo* engineInfo) {
				return Object::Instantiate<TriangleRendererData>(this, engineInfo);
			}

			Scene* TriangleRenderer::GetScene()const {
				return m_scene;
			}

			void TriangleRenderer::Render(Object* engineData, Pipeline::CommandBufferInfo bufferInfo) {
				TriangleRendererData* data = dynamic_cast<TriangleRendererData*>(engineData);
				assert(data != nullptr);

				// Update camera perspective
				{
					Size2 size = data->EngineInfo()->ImageSize();
					Matrix4 projection = glm::perspective(glm::radians(64.0f), (float)size.x / (float)size.y, 0.001f, 10000.0f);
					projection[2] *= -1.0f;
					float time = m_stopwatch.Elapsed();
					const Vector3 position = Vector3(2.0f, 1.5f + 1.2f * cos(time * glm::radians(15.0f)), 2.0f);
					const Vector3 target = Vector3(0.0f, 0.0f, 0.0f);
					Matrix4 lookAt = glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
					lookAt[0][0] *= -1;
					lookAt[1][0] *= -1;
					lookAt[2][0] *= -1;
					lookAt[3][0] *= -1;
					lookAt[0][2] *= -1;
					lookAt[1][2] *= -1;
					lookAt[2][2] *= -1;
					lookAt[3][2] *= -1;
					m_cameraTransform.Map() = (projection * lookAt * glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
					m_cameraTransform->Unmap(true);
				}

				// Begin render pass
				const Vector4 CLEAR_VALUE(0.0f, 0.25f, 0.25f, 1.0f);
				data->GetRenderPass()->BeginPass(bufferInfo.commandBuffer, data->GetFrameBuffer(bufferInfo.inFlightBufferId), &CLEAR_VALUE);

				// Draw geometry
				data->Render(bufferInfo);

				// End render pass
				data->GetRenderPass()->EndPass(bufferInfo.commandBuffer);
			}

			VertexBuffer* TriangleRenderer::PositionBuffer() { return &m_positionBuffer; }

			InstanceBuffer* TriangleRenderer::InstanceOffsetBuffer() { return &m_instanceOffsetBuffer; }


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
