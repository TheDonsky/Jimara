#include "UIImage.h"
#include "../../Data/Materials/SampleUI/SampleUIShader.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIImage::Helpers {
#pragma warning(disable: 4250)
			// Vertex and index buffers:
			class SharedVertexBuffer 
				: public virtual Graphics::Legacy::VertexBuffer
				, public virtual ObjectCache<Reference<Object>>::StoredObject {
			public:
				const Graphics::ArrayBufferReference<MeshVertex> vertices;
				const Graphics::ArrayBufferReference<uint32_t> indices;

				inline virtual ~SharedVertexBuffer() {}
				inline static Reference<SharedVertexBuffer> Get(SceneContext* context) { return Cache::Get(context); }

				inline virtual size_t AttributeCount()const override { return 3u; }
				inline virtual AttributeInfo Attribute(size_t index)const override {
					static const AttributeInfo INFOS[] = {
						{ Graphics::SPIRV_Binary::ShaderInputInfo::Type::FLOAT3, 0, offsetof(MeshVertex, position) },
						{ Graphics::SPIRV_Binary::ShaderInputInfo::Type::FLOAT3, 1, offsetof(MeshVertex, normal) },
						{ Graphics::SPIRV_Binary::ShaderInputInfo::Type::FLOAT2, 2, offsetof(MeshVertex, uv) },
					};
					return INFOS[index];
				}
				inline virtual size_t BufferElemSize()const override { return sizeof(MeshVertex); }
				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return vertices; }

			private:
				inline SharedVertexBuffer(Graphics::ArrayBuffer* verts, Graphics::ArrayBuffer* ids) 
					: vertices(verts)
					, indices(ids) {
					// Fill in the vertices:
					assert(vertices != nullptr && vertices->ObjectCount() == 4u);
					{
						MeshVertex* vertexData = vertices.Map();
						vertexData[0].uv = Vector2(0.0f, 0.0f);
						vertexData[1].uv = Vector2(0.0f, 1.0f);
						vertexData[2].uv = Vector2(1.0f, 1.0f);
						vertexData[3].uv = Vector2(1.0f, 0.0f);
						for (size_t i = 0; i < 4u; i++) {
							MeshVertex& vertex = vertexData[i];
							vertex.position = Vector3((vertex.uv - 0.5f) * Vector2(1.0f, -1.0f), 0.0f);
							vertex.normal = Vector3(0.0f, 0.0f, -1.0f);
						}
						vertices->Unmap(true);
					}

					// Fill in the indices:
					assert(indices != nullptr && indices->ObjectCount() == 6u);
					{
						uint32_t* indexData = indices.Map();
						indexData[0] = 0u;
						indexData[1] = 1u;
						indexData[2] = 2u;
						indexData[3] = 0u;
						indexData[4] = 2u;
						indexData[5] = 3u;
						indices->Unmap(true);
					}
				}

				struct Cache : public virtual ObjectCache<Reference<Object>> {
					inline static Reference<SharedVertexBuffer> Get(SceneContext* context) {
						if (context == nullptr) return nullptr;
						static Cache cache;
						return cache.GetCachedOrCreate(context->Graphics()->Device(), false,
							[&]() -> Reference<ObjectCache<Reference<Object>>::StoredObject> {
								auto fail = [&](const auto&... comment) -> Reference<ObjectCache<Reference<Object>>::StoredObject> {
									context->Log()->Error("UIImage::Helpers::SharedVertexBuffer::Cache::Get - ", comment...);
									return nullptr;
								};
								
								const Graphics::ArrayBufferReference<MeshVertex> vertices =
									context->Graphics()->Device()->CreateArrayBuffer<MeshVertex>(4u);
								if (vertices == nullptr)
									return fail("Failed to create vertex buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								
								const Graphics::ArrayBufferReference<uint32_t> indices =
									context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(6u);
								if (indices == nullptr)
									return fail("Failed to create index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

								Reference<ObjectCache<Reference<Object>>::StoredObject> instance = new SharedVertexBuffer(vertices, indices);
								instance->ReleaseRef();
								return instance;
							});
					}
				};
			}; 
#pragma warning(default: 4250)



			// Instance buffer:
			class ImageInstanceBuffer : public virtual Graphics::Legacy::InstanceBuffer {
			private:
				struct InstanceData {
					Matrix4 transform = Math::Identity();
					Vector4 color = Vector4(1.0f);
				};
				InstanceData m_lastInstanceData = {};
				const Reference<Graphics::GraphicsDevice> m_device;
				const Graphics::ArrayBufferReference<InstanceData> m_perInstanceData;

				inline ImageInstanceBuffer(Graphics::GraphicsDevice* graphicsDevice, Graphics::ArrayBuffer* buffer)
					: m_device(graphicsDevice), m_perInstanceData(buffer) {
					assert(m_perInstanceData != nullptr);
					assert(m_perInstanceData->ObjectCount() == 1u);
					m_perInstanceData.Map()[0] = m_lastInstanceData;
					m_perInstanceData->Unmap(true);
				}

			public:
				inline static Reference<ImageInstanceBuffer> Create(SceneContext* context) {
					if (context == nullptr) return nullptr;
					auto fail = [&](const auto&... comment) -> Reference<ImageInstanceBuffer> {
						context->Log()->Error("UIImage::Helpers::InstanceBuffer::Create - ", comment...);
						return nullptr;
					};

					const Graphics::ArrayBufferReference<InstanceData> perInstanceData =
						context->Graphics()->Device()->CreateArrayBuffer<InstanceData>(1u, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (perInstanceData == nullptr)
						return fail("Failed to create a instance buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<ImageInstanceBuffer> instance = new ImageInstanceBuffer(context->Graphics()->Device(), perInstanceData);
					instance->ReleaseRef();
					return instance;
				}

				inline virtual ~ImageInstanceBuffer() {}

				inline void Update(const UIImage* image) {
					const Matrix4 transform = [&]() {
						Matrix4 matrix = Math::Identity();
						const UITransform* transform = image->GetComponentInParents<UITransform>();
						if (transform != nullptr) {
							UITransform::UIPose pose = transform->Pose();
							if (image->KeepAspectRatio() && image->Texture() != nullptr && std::abs(pose.size.x * pose.size.y) > std::numeric_limits<float>::epsilon()) {
								Size2 size = image->Texture()->TargetView()->TargetTexture()->Size();
								if (size.x > 0 && size.y > 0) {
									const float imageAspect = static_cast<float>(size.x) / static_cast<float>(size.y);
									const float poseAspect = pose.size.x / pose.size.y;
									if (imageAspect > poseAspect)
										pose.size.y = pose.size.x / imageAspect;
									else pose.size.x = imageAspect * pose.size.y;
								}
							}
							matrix[0] = Vector4(pose.right, 0.0f, 0.0f) * pose.size.x;
							matrix[1] = Vector4(pose.Up(), 0.0f, 0.0f) * pose.size.y;
							matrix[3] = Vector4(pose.center, 0.0f, 1.0f);
						}
						return matrix;
					}();
					const Vector4 color = image->Color();
					if (m_lastInstanceData.transform == transform && m_lastInstanceData.color == color) return;
					{
						m_lastInstanceData.transform = transform;
						m_lastInstanceData.color = color;
					}
					Graphics::ArrayBufferReference<InstanceData> stagingBuffer =
						m_device->CreateArrayBuffer<InstanceData>(1u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (stagingBuffer == nullptr) {
						return m_device->Log()->Warning(
							"UIImage::Helpers::InstanceBuffer::Update - Failed to create a staging buffer! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						stagingBuffer = m_perInstanceData;
					}
					{
						stagingBuffer.Map()[0] = m_lastInstanceData;
						stagingBuffer->Unmap(true);
					}
					if (stagingBuffer != m_perInstanceData) {
						Graphics::CommandBuffer* const commandBuffer = image->Context()->Graphics()->GetWorkerThreadCommandBuffer().commandBuffer;
						m_perInstanceData->Copy(commandBuffer, stagingBuffer);
					}
				}

				inline virtual size_t AttributeCount()const override { return 2u; }
				inline virtual AttributeInfo Attribute(size_t index)const override {
					static const AttributeInfo INFOS[] = {
						{ Graphics::SPIRV_Binary::ShaderInputInfo::Type::MAT_4X4, 3, offsetof(InstanceData, transform) },
						{ Graphics::SPIRV_Binary::ShaderInputInfo::Type::FLOAT3, 7, offsetof(InstanceData, color) },
					};
					return INFOS[index];
				}
				inline virtual size_t BufferElemSize()const override { return sizeof(InstanceData); }
				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_perInstanceData; }
			};



			// Graphics object:
			class GraphicsObject 
				: public virtual GraphicsObjectDescriptor
				, public virtual GraphicsObjectDescriptor::ViewportData
				, public virtual JobSystem::Job {
			private:
				const Reference<UIImage> m_image;
				const Reference<SharedVertexBuffer> m_vertexBuffer;
				const Reference<ImageInstanceBuffer> m_instanceBuffer;
				const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_fallbackTexturebinding;
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_textureBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				Material::CachedInstance m_cachedMaterialInstance;


				inline GraphicsObject(
					UIImage* image, SharedVertexBuffer* vertexBuffer, ImageInstanceBuffer* instanceBuffer,
					const Graphics::ResourceBinding<Graphics::TextureSampler>* fallbackTexturebinding,
					const Material::Instance* materialInstance)
					: GraphicsObjectDescriptor(0u)
					, GraphicsObjectDescriptor::ViewportData(
						image->Context(), materialInstance->Shader(), Graphics::GraphicsPipeline::IndexType::TRIANGLE,
						Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND)
					, m_image(image), m_vertexBuffer(vertexBuffer), m_instanceBuffer(instanceBuffer)
					, m_fallbackTexturebinding(fallbackTexturebinding)
					, m_cachedMaterialInstance(materialInstance) {
					assert(m_image != nullptr);
					assert(m_vertexBuffer != nullptr);
					assert(m_instanceBuffer != nullptr);
					assert(m_fallbackTexturebinding != nullptr);
					m_textureBinding->BoundObject() = m_fallbackTexturebinding->BoundObject();
				}

			public:
				inline static Reference<GraphicsObject> Create(UIImage* image) {
					if (image == nullptr) return nullptr;

					const Reference<SharedVertexBuffer> vertexBuffer = SharedVertexBuffer::Get(image->Context());
					if (vertexBuffer == nullptr) return nullptr;

					const Reference<ImageInstanceBuffer> instanceBuffer = ImageInstanceBuffer::Create(image->Context());
					if (instanceBuffer == nullptr) return nullptr;

					const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> fallbackTexturebinding =
						Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(1.0f), image->Context()->Graphics()->Device());
					if (fallbackTexturebinding == nullptr) {
						image->Context()->Log()->Error(
							"UIImage::Helpers::GraphicsObject::Create - Failed to get default texture sampler binding! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}

					Reference<const Material::Instance> materialInstance = image->m_materialInstance;
					if ((materialInstance == nullptr || materialInstance->Shader() == nullptr) && image->m_material != nullptr) {
						Material::Reader reader(image->m_material);
						materialInstance = reader.SharedInstance();
					}
					if (materialInstance == nullptr || materialInstance->Shader() == nullptr)
						materialInstance = SampleUIShader::MaterialInstance(image->Context()->Graphics()->Device());
					if (materialInstance == nullptr || materialInstance->Shader() == nullptr) {
						image->Context()->Log()->Error(
							"UIImage::Helpers::GraphicsObject::Create - Failed to assign material instance! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}

					const Reference<GraphicsObject> instance = new GraphicsObject(
						image, vertexBuffer, instanceBuffer, fallbackTexturebinding, materialInstance);
					instance->ReleaseRef();
					return instance;
				}

				inline ~GraphicsObject() {

				}

			protected:
				/** JobSystem::Job: */
				virtual void CollectDependencies(Callback<JobSystem::Job*>)override {}

				virtual void Execute()final override {
					m_cachedMaterialInstance.Update();
					m_instanceBuffer->Update(m_image);
					m_textureBinding->BoundObject() = (m_image->Texture() != nullptr)
						? m_image->Texture() : m_fallbackTexturebinding->BoundObject();
				}


				/** ShaderResourceBindingSet: */

				inline virtual Reference<const Graphics::ResourceBinding<Graphics::Buffer>> FindConstantBufferBinding(const std::string_view& name)const override {
					// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
					for (size_t i = 0; i < m_cachedMaterialInstance.ConstantBufferCount(); i++)
						if (m_cachedMaterialInstance.ConstantBufferName(i) == name) return m_cachedMaterialInstance.ConstantBuffer(i);
					return nullptr;
				}

				inline virtual Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> FindStructuredBufferBinding(const std::string_view& name)const override {
					// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
					for (size_t i = 0; i < m_cachedMaterialInstance.StructuredBufferCount(); i++)
						if (m_cachedMaterialInstance.StructuredBufferName(i) == name) return m_cachedMaterialInstance.StructuredBuffer(i);
					return nullptr;
				}

				inline virtual Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> FindTextureSamplerBinding(const std::string_view& name)const override {
					if (name == TextureShaderBindingName()) return m_textureBinding;
					// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
					for (size_t i = 0; i < m_cachedMaterialInstance.TextureSamplerCount(); i++)
						if (m_cachedMaterialInstance.TextureSamplerName(i) == name) return m_cachedMaterialInstance.TextureSampler(i);
					return nullptr;
				}

				inline virtual Reference<const Graphics::ResourceBinding<Graphics::TextureView>> FindTextureViewBinding(const std::string_view&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> FindBindlessStructuredBufferSetBinding(const std::string_view&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> FindBindlessTextureSamplerSetBinding(const std::string_view&)const override { return nullptr; }
				inline virtual Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureView>::Instance>> FindBindlessTextureViewSetBinding(const std::string_view&)const override { return nullptr; }



				/** GraphicsObjectDescriptor */

				inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const ViewportDescriptor*) override { return this; }

				inline virtual AABB Bounds()const override {
					// __TODO__: Implement this crap!
					return AABB();
				}

				inline virtual size_t VertexBufferCount()const override { return 1; }

				inline virtual Reference<Graphics::Legacy::VertexBuffer> VertexBuffer(size_t index)const override { return m_vertexBuffer; }

				inline virtual size_t InstanceBufferCount()const override { return 1; }

				inline virtual Reference<Graphics::Legacy::InstanceBuffer> InstanceBuffer(size_t index)const override { return m_instanceBuffer; }

				inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_vertexBuffer->indices; }

				inline virtual size_t IndexCount()const override { return m_vertexBuffer->indices->ObjectCount(); }

				inline virtual size_t InstanceCount()const override { return 1u; }

				inline virtual Reference<Component> GetComponent(size_t, size_t)const override { return m_image; }
			};



			// Graphics object recreation events:
			inline static void RefreshGraphicsObject(UIImage* self) {
				if (self->m_graphicsObject != nullptr) {
					self->m_canvas->GraphicsObjects()->Remove(self->m_graphicsObject);
					self->Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<GraphicsObject*>(self->m_graphicsObject->Item()));
					self->m_graphicsObject = nullptr;
				}
				self->m_canvas = nullptr;

				if (!self->ActiveInHeirarchy()) return;

				self->m_canvas = self->GetComponentInParents<Canvas>();
				if (self->m_canvas == nullptr) return;

				const Reference<GraphicsObject> graphicsObject = GraphicsObject::Create(self);
				if (graphicsObject == nullptr) {
					self->m_canvas = nullptr;
					return;
				}
				self->m_graphicsObject = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(graphicsObject);

				self->Context()->Graphics()->SynchPointJobs().Add(dynamic_cast<GraphicsObject*>(self->m_graphicsObject->Item()));
				self->m_canvas->GraphicsObjects()->Add(self->m_graphicsObject);
			}

			inline static void UnsubscribeParentChain(UIImage* self) {
				for (size_t i = 0; i < self->m_parentChain.Size(); i++)
					self->m_parentChain[i]->OnParentChanged() -= Callback<ParentChangeInfo>(Helpers::OnParentChanged, self);
				self->m_parentChain.Clear();
			}

			inline static void SubscribeParentChain(UIImage* self) {
				UnsubscribeParentChain(self);
				if (self->Destroyed()) return;
				Component* parent = self;
				while (parent != nullptr) {
					parent->OnParentChanged() += Callback<ParentChangeInfo>(Helpers::OnParentChanged, self);
					self->m_parentChain.Push(parent);
					if (dynamic_cast<Canvas*>(parent) != nullptr) break;
					else parent = parent->Parent();
				}
			}

			inline static void OnParentChanged(UIImage* self, ParentChangeInfo) {
				RefreshGraphicsObject(self);
				SubscribeParentChain(self);
			}

			inline static void OnImageDestroyed(Component* self) {
				Helpers::UnsubscribeParentChain(dynamic_cast<UIImage*>(self));
				Helpers::RefreshGraphicsObject(dynamic_cast<UIImage*>(self));
				self->OnDestroyed() -= Callback(Helpers::OnImageDestroyed);
			}
		};


		UIImage::UIImage(Component* parent, const std::string_view& name) 
			: Component(parent, name) {
			Helpers::SubscribeParentChain(this);
			OnDestroyed() += Callback(Helpers::OnImageDestroyed);
		}

		UIImage::~UIImage() {
			Helpers::OnImageDestroyed(this);
		}

		void UIImage::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				static const std::string textureHint = "Sampler to the main texture (overrides material filed named '" + std::string(TextureShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", textureHint);

				static const std::string colorHint = "Image color multiplier (appears as vertex color input with the name: '" + std::string(ColorShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", colorHint, Object::Instantiate<Serialization::ColorAttribute>());
				
				JIMARA_SERIALIZE_FIELD_GET_SET(KeepAspectRatio, KeepAspectRatio, "Keep Aspect", "If true, the UIImage will keep the aspect ratio of the Texture");
			};
		}

		void UIImage::OnComponentEnabled() {
			Helpers::RefreshGraphicsObject(this);
		}

		void UIImage::OnComponentDisabled() {
			Helpers::RefreshGraphicsObject(this);
		}
	}


	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIImage>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<UI::UIImage> serializer("Jimara/UI/Image", "Image");
		report(&serializer);
	}
}
