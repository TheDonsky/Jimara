#include "UIText.h"
#include "../../Data/Materials/SampleText/SampleTextShader.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIText::Helpers {
			// Instance buffer:
			struct InstanceData {
				Matrix4 transform = Math::Identity();
				Vector4 color = Vector4(1.0f);
			};

			class GraphicsObject 
				: public virtual GraphicsObjectDescriptor
				, public virtual GraphicsObjectDescriptor::ViewportData
				, public virtual JobSystem::Job {
			private:
				const Reference<UIText> m_text;
				const Reference<Jimara::Font> m_font;

				struct InstanceBuffer {
					InstanceData lastInstanceData;
					const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> binding;

					inline InstanceBuffer(Graphics::ArrayBuffer* buffer)
						: binding(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(buffer)) {
						assert(binding->BoundObject() != nullptr);
						assert(binding->BoundObject()->ObjectCount() == 1u);
						reinterpret_cast<InstanceData*>(binding->BoundObject()->Map())[0] = lastInstanceData;
						binding->BoundObject()->Unmap(true);
					}
				} m_instanceData;

				struct {
					float atlasSize = 0.0f;
					Reference<Jimara::Font::Atlas> atlas;
					std::atomic<bool> textureBindingDirty = true;
					const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> textureBinding =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				} m_atlas;

				struct {
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> vertices = 
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> indices =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

					Stacktor<Font::GlyphInfo> symbolUVBuffer;

					std::string text;
					Vector2 size = Vector2(0.0f);
					size_t usedIndexCount = 0u;
				} m_textMesh;

				Material::CachedInstance m_cachedMaterialInstance;

				UITransform::UIPose GetPose() {
					const UITransform* transform = m_text->GetComponentInParents<UITransform>();
					if (transform != nullptr)
						return transform->Pose();
					UITransform::UIPose fullScreen = {};
					{
						fullScreen.center = Vector2(0.0f);
						fullScreen.right = Vector2(1.0f, 0.0f);
						if (m_text->m_canvas != nullptr)
							fullScreen.size = m_text->m_canvas->Size();
					}
					return fullScreen;
				}

				void UpdateText() {
					auto cleanup = [&]() {
						m_atlas.atlas = nullptr;
						m_atlas.atlasSize = 0.0f;
						m_atlas.textureBinding->BoundObject() = nullptr;
						m_atlas.textureBindingDirty = true;

						m_textMesh.vertices->BoundObject() = nullptr;
						m_textMesh.indices->BoundObject() = nullptr;
						m_textMesh.text = "";
						m_textMesh.size = Vector2(0.0f);
						m_textMesh.usedIndexCount = 0u;
					};

					auto fail = [&](const auto&... message) {
						cleanup();
						m_text->Context()->Log()->Error("UIText::Helpers::GraphicsObject::UpdateText - ", message...);
					};

					// Calculate desired font size:
					const float fontSize = [&]() -> float {
						const Vector2 canvasSize = (m_text->m_canvas == nullptr) ? Vector2(0.0f) : m_text->m_canvas->Size();
						const Vector2 canvasResolution = (m_text->m_canvas == nullptr || m_text->m_canvas->TargetRenderStack() == nullptr)
							? Vector2(0.0f) : Vector2(m_text->m_canvas->TargetRenderStack()->Resolution());
						return (canvasSize.y >= std::numeric_limits<float>::epsilon())
							? (m_text->FontSize() * canvasResolution.y / canvasSize.y) : 0.0f;
					}();

					// If we have a size mismatch, update atlas:
					if (m_atlas.atlas == nullptr || m_atlas.atlasSize != fontSize) {
						m_atlas.atlas = m_font->GetAtlas(Math::Max(fontSize, 1.0f), 
							Jimara::Font::AtlasFlags::EXACT_GLYPH_SIZE | Jimara::Font::AtlasFlags::NO_MIPMAPS);
						if (m_atlas.atlas == nullptr) {
							fail("Failed to get atlas! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
						m_atlas.atlasSize = fontSize;
						m_atlas.textureBindingDirty = true;
					}

					// If atlas is not changed and text is the same, no need to do anything more:
					if ((!m_atlas.textureBindingDirty) &&
						m_textMesh.text == m_text->Text() &&
						m_textMesh.vertices->BoundObject() != nullptr &&
						m_textMesh.indices->BoundObject() != nullptr)
						return;

					// Fill symbolUVBuffer:
					{
						const std::wstring text = Convert<std::wstring>(m_text->Text());
						m_font->RequireGlyphs(text); // Should we warn if a glyph is missing?
						m_textMesh.symbolUVBuffer.Clear();
						Font::Reader reader(m_atlas.atlas);
						for (size_t i = 0u; i < text.length(); i++) {
							const std::optional<Font::GlyphInfo> bounds = reader.GetGlyphInfo(text[i]);
							// Should we warn about the missing glyphs?
							if ((!bounds.has_value()) ||
								(bounds.value().shape.advance <= 0.0f &&
									(bounds.value().boundaries.Size().x <= 0.0f || bounds.value().boundaries.Size().y <= 0.0f)))
								continue;
							m_textMesh.symbolUVBuffer.Push(bounds.value());
						}
						m_atlas.textureBinding->BoundObject() = reader.GetTexture();
					}

					// Make sure we have a texture:
					if (m_atlas.textureBinding == nullptr) {
						fail("Failed to get atlas texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}

					// Fill vertex buffer:
					{
						const size_t vertexCount = m_textMesh.symbolUVBuffer.Size() * 4u;
						if (m_textMesh.vertices->BoundObject() == nullptr ||
							m_textMesh.vertices->BoundObject()->ObjectCount() < vertexCount) {
							m_textMesh.vertices->BoundObject() = m_text->Context()->Graphics()->Device()
								->CreateArrayBuffer<MeshVertex>(vertexCount, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
							if (m_textMesh.vertices->BoundObject() == nullptr) {
								fail("Failed to create vertex buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								return;
							}
						}
						
						MeshVertex* vertPtr = reinterpret_cast<MeshVertex*>(m_textMesh.vertices->BoundObject()->Map());
						if (vertPtr == nullptr) {
							m_textMesh.vertices->BoundObject()->Unmap(false);
							fail("Failed to map vertex buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}

						auto addVert = [&](const Vector2& position, const Vector2& uv) {
							vertPtr->position = Vector3(position.x, position.y, 0.0f);
							vertPtr->normal = Vector3(0.0f, 0.0f, -1.0f);
							vertPtr->uv = Vector2(uv.x, uv.y);
							vertPtr++;
						};

						const float fontHeight = m_text->FontSize();
						m_textMesh.size = Vector2(0.0f, fontHeight);
						for (size_t i = 0u; i < m_textMesh.symbolUVBuffer.Size(); i++) {
							const Font::GlyphInfo& glyphInfo = m_textMesh.symbolUVBuffer[i];
							const Rect& uvRect = glyphInfo.boundaries;
							const Vector2 size = fontHeight * glyphInfo.shape.size;
							const Vector2 origin = m_textMesh.size + (fontHeight * glyphInfo.shape.offset);
							addVert(origin, Vector2(uvRect.start.x, uvRect.end.y));
							addVert(origin + Vector2(0.0f, size.y), uvRect.start);
							addVert(origin + size, Vector2(uvRect.end.x, uvRect.start.y));
							addVert(origin + Vector2(size.x, 0.0f), uvRect.end);
							m_textMesh.size.x += fontHeight * glyphInfo.shape.advance;
						}
						m_textMesh.vertices->BoundObject()->Unmap(true);
					}

					// Fill index buffer:
					const size_t indexCount = m_textMesh.symbolUVBuffer.Size() * 6u;
					if (m_textMesh.indices->BoundObject() == nullptr ||
						m_textMesh.indices->BoundObject()->ObjectCount() < indexCount) {
						m_textMesh.indices->BoundObject() = m_text->Context()->Graphics()->Device()
							->CreateArrayBuffer<uint32_t>(indexCount, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
						if (m_textMesh.indices->BoundObject() == nullptr) {
							fail("Failed to create index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}

						uint32_t* indexPtr = reinterpret_cast<uint32_t*>(m_textMesh.indices->BoundObject()->Map());
						if (indexPtr == nullptr) {
							m_textMesh.indices->BoundObject()->Unmap(false);
							m_textMesh.indices->BoundObject() = nullptr;
							fail("Failed to map index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}

						auto addTriangle = [&](uint32_t a, uint32_t b, uint32_t c) {
							indexPtr[0u] = a;
							indexPtr[1u] = b;
							indexPtr[2u] = c;
							indexPtr += 3u;
						};

						for (size_t i = 0u; i < m_textMesh.symbolUVBuffer.Size(); i++) {
							const uint32_t a = static_cast<uint32_t>(i * 4u);
							addTriangle(a, a + 2u, a + 1u);
							addTriangle(a, a + 3u, a + 2u);
						}

						m_textMesh.indices->BoundObject()->Unmap(true);
					}

					// Stuff is set, so we're OK:
					m_textMesh.text = m_text->Text();
					m_textMesh.usedIndexCount = indexCount;
					m_atlas.textureBindingDirty = false;
				}

				void UpdateInstanceData(const UITransform::UIPose& pose) {
					// Calculate new transform and color:
					// __TODO__: Correct position with alignment options...
					Matrix4 transform = Math::Identity();
					transform[3] = Vector4(pose.center, 0.0f, 1.0f);
					const Vector4 color = m_text->Color();

					if (m_instanceData.lastInstanceData.transform == transform && m_instanceData.lastInstanceData.color == color)
						return;

					// Update last instance data:
					{
						m_instanceData.lastInstanceData.transform = transform;
						m_instanceData.lastInstanceData.color = color;
					}

					// Transfer data to GPU:
					Graphics::ArrayBufferReference<InstanceData> stagingBuffer = m_text->Context()->Graphics()->Device()
						->CreateArrayBuffer<InstanceData>(1u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (stagingBuffer == nullptr) {
						return m_text->Context()->Log()->Warning(
							"UIText::Helpers::GraphicsObject::UpdateInstanceData - Failed to create a staging buffer! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						stagingBuffer = m_instanceData.binding->BoundObject();
					}
					{
						stagingBuffer.Map()[0] = m_instanceData.lastInstanceData;
						stagingBuffer->Unmap(true);
					}
					if (stagingBuffer != m_instanceData.binding->BoundObject()) {
						Graphics::CommandBuffer* const commandBuffer = m_text->Context()->Graphics()->GetWorkerThreadCommandBuffer().commandBuffer;
						m_instanceData.binding->BoundObject()->Copy(commandBuffer, stagingBuffer);
					}
				}

				void OnAtlasInvalidate(Jimara::Font*) {
					m_atlas.textureBindingDirty = true;
				}

				inline GraphicsObject(
					UIText* text,
					const Graphics::ArrayBufferReference<InstanceData>& instanceBuffer,
					const Material::Instance* materialInstance)
					: GraphicsObjectDescriptor(0u)
					, GraphicsObjectDescriptor::ViewportData(
						text->Context(), materialInstance->Shader(), Graphics::GraphicsPipeline::IndexType::TRIANGLE)
					, m_text(text)
					, m_font(text->Font())
					, m_instanceData(instanceBuffer)
					, m_cachedMaterialInstance(materialInstance) {
					assert(m_text != nullptr);
					assert(m_font != nullptr);
					// __TODO__: Implement this crap!
					m_font->OnAtlasInvalidated() += Callback(&GraphicsObject::OnAtlasInvalidate, this);
				}

			public:
				inline virtual ~GraphicsObject() {
					m_font->OnAtlasInvalidated() -= Callback(&GraphicsObject::OnAtlasInvalidate, this);
				}

				static Reference<GraphicsObject> Create(UIText* text) {
					if (text == nullptr)
						return nullptr;

					if (text->Font() == nullptr)
						return nullptr;

					auto fail = [&](const auto&... message) {
						text->Context()->Log()->Error("UIText::Helpers::GraphicsObject::Create - ", message...);
						return nullptr;
					};

					// Establish material:
					Reference<const Material::Instance> materialInstance = text->m_materialInstance;
					if ((materialInstance == nullptr || materialInstance->Shader() == nullptr) && text->m_material != nullptr) {
						Material::Reader reader(text->m_material);
						materialInstance = reader.SharedInstance();
					}
					if (materialInstance == nullptr || materialInstance->Shader() == nullptr)
						materialInstance = SampleTextShader::MaterialInstance(text->Context()->Graphics()->Device());
					if (materialInstance == nullptr || materialInstance->Shader() == nullptr)
						return fail("Failed to assign material instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					// Create instance buffer:
					const Graphics::ArrayBufferReference<InstanceData> instanceBuffer = text->Context()->Graphics()->Device()
						->CreateArrayBuffer<InstanceData>(1u, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					if (instanceBuffer == nullptr)
						return fail("Failed to allocate instance buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					const Reference<GraphicsObject> instance = new GraphicsObject(text, instanceBuffer, materialInstance);
					instance->ReleaseRef();
					return instance;
				}

				/** JobSystem::Job: */
				virtual void CollectDependencies(Callback<JobSystem::Job*>)override {}

				virtual void Execute()final override {
					m_cachedMaterialInstance.Update();
					const UITransform::UIPose pose = GetPose();
					UpdateText();
					UpdateInstanceData(pose);
				}


				/** ShaderResourceBindingSet: */
				inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override {
					Graphics::BindingSet::BindingSearchFunctions searchFunctions = m_cachedMaterialInstance.BindingSearchFunctions();
					static Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>>
						(*findFn)(const GraphicsObject*, const Graphics::BindingSet::BindingDescriptor&) =
						[](const GraphicsObject* self, const Graphics::BindingSet::BindingDescriptor& desc)
						-> Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> {
						if (desc.name == FontTextureShaderBindingName()) return self->m_atlas.textureBinding;
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
						vertexInfo.binding = m_textMesh.vertices;
					}
					{
						GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
						instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
						instanceInfo.layout.bufferElementSize = sizeof(InstanceData);
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("localTransform", offsetof(InstanceData, transform)));
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo("vertexColor", offsetof(InstanceData, color)));
						instanceInfo.binding = m_instanceData.binding;
					}
					info.indexBuffer = m_textMesh.indices;
					return info;
				}

				inline virtual size_t IndexCount()const override { return m_textMesh.usedIndexCount; }

				inline virtual size_t InstanceCount()const override { return 1u; }

				inline virtual Reference<Component> GetComponent(size_t, size_t)const override { return m_text; }
			};


			inline static void RefreshGraphicsObject(UIText* self) {
				if (self->m_graphicsObject != nullptr) {
					self->m_canvas->GraphicsObjects()->Remove(self->m_graphicsObject);
					self->Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<GraphicsObject*>(self->m_graphicsObject->Item()));
					self->m_graphicsObject = nullptr;
				}
				self->m_canvas = nullptr;

				if (!self->ActiveInHeirarchy()) return;

				self->m_canvas = self->GetComponentInParents<Canvas>();
				if (self->m_canvas == nullptr || self->m_font == nullptr) return;

				const Reference<GraphicsObjectDescriptor> graphicsObject = GraphicsObject::Create(self);
				if (graphicsObject == nullptr) {
					self->m_canvas = nullptr;
					return;
				}
				self->m_graphicsObject = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(graphicsObject);

				self->Context()->Graphics()->SynchPointJobs().Add(dynamic_cast<JobSystem::Job*>(self->m_graphicsObject->Item()));
				self->m_canvas->GraphicsObjects()->Add(self->m_graphicsObject);
			}

			inline static void UnsubscribeParentChain(UIText* self) {
				for (size_t i = 0; i < self->m_parentChain.Size(); i++)
					self->m_parentChain[i]->OnParentChanged() -= Callback<ParentChangeInfo>(Helpers::OnParentChanged, self);
				self->m_parentChain.Clear();
			}

			inline static void SubscribeParentChain(UIText* self) {
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

			inline static void OnParentChanged(UIText* self, ParentChangeInfo) {
				RefreshGraphicsObject(self);
				SubscribeParentChain(self);
			}

			inline static void OnImageDestroyed(Component* self) {
				Helpers::UnsubscribeParentChain(dynamic_cast<UIText*>(self));
				Helpers::RefreshGraphicsObject(dynamic_cast<UIText*>(self));
				self->OnDestroyed() -= Callback(Helpers::OnImageDestroyed);
			}
		};

		UIText::UIText(Component* parent, const std::string_view& name) 
			: Component(parent, name) {
			Helpers::SubscribeParentChain(this);
			OnDestroyed() += Callback(Helpers::OnImageDestroyed);
		}

		UIText::~UIText() {
			Helpers::OnImageDestroyed(this);
		}

		void UIText::SetFont(Jimara::Font* font) {
			if (m_font == font)
				return;
			m_font = font;
			Helpers::RefreshGraphicsObject(this);
		}

		void UIText::SetFontSize(float size) {
			m_fontSize = size;
		}

		void UIText::SetColor(const Vector4& color) {
			m_color = color;
		}

		void UIText::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_text, "Text", "Displayed text");

				static const std::string fontHint = "Sampler to the main texture (overrides material filed named '" + std::string(FontTextureShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Font, SetFont, "Font", fontHint);
				JIMARA_SERIALIZE_FIELD_GET_SET(FontSize, SetFontSize, "Font Size", "Font size in canvas units");

				static const std::string colorHint = "Image color multiplier (appears as vertex color input with the name: '" + std::string(ColorShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", colorHint, Object::Instantiate<Serialization::ColorAttribute>());
			};
		}

		void UIText::OnComponentEnabled() {
			Helpers::RefreshGraphicsObject(this);
		}

		void UIText::OnComponentDisabled() {
			Helpers::RefreshGraphicsObject(this);
		}
	}

	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIText>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIText>(
			"UI Image", "Jimara/UI/Text", "Text that can appear on UI Canvas");
		report(factory);
	}
}
