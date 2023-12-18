#include "UIText.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Data/Materials/SampleText/SampleTextShader.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Data/Serialization/Attributes/TextBoxAttribute.h"


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

				class AtlasRef {
				private:
					Reference<Jimara::Font::Atlas> m_atlas;

				public:
					void Set(Jimara::Font::Atlas* atlas, GraphicsObject* owner) {
						assert(owner != nullptr);
						if (m_atlas == atlas)
							return;
						if (m_atlas != nullptr)
							m_atlas->OnAtlasInvalidated() -= Callback(&GraphicsObject::OnAtlasInvalidate, owner);
						m_atlas = atlas;
						if (m_atlas != nullptr)
							m_atlas->OnAtlasInvalidated() += Callback(&GraphicsObject::OnAtlasInvalidate, owner);
					}

					~AtlasRef() {
						assert(m_atlas == nullptr);
					}

					inline Jimara::Font::Atlas* operator->()const { return m_atlas; }
				};

				struct {
					float atlasSize = 0.0f;
					AtlasRef atlas;
					std::atomic<bool> textureBindingDirty = true;
					const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> textureBinding =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				} m_atlas;

				struct {
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> vertices = 
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
					const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> indices =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

					std::vector<Font::GlyphInfo> symbolUVBuffer;

					std::string text;
					Vector2 lastRectSize = Vector2(0.0f);
					Vector2 lastScale = Vector2(0.0f);
					bool lastWasFlipped = false;
					float lastHorAlignment = 0.0f;
					WrappingMode lastWrappingMode = WrappingMode::NONE;
					Vector2 size = Vector2(0.0f);
					size_t usedIndexCount = 0u;
				} m_textMesh;

				Material::CachedInstance m_cachedMaterialInstance;

				UITransform::UIPose GetPose() {
					const UITransform* transform = m_text->GetComponentInParents<UITransform>();
					UITransform::UIPose pose;
					if (transform != nullptr) {
						pose = transform->Pose();
						Vector2 sizeSign(
							pose.size.x >= 0.0f ? 1.0f : -1.0f,
							pose.size.y >= 0.0f ? 1.0f : -1.0f);
						pose.size *= sizeSign;
						pose.right *= sizeSign.x;
						pose.up *= sizeSign.y;
					}
					else {
						pose = {};
						pose.center = Vector2(0.0f);
						pose.right = Vector2(1.0f, 0.0f);
						if (m_text->m_canvas != nullptr)
							pose.size = m_text->m_canvas->Size();
						assert(pose.size.x >= 0.0f && pose.size.y >= 0.0f);
					}
					return pose;
				}

				void UpdateText(const UITransform::UIPose& pose) {
					auto cleanup = [&]() {
						m_atlas.atlas.Set(nullptr, this);
						m_atlas.atlasSize = 0.0f;
						m_atlas.textureBinding->BoundObject() = nullptr;
						m_atlas.textureBindingDirty = true;

						m_textMesh.vertices->BoundObject() = nullptr;
						m_textMesh.indices->BoundObject() = nullptr;
						m_textMesh.text = "";
						m_textMesh.lastRectSize = Vector2(0.0f);
						m_textMesh.lastScale = Vector2(0.0f);
						m_textMesh.lastWasFlipped = false;
						m_textMesh.lastHorAlignment = 0.0f;
						m_textMesh.lastWrappingMode = WrappingMode::NONE;
						m_textMesh.size = Vector2(0.0f);
						m_textMesh.usedIndexCount = 0u;
					};

					auto fail = [&](const auto&... message) {
						cleanup();
						m_text->Context()->Log()->Error("UIText::Helpers::GraphicsObject::UpdateText - ", message...);
					};
					const Vector2 poseScale = pose.Scale();
					const Vector2 poseSize = pose.size;
					const float fontSize = (m_text->FontSizeMode() == SizeMode::RECT_FRACTION)
						? (m_text->FontSize() * Math::Lerp(poseSize.y, poseSize.x, m_text->RectSizeBias()))
						: m_text->FontSize();
					assert(pose.size.x >= 0.0f && pose.size.y >= 0.0f);
					const bool isFlipped = Math::Cross(Vector3(pose.right, 0.0f), Vector3(pose.up, 0.0f)).z < 0.0f;

					// Calculate desired font size:
					const float desiredFontSize = [&]() -> float {
						const Vector2 canvasSize = (m_text->m_canvas == nullptr) ? Vector2(0.0f) : m_text->m_canvas->Size();
						const Vector2 canvasResolution = (m_text->m_canvas == nullptr || m_text->m_canvas->TargetRenderStack() == nullptr)
							? Vector2(0.0f) : Vector2(m_text->m_canvas->TargetRenderStack()->Resolution());
						const float baseSize = (canvasSize.y >= std::numeric_limits<float>::epsilon())
							? (fontSize * canvasResolution.y / canvasSize.y) : 0.0f;
						return baseSize * Math::Max(poseScale.x, poseScale.y);
					}();

					// If we have a size mismatch, update atlas:
					if (m_atlas.atlas.operator->() == nullptr || m_atlas.atlasSize != desiredFontSize) {
						Reference<Font::Atlas> atlas = m_font->GetAtlas(Math::Max(desiredFontSize, 1.0f),
							Jimara::Font::AtlasFlags::EXACT_GLYPH_SIZE | Jimara::Font::AtlasFlags::NO_MIPMAPS);
						m_atlas.atlas.Set(atlas, this);
						if (m_atlas.atlas.operator->() == nullptr) {
							fail("Failed to get atlas! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
						m_atlas.atlasSize = desiredFontSize;
						m_atlas.textureBindingDirty = true;
					}

					// If atlas is not changed and text is the same, no need to do anything more:
					if ((!m_atlas.textureBindingDirty) &&
						m_textMesh.lastRectSize == poseSize &&
						m_textMesh.lastScale == poseScale &&
						m_textMesh.lastWasFlipped == isFlipped &&
						m_textMesh.lastHorAlignment == m_text->HorizontalAlignment() &&
						m_textMesh.lastWrappingMode == m_text->LineWrapping() &&
						m_textMesh.text == m_text->Text() &&
						m_textMesh.vertices->BoundObject() != nullptr &&
						m_textMesh.indices->BoundObject() != nullptr)
						return;

					// Fill symbolUVBuffer:
					{
						const std::wstring text = Convert<std::wstring>(m_text->Text());
						m_atlas.atlas->RequireGlyphs(text); // Should we warn if a glyph is missing?
						m_textMesh.symbolUVBuffer.clear();
						Font::Reader reader(m_atlas.atlas.operator->());
						for (size_t i = 0u; i < text.length(); i++) {
							const std::optional<Font::GlyphInfo> bounds = reader.GetGlyphInfo(text[i]);
							// Should we warn about the missing glyphs?
							if ((!bounds.has_value()) ||
								(bounds.value().shape.advance <= 0.0f &&
									(bounds.value().boundaries.Size().x <= 0.0f || bounds.value().boundaries.Size().y <= 0.0f)))
								continue;
							m_textMesh.symbolUVBuffer.push_back(bounds.value());
						}
						m_atlas.textureBinding->BoundObject() = reader.GetTexture();
					}

					// Make sure we have a texture:
					if (m_atlas.textureBinding == nullptr) {
						fail("Failed to get atlas texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}

					// Fill vertex buffer:
					size_t drawnCharacterCount = 0u;
					{
						const size_t vertexCount = m_textMesh.symbolUVBuffer.size() * 4u;
						if (m_textMesh.vertices->BoundObject() == nullptr ||
							m_textMesh.vertices->BoundObject()->ObjectCount() < vertexCount) {
							m_textMesh.vertices->BoundObject() = m_text->Context()->Graphics()->Device()
								->CreateArrayBuffer<MeshVertex>(vertexCount, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
							if (m_textMesh.vertices->BoundObject() == nullptr) {
								fail("Failed to create vertex buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								return;
							}
						}
						
						MeshVertex* const vertices = reinterpret_cast<MeshVertex*>(m_textMesh.vertices->BoundObject()->Map());
						MeshVertex* vertPtr = vertices;
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

						const Font::LineSpacing spacing = m_atlas.atlas->Spacing();
						const float characterScale = fontSize;
						Vector2 cursor = Vector2(0.0f, -Math::Max(spacing.ascender, 1.0f) * characterScale);
						const float yDelta = spacing.lineHeight * characterScale;
						m_textMesh.size = Vector2(0.0f, spacing.descender * characterScale - cursor.y);
						
						bool lastWasWhiteSpace = true;
						MeshVertex* wordStartPtr = vertices;
						float wordWidth = 0.0f;
						float lastNonWsXBeforeWordStart = 0.0f;
						MeshVertex* lastWordEndPtr = vertPtr;
						float wordStartX = 0.0f;
						
						// Line alignment/centering:
						MeshVertex* lineStart = vertices;
						float lastNonWsX = 0.0f;
						auto alignLine = [&](MeshVertex* lineEnd, float lineWidth) {
							m_textMesh.size.x = Math::Max(m_textMesh.size.x, lineWidth);
							if (lineWidth) {
								const float xDelta = lineWidth * m_text->HorizontalAlignment();
								for (MeshVertex* ptr = lineStart; ptr < lineEnd; ptr++)
									ptr->position.x -= xDelta;
							}
							lineStart = lineEnd;
							wordStartPtr = vertPtr;
							lastWordEndPtr = vertPtr;
						};

						for (size_t i = 0u; i < m_textMesh.symbolUVBuffer.size(); i++) {
							const Font::GlyphInfo& glyphInfo = m_textMesh.symbolUVBuffer[i];

							// If we have an end of line, move to next:
							if ((static_cast<char>(glyphInfo.glyph) == glyphInfo.glyph) &&
								(static_cast<char>(glyphInfo.glyph) == '\n')) {
								alignLine(vertPtr, cursor.x);
								cursor.x = 0.0f;
								cursor.y -= yDelta;
								m_textMesh.size.y += yDelta;
								wordWidth = 0.0f;
								lastWasWhiteSpace = true;
								lastNonWsXBeforeWordStart = 0.0f;
								lastNonWsX = 0.0f;
								continue;
							}

							// Check if a new word started:
							static const auto isWS = [](const Font::Glyph& glyph) {
								return (static_cast<char>(glyph) == glyph) && std::isspace(static_cast<char>(glyph));
							};
							const bool isWhiteSpace = isWS(glyphInfo.glyph);
							if (lastWasWhiteSpace && (!isWhiteSpace)) {
								wordStartPtr = vertPtr;
								wordStartX = cursor.x;
								wordWidth = 0.0f;
								for (size_t j = i; j < m_textMesh.symbolUVBuffer.size(); j++) {
									const Font::GlyphInfo& symbol = m_textMesh.symbolUVBuffer[j];
									if (isWS(symbol.glyph))
										break;
									else wordWidth += characterScale * symbol.shape.advance;
								}
							}
							else if ((!lastWasWhiteSpace) && isWhiteSpace) {
								lastNonWsXBeforeWordStart = cursor.x;
								lastWordEndPtr = vertPtr;
							}
							lastWasWhiteSpace = isWhiteSpace;

							const float advance = characterScale * glyphInfo.shape.advance;

							// If character does not fit on the line, we need to wrap around to a new line:
							auto hasWrappingFlag = [&](WrappingMode flag) {
								using UnderlyingType = std::underlying_type_t<WrappingMode>;
								return static_cast<WrappingMode>(
									static_cast<UnderlyingType>(m_text->LineWrapping()) &
									static_cast<UnderlyingType>(flag)) == flag;
							};
							const bool wrapWords = hasWrappingFlag(WrappingMode::WORD);
							if (cursor.x >= std::numeric_limits<float>::epsilon() &&
								(cursor.x + advance) >= poseSize.x && 
								hasWrappingFlag(WrappingMode::CHARACTER)) {
								cursor.y -= yDelta;
								m_textMesh.size.y += yDelta;

								// Remove white spaces:
								auto removeWhiteSpaceChars = [&]() {
									const MeshVertex* wsEndPtr = isWhiteSpace ? vertPtr : wordStartPtr;
									const size_t numWsVerts = (wsEndPtr - lastWordEndPtr);
									const size_t shiftSize = (vertPtr - wsEndPtr);
									for (size_t vi = 0u; vi < shiftSize; vi++)
										lastWordEndPtr[vi] = wsEndPtr[vi];
									vertPtr -= numWsVerts;
									wordStartPtr = lastWordEndPtr;
									drawnCharacterCount -= numWsVerts / 4u;
								};

								if ((!isWhiteSpace) && wordWidth < poseSize.x && wrapWords) {
									// Move word to next line:
									removeWhiteSpaceChars();
									const Vector3 delta = Vector3(-wordStartX, -yDelta, 0.0f);
									for (MeshVertex* ptr = wordStartPtr; ptr < vertPtr; ptr++)
										ptr->position += delta;
									alignLine(wordStartPtr, lastNonWsXBeforeWordStart);
									cursor.x = cursor.x - wordStartX;
									lastNonWsXBeforeWordStart = 0.0f;
									lastNonWsX = wordWidth;
									i--;
								}
								else {
									// Move current character on the next line:
									if ((isWhiteSpace || (wordStartPtr == vertPtr)) && wrapWords)
										removeWhiteSpaceChars();
									alignLine(vertPtr, wrapWords ? lastNonWsX : cursor.x);
									cursor.x = 0.0f;
									lastNonWsX = 0.0f;
									if (!(isWhiteSpace && wrapWords))
										i--;
								}
								continue;
							}

							// Calculate basic shape of the character:
							const Rect& uvRect = glyphInfo.boundaries;
							const Vector2 start = Vector2(cursor.x, cursor.y) + (characterScale * glyphInfo.shape.offset);
							const Vector2 end = start + characterScale * glyphInfo.shape.size;

							// Draw character:
							addVert(start, Vector2(uvRect.start.x, uvRect.end.y));
							addVert(Vector2(start.x, end.y), uvRect.start);
							addVert(end, Vector2(uvRect.end.x, uvRect.start.y));
							addVert(Vector2(end.x, start.y), uvRect.end);
							drawnCharacterCount++;
							cursor.x += advance;
							if (!isWhiteSpace)
								lastNonWsX = cursor.x;
							m_textMesh.size.y = Math::Max(m_textMesh.size.y, -start.y);
						}
						alignLine(vertPtr, cursor.x);

						// Recenter lines:
						if (vertices < vertPtr) {
							const float xDelta = m_textMesh.size.x * m_text->HorizontalAlignment();
							for (MeshVertex* ptr = vertices; ptr < vertPtr; ptr++)
								ptr->position.x += xDelta;
						}

						m_textMesh.vertices->BoundObject()->Unmap(true);
					}

					// Fill index buffer:
					const size_t indexCount = drawnCharacterCount * 6u;
					if (m_textMesh.lastWasFlipped != isFlipped ||
						m_textMesh.indices->BoundObject() == nullptr ||
						m_textMesh.indices->BoundObject()->ObjectCount() < indexCount) {

						if (m_textMesh.indices->BoundObject() == nullptr ||
							m_textMesh.indices->BoundObject()->ObjectCount() < indexCount) {
							m_textMesh.indices->BoundObject() = m_text->Context()->Graphics()->Device()
								->CreateArrayBuffer<uint32_t>(indexCount, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
							if (m_textMesh.indices->BoundObject() == nullptr) {
								fail("Failed to create index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								return;
							}
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

						const size_t symbolSlotCount = m_textMesh.indices->BoundObject()->ObjectCount() / 6u;
						if (!isFlipped) {
							for (size_t i = 0u; i < symbolSlotCount; i++) {
								const uint32_t a = static_cast<uint32_t>(i * 4u);
								addTriangle(a, a + 2u, a + 1u);
								addTriangle(a, a + 3u, a + 2u);
							}
						}
						else for (size_t i = 0u; i < symbolSlotCount; i++) {
							const uint32_t a = static_cast<uint32_t>(i * 4u);
							addTriangle(a, a + 1u, a + 2u);
							addTriangle(a, a + 2u, a + 3u);
						}

						m_textMesh.indices->BoundObject()->Unmap(true);
					}

					// Stuff is set, so we're OK:
					m_textMesh.text = m_text->Text();
					m_textMesh.lastRectSize = poseSize;
					m_textMesh.lastScale = poseScale;
					m_textMesh.lastWasFlipped = isFlipped;
					m_textMesh.lastHorAlignment = m_text->HorizontalAlignment();
					m_textMesh.lastWrappingMode = m_text->LineWrapping();
					m_textMesh.usedIndexCount = indexCount;
					m_atlas.textureBindingDirty = false;
				}

				void UpdateInstanceData(const UITransform::UIPose& pose) {
					// Calculate new transform and color:
					Matrix4 transform = Math::Identity();
					transform[0] = Vector4(pose.right, 0.0f, 0.0f);
					transform[1] = Vector4(pose.up, 0.0f, 0.0f);
					transform[3] = Vector4(pose.center +
						pose.right * (-pose.size.x * 0.5f + (pose.size.x - m_textMesh.size.x) * m_text->HorizontalAlignment()) +
						pose.up * (pose.size.y * 0.5f + (m_textMesh.size.y - pose.size.y) * m_text->VerticalAlignment()), 0.0f, 1.0f);
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

				void OnAtlasInvalidate(Jimara::Font::Atlas*) {
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
				}

			public:
				inline virtual ~GraphicsObject() {
					m_atlas.atlas.Set(nullptr, this);
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
					UpdateText(pose);
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

				inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const RendererFrustrumDescriptor*) override { return this; }

				inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const override {
					GraphicsObjectDescriptor::VertexInputInfo info = {};
					info.vertexBuffers.Resize(2u);
					{
						GraphicsObjectDescriptor::VertexBufferInfo& vertexInfo = info.vertexBuffers[0u];
						vertexInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
						vertexInfo.layout.bufferElementSize = sizeof(MeshVertex);
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_VertexPosition_Location, offsetof(MeshVertex, position)));
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_VertexNormal_Location, offsetof(MeshVertex, normal)));
						vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_VertexUV_Location, offsetof(MeshVertex, uv)));
						vertexInfo.binding = m_textMesh.vertices;
					}
					{
						GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
						instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
						instanceInfo.layout.bufferElementSize = sizeof(InstanceData);
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(InstanceData, transform)));
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_VertexColor_Location, offsetof(InstanceData, color)));
						instanceInfo.binding = m_instanceData.binding;
					}
					info.indexBuffer = m_textMesh.indices;
					return info;
				}

				inline virtual size_t IndexCount()const override { return m_textMesh.usedIndexCount; }

				inline virtual size_t InstanceCount()const override { return 1u; }

				inline virtual Reference<Component> GetComponent(size_t)const override { return m_text; }
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

		const Object* UIText::WrappingModeEnumAttribute() {
			static const Reference<const Object> attribute =
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<WrappingMode>>>(false,
					"NONE", WrappingMode::NONE,
					"CHARACTER", WrappingMode::CHARACTER,
					"WORD", WrappingMode::WORD);
			return attribute;
		}

		const Object* UIText::SizeModeEnumAttribute() {
			static const Reference<const Object> attribute =
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<SizeMode>>>(false,
					"CANVAS_UNITS", SizeMode::CANVAS_UNITS,
					"RECT_FRACTION", SizeMode::RECT_FRACTION);
			return attribute;
		}

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

		void UIText::SetFontSizeMode(SizeMode mode) {
			m_sizeMode = (mode == SizeMode::RECT_FRACTION) ? SizeMode::RECT_FRACTION : SizeMode::CANVAS_UNITS;
		}

		void UIText::SetRectSizeBias(float bias) {
			m_rectSizeBias = Math::Min(Math::Max(0.0f, bias), 1.0f);
		}

		void UIText::SetColor(const Vector4& color) {
			m_color = color;
		}

		void UIText::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_text, "Text", "Displayed text", Serialization::TextBoxAttribute::Instance());

				static const std::string fontHint = "Sampler to the main texture (overrides material filed named '" + std::string(FontTextureShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Font, SetFont, "Font", fontHint);

				JIMARA_SERIALIZE_FIELD_GET_SET(FontSize, SetFontSize, "Font Size", "Font size in canvas units");
				JIMARA_SERIALIZE_FIELD_GET_SET(FontSizeMode, SetFontSizeMode, "Size Mode", "Font size mode", SizeModeEnumAttribute());
				if (FontSizeMode() == SizeMode::RECT_FRACTION)
					JIMARA_SERIALIZE_FIELD_GET_SET(RectSizeBias, SetRectSizeBias, "Size bias",
						"Size mode bias for SizeMode::RECT_FRACTION; Ignored for SizeMode::CANVAS_UNITS; 0 means scaled height; 1 - width.",
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));

				static const std::string colorHint = "Image color multiplier (appears as vertex color input with the name: '" + std::string(ColorShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", colorHint, Object::Instantiate<Serialization::ColorAttribute>());

				JIMARA_SERIALIZE_FIELD_GET_SET(HorizontalAlignment, SetHorizontalAlignment, "Horizontal Alignment",
					"0.5 means 'centered', 0 will start from boundary rect start and 1 will make the text end at the boundary end",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD_GET_SET(VerticalAlignment, SetVerticalAlignment, "Vertical Alignment",
					"0.5 means 'centered', 0 will start from boundary rect top and 1 will make the text end at the boundary bottom",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));

				JIMARA_SERIALIZE_FIELD_GET_SET(LineWrapping, SetLineWrapping, "Line Wrapping", "Line wrapping mode", WrappingModeEnumAttribute());
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
