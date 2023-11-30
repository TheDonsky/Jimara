#include "UIImage.h"
#include "../../Data/Materials/SampleUI/SampleUIShader.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIImage::Helpers {
#pragma warning(disable: 4250)
			// Vertex and index buffers:
			class SharedVertexBuffer : public virtual ObjectCache<Reference<Object>>::StoredObject {
			public:
				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> vertices;
				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> indices;
				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> flippedIndices;

				inline virtual ~SharedVertexBuffer() {}
				inline static Reference<SharedVertexBuffer> Get(SceneContext* context) { return Cache::Get(context); }

			private:
				inline SharedVertexBuffer(Graphics::ArrayBuffer* verts, Graphics::ArrayBuffer* ids, Graphics::ArrayBuffer* invIds)
					: vertices(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(verts))
					, indices(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(ids))
					, flippedIndices(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(invIds)) {
					// Fill in the vertices:
					assert(vertices->BoundObject() != nullptr && vertices->BoundObject()->ObjectCount() == 4u);
					{
						MeshVertex* vertexData = reinterpret_cast<MeshVertex*>(vertices->BoundObject()->Map());
						vertexData[0].uv = Vector2(0.0f, 0.0f);
						vertexData[1].uv = Vector2(0.0f, 1.0f);
						vertexData[2].uv = Vector2(1.0f, 1.0f);
						vertexData[3].uv = Vector2(1.0f, 0.0f);
						for (size_t i = 0; i < 4u; i++) {
							MeshVertex& vertex = vertexData[i];
							vertex.position = Vector3((vertex.uv - 0.5f) * Vector2(1.0f, -1.0f), 0.0f);
							vertex.normal = Vector3(0.0f, 0.0f, -1.0f);
						}
						vertices->BoundObject()->Unmap(true);
					}

					// Fill in the indices:
					assert(indices->BoundObject() != nullptr && indices->BoundObject()->ObjectCount() == 6u);
					{
						uint32_t* indexData = reinterpret_cast<uint32_t*>(indices->BoundObject()->Map());
						indexData[0] = 0u;
						indexData[1] = 1u;
						indexData[2] = 2u;
						indexData[3] = 0u;
						indexData[4] = 2u;
						indexData[5] = 3u;
						indices->BoundObject()->Unmap(true);
					}

					// Fill in the flipped indices:
					assert(flippedIndices->BoundObject() != nullptr && flippedIndices->BoundObject()->ObjectCount() == 6u);
					{
						uint32_t* indexData = reinterpret_cast<uint32_t*>(flippedIndices->BoundObject()->Map());
						indexData[0] = 0u;
						indexData[1] = 2u;
						indexData[2] = 1u;
						indexData[3] = 0u;
						indexData[4] = 3u;
						indexData[5] = 2u;
						flippedIndices->BoundObject()->Unmap(true);
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

								const Graphics::ArrayBufferReference<uint32_t> flippedIndices =
									context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(6u);
								if (flippedIndices == nullptr)
									return fail("Failed to create flipped index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

								Reference<ObjectCache<Reference<Object>>::StoredObject> instance = new SharedVertexBuffer(vertices, indices, flippedIndices);
								instance->ReleaseRef();
								return instance;
							});
					}
				};
			}; 
#pragma warning(default: 4250)



			// Instance buffer:
			struct InstanceData {
				Matrix4 transform = Math::Identity();
				Vector4 color = Vector4(1.0f);
			};
			class ImageInstanceBuffer : public virtual Object {
			public:
				const Reference<Graphics::GraphicsDevice> device;
				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> instanceData;

			private:
				InstanceData m_lastInstanceData = {};

				inline ImageInstanceBuffer(Graphics::GraphicsDevice* graphicsDevice, Graphics::ArrayBuffer* buffer)
					: device(graphicsDevice)
					, instanceData(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(buffer)) {
					assert(instanceData->BoundObject() != nullptr);
					assert(instanceData->BoundObject()->ObjectCount() == 1u);
					reinterpret_cast<InstanceData*>(instanceData->BoundObject()->Map())[0] = m_lastInstanceData;
					instanceData->BoundObject()->Unmap(true);
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

				inline void Update(const UIImage* image, const UITransform::UIPose& pose) {
					const Matrix4 transform = [&]() {
						Matrix4 matrix = Math::Identity();
						matrix[0] = Vector4(pose.right, 0.0f, 0.0f) * pose.size.x;
						matrix[1] = Vector4(pose.up, 0.0f, 0.0f) * pose.size.y;
						matrix[3] = Vector4(pose.center, 0.0f, 1.0f);
						return matrix;
					}();
					const Vector4 color = image->Color();
					if (m_lastInstanceData.transform == transform && m_lastInstanceData.color == color) return;
					{
						m_lastInstanceData.transform = transform;
						m_lastInstanceData.color = color;
					}
					Graphics::ArrayBufferReference<InstanceData> stagingBuffer =
						device->CreateArrayBuffer<InstanceData>(1u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (stagingBuffer == nullptr) {
						return device->Log()->Warning(
							"UIImage::Helpers::InstanceBuffer::Update - Failed to create a staging buffer! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						stagingBuffer = instanceData->BoundObject();
					}
					{
						stagingBuffer.Map()[0] = m_lastInstanceData;
						stagingBuffer->Unmap(true);
					}
					if (stagingBuffer != instanceData->BoundObject()) {
						Graphics::CommandBuffer* const commandBuffer = image->Context()->Graphics()->GetWorkerThreadCommandBuffer().commandBuffer;
						instanceData->BoundObject()->Copy(commandBuffer, stagingBuffer);
					}
				}
			};



			// Graphics object:
			class GraphicsObject 
				: public virtual GraphicsObjectDescriptor
				, public virtual GraphicsObjectDescriptor::ViewportData
				, public virtual JobSystem::Job {
			private:
				const Reference<UIImage> m_image;
				const Reference<SharedVertexBuffer> m_vertexBuffer;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_indexBuffer =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Reference<ImageInstanceBuffer> m_instanceBuffer;
				const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_fallbackTexturebinding;
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_textureBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				Material::CachedInstance m_cachedMaterialInstance;


				inline UITransform::UIPose GetPose() {
					UITransform::UIPose pose = {};
					const UITransform* transform = m_image->GetComponentInParents<UITransform>(); 
					if (transform != nullptr)
						pose = transform->Pose();
					else if (m_image->m_canvas != nullptr)
						pose.size = m_image->m_canvas->Size();
					if (m_image->KeepAspectRatio() && m_image->Texture() != nullptr && 
						std::abs(pose.size.x * pose.size.y) > std::numeric_limits<float>::epsilon()) {
						Size2 size = m_image->Texture()->TargetView()->TargetTexture()->Size();
						if (size.x > 0 && size.y > 0) {
							const float imageAspect = static_cast<float>(size.x) / static_cast<float>(size.y);
							const float poseAspect = pose.size.x / pose.size.y;
							if (imageAspect > poseAspect)
								pose.size.y = pose.size.x / imageAspect;
							else pose.size.x = imageAspect * pose.size.y;
						}
					}
					return pose;
				}

				inline GraphicsObject(
					UIImage* image, SharedVertexBuffer* vertexBuffer, ImageInstanceBuffer* instanceBuffer,
					const Graphics::ResourceBinding<Graphics::TextureSampler>* fallbackTexturebinding,
					const Material::Instance* materialInstance)
					: GraphicsObjectDescriptor(0u)
					, GraphicsObjectDescriptor::ViewportData(
						image->Context(), materialInstance->Shader(), Graphics::GraphicsPipeline::IndexType::TRIANGLE)
					, m_image(image), m_vertexBuffer(vertexBuffer), m_instanceBuffer(instanceBuffer)
					, m_fallbackTexturebinding(fallbackTexturebinding)
					, m_cachedMaterialInstance(materialInstance) {
					assert(m_image != nullptr);
					assert(m_vertexBuffer != nullptr);
					assert(m_instanceBuffer != nullptr);
					assert(m_fallbackTexturebinding != nullptr);
					m_indexBuffer->BoundObject() = m_vertexBuffer->indices->BoundObject();
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
					const UITransform::UIPose pose = GetPose();
					m_instanceBuffer->Update(m_image, pose);
					m_indexBuffer->BoundObject() = (Math::Cross(Vector3(pose.right, 0.0f), Vector3(pose.up, 0.0f)).z >= 0.0f)
						? m_vertexBuffer->indices->BoundObject() : m_vertexBuffer->flippedIndices->BoundObject();
					m_textureBinding->BoundObject() = (m_image->Texture() != nullptr)
						? m_image->Texture() : m_fallbackTexturebinding->BoundObject();
				}


				/** ShaderResourceBindingSet: */
				inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override { 
					Graphics::BindingSet::BindingSearchFunctions searchFunctions = m_cachedMaterialInstance.BindingSearchFunctions();
					static Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>>
						(*findFn)(const GraphicsObject*, const Graphics::BindingSet::BindingDescriptor&) =
						[](const GraphicsObject* self, const Graphics::BindingSet::BindingDescriptor& desc)
						-> Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> {
						if (desc.name == TextureShaderBindingName()) return self->m_textureBinding;
						else return self->m_cachedMaterialInstance.FindTextureSampler(desc.name);
					};
					searchFunctions.textureSampler = Graphics::BindingSet::BindingSearchFn<Graphics::TextureSampler>(findFn, this);
					return searchFunctions;
				}


				/** GraphicsObjectDescriptor */

				inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const ViewportDescriptor*) override { return this; }

				inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const override {
					GraphicsObjectDescriptor::VertexInputInfo info = {};
					info.vertexBuffers.Resize(2u);
					{
						GraphicsObjectDescriptor::VertexBufferInfo& vertexInfo = info.vertexBuffers[0u];
						vertexInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
						vertexInfo.layout.bufferElementSize = sizeof(MeshVertex);
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertPosition", offsetof(MeshVertex, position)));
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertNormal", offsetof(MeshVertex, normal)));
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertUV", offsetof(MeshVertex, uv)));
						vertexInfo.binding = m_vertexBuffer->vertices;
					}
					{
						GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
						instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
						instanceInfo.layout.bufferElementSize = sizeof(InstanceData);
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("localTransform", offsetof(InstanceData, transform)));
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertexColor", offsetof(InstanceData, color)));
						instanceInfo.binding = m_instanceBuffer->instanceData;
					}
					info.indexBuffer = m_indexBuffer;
					return info;
				}

				inline virtual size_t IndexCount()const override { return m_indexBuffer->BoundObject()->ObjectCount(); }

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
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIImage>(
			"UI Image", "Jimara/UI/Image", "Image that can appear on UI Canvas");
		report(factory);
	}
}
