#include "SceneViewSelection.h"
#include "../../GUI/ImGuiRenderer.h"
#include <Core/Stopwatch.h>


namespace Jimara {
	namespace Editor {
		SceneViewSelection::SceneViewSelection(Scene::LogicContext* context)
			: Component(context, "SceneViewSelection") {
			m_hover = GizmoViewportHover::GetFor(GizmoContext()->Viewport());
		}

		SceneViewSelection::~SceneViewSelection() {}

		namespace {
			inline static const constexpr OS::Input::KeyCode SELECTION_KEY = OS::Input::KeyCode::MOUSE_LEFT_BUTTON;
		
			inline static const auto CtrlPressed(SceneViewSelection* self) {
				return self->Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
					|| self->Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL);
			}
			inline static const auto AltPressed(SceneViewSelection* self) {
				return self->Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
					|| self->Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT);
			}

			inline static void ProcessResultComponentFromGizmoScene(ObjectSet<Component>& components, Component* component) {
				if (component == nullptr) return;
				Gizmo* gizmo = component->GetComponentInParents<Gizmo>();
				if (gizmo != nullptr)
					for (size_t i = 0; i < gizmo->TargetCount(); i++)
						components.Add(gizmo->TargetComponent(i));
			}

			inline static void ProcessResultComponent(SceneViewSelection* self, Component* component) {
				static thread_local ObjectSet<Component> components;
				components.Clear();
				if (component == nullptr) return;
				else if (component->Context() == self->Context())
					ProcessResultComponentFromGizmoScene(components, component);
				else components.Add(component);
				if (AltPressed(self))
					self->GizmoContext()->Selection()->Deselect(components.Data(), components.Size());
				else self->GizmoContext()->Selection()->Select(components.Data(), components.Size());
				components.Clear();
			}

			inline static void ProcessHoverResult(Object* selfPtr, ViewportObjectQuery::Result result) {
				SceneViewSelection* const self = dynamic_cast<SceneViewSelection*>(selfPtr);
				ProcessResultComponent(self, result.component);
			};

			inline static Reference<Graphics::ImageTexture> CombineImages(
				Graphics::GraphicsDevice* device, SizeAABB cursorRect,
				Graphics::TextureSampler* gizmoObjectIndex, Graphics::TextureSampler* gizmoInstanceIndex, Graphics::TextureSampler* gizmoPrimitiveIndex,
				Graphics::TextureSampler* sceneObjectIndex, Graphics::TextureSampler* sceneInstanceIndex, Graphics::TextureSampler* scenePrimitiveIndex) {
				const Size2 resultSize = (cursorRect.end - cursorRect.start + 1u);
				const Reference<Graphics::ImageTexture> resultTexture = device->CreateCpuReadableTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, gizmoObjectIndex->TargetView()->TargetTexture()->ImageFormat(), Size3(resultSize.x * 3, resultSize.y * 2, 1), 1);
				if (resultTexture == nullptr) {
					device->Log()->Error("SceneViewSelection::CombineImages - Failed to allocate host-readable texture for readback! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				Reference<Graphics::PrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
				commandBuffer->BeginRecording();
				auto copyImage = [&](Graphics::TextureSampler* sampler, uint32_t index, uint32_t scene) {
					Size3 base = Size3(resultSize.x * index, resultSize.y * scene, 0);
					resultTexture->Copy(commandBuffer, sampler->TargetView()->TargetTexture(), base, cursorRect.start, Size3(resultSize, 1));
				};
				auto copyImageRow = [&](Graphics::TextureSampler* objectIndex, Graphics::TextureSampler* instanceIndex, Graphics::TextureSampler* primitiveIndex, uint32_t scene) {
					copyImage(objectIndex, 0, scene);
					copyImage(instanceIndex, 1, scene);
					copyImage(primitiveIndex, 2, scene);
				};
				copyImageRow(gizmoObjectIndex, gizmoInstanceIndex, gizmoPrimitiveIndex, 0);
				copyImageRow(sceneObjectIndex, sceneInstanceIndex, scenePrimitiveIndex, 1);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
				return resultTexture;
			}

			inline static Reference<Graphics::ImageTexture> CombineImages(
				Graphics::GraphicsDevice* device, SizeAABB cursorRect,
				const ObjectIdRenderer::Reader& gizmoRendererResults, const ObjectIdRenderer::Reader& sceneRendererResult) {
				const ObjectIdRenderer::ResultBuffers gizmoImages = gizmoRendererResults.LastResults();
				const ObjectIdRenderer::ResultBuffers sceneImages = sceneRendererResult.LastResults();
				const constexpr Graphics::Texture::PixelFormat indexFormat = Graphics::Texture::PixelFormat::R32_UINT;
				auto incorrectFormat = [&](Graphics::TextureSampler* sampler) {
					return sampler->TargetView()->TargetTexture()->ImageFormat() != indexFormat;
				};
				if (incorrectFormat(sceneImages.objectIndex) ||
					incorrectFormat(sceneImages.instanceIndex) ||
					incorrectFormat(sceneImages.primitiveIndex) ||
					incorrectFormat(gizmoImages.objectIndex) ||
					incorrectFormat(gizmoImages.instanceIndex) ||
					incorrectFormat(gizmoImages.primitiveIndex)) {
					device->Log()->Error(
						"SceneViewSelection::CombineImages - instanceIndex, objectIndex and primitiveIndex expected to be of uin32_t type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return CombineImages(device, cursorRect,
					gizmoImages.objectIndex, gizmoImages.instanceIndex, gizmoImages.primitiveIndex,
					sceneImages.objectIndex, sceneImages.instanceIndex, sceneImages.primitiveIndex);
			}

			inline static SizeAABB CursorRect(const std::optional<Vector2>& clickStart, GizmoViewportHover* hover, Size2 resolution) {
				if (resolution.x > 0) resolution.x--;
				if (resolution.y > 0) resolution.y--;
				auto toSize = [&](const Vector2& pos) { return Size2(static_cast<uint32_t>(max(pos.x, 0.0f)), static_cast<uint32_t>(max(pos.y, 0.0f))); };
				const Size2 queryEnd = toSize(hover->CursorPosition());
				const Size2 queryStart = clickStart.has_value() ? toSize(clickStart.value()) : queryEnd;
				return SizeAABB(
					Size3(min(resolution.x, min(queryStart.x, queryEnd.x)), min(resolution.y, min(queryStart.y, queryEnd.y)), 0),
					Size3(min(resolution.x, max(queryStart.x, queryEnd.x)), min(resolution.y, max(queryStart.y, queryEnd.y)), 1));
			}

			template<typename Inspect>
			inline static void ExtractComponents(
				ThreadBlock& block, const ObjectIdRenderer::Reader& results, 
				const uint32_t* data, Size2 size, uint32_t rowSize, Inspect inspect) {
				static thread_local std::vector<std::unordered_set<Reference<Component>>> perThreadComponents(std::thread::hardware_concurrency());
				auto executeOnThread = [&](ThreadBlock::ThreadInfo info, void* sets) {
					std::unordered_set<Reference<Component>>& set = reinterpret_cast<std::unordered_set<Reference<Component>>*>(sets)[info.threadId];
					set.clear();
					uint32_t rowsPerThread = (size.y + static_cast<uint32_t>(info.threadCount) - 1) / static_cast<uint32_t>(info.threadCount);
					uint32_t firstRow = rowsPerThread * static_cast<uint32_t>(info.threadId);
					uint32_t lastRow = min(firstRow + rowsPerThread, size.y);
					for (uint32_t y = firstRow; y < lastRow; y++) {
						const uint32_t* row = data + (static_cast<size_t>(rowSize) * y);
						const uint32_t* const rowEnd = row + size.x;
						while (row < rowEnd) {
							const uint32_t objectId = (*row);
							row++;
							if (objectId == (~(uint32_t(0u)))) continue;
							GraphicsObjectDescriptor* descriptor = results.Descriptor(objectId);
							if (descriptor == nullptr) continue;
							const uint32_t* instanceIdPtr = row + size.x;
							const uint32_t* primitiveIdPtr = instanceIdPtr + size.x;
							const uint32_t instanceId = *instanceIdPtr;
							const uint32_t primitiveId = *primitiveIdPtr;
							set.insert(descriptor->GetComponent(instanceId, primitiveId));
							while (row < rowEnd) {
								if (objectId != (*row)) break;
								instanceIdPtr++;
								if (instanceId != (*instanceIdPtr)) break;
								primitiveIdPtr++;
								if (primitiveId != (*primitiveIdPtr)) break;
								row++;
							}
						}
					}
				};
				block.Execute(
					perThreadComponents.size(), 
					reinterpret_cast<void*>(perThreadComponents.data()), 
					Callback<ThreadBlock::ThreadInfo, void*>::FromCall(&executeOnThread));
				for (size_t i = 0; i < perThreadComponents.size(); i++) {
					std::unordered_set<Reference<Component>>& set = perThreadComponents[i];
					for (const auto& component : set) inspect(component);
					set.clear();
				}
			}
		}

		void SceneViewSelection::OnDrawGizmoGUI() {
			if (!m_clickStart.has_value()) return;
			const SizeAABB cursorRect = CursorRect(m_clickStart, m_hover, GizmoContext()->Viewport()->Resolution());
			const Vector2 basePosition = dynamic_cast<const EditorInput*>(Context()->Input())->MouseOffset();
			ImDrawList* const draw_list = ImGui::GetWindowDrawList();
			auto toPos = [&](const Size2& vec) { return ImVec2(basePosition.x + vec.x, basePosition.y + vec.y); };
			draw_list->AddRectFilled(toPos(cursorRect.start), toPos(cursorRect.end), IM_COL32(200, 200, 200, 100));
		}

		void SceneViewSelection::Update() {
			if (Context()->Input()->KeyUp(SELECTION_KEY) && m_clickStart.has_value()) {
				const SizeAABB cursorRect = CursorRect(m_clickStart, m_hover, GizmoContext()->Viewport()->Resolution());
				if ((!CtrlPressed(this)) && (!AltPressed(this)))
					GizmoContext()->Selection()->DeselectAll();
				if (cursorRect.start.x == cursorRect.end.x && cursorRect.start.y == cursorRect.end.y) {
					const ViewportObjectQuery::Result selectionGizmoHover = m_hover->SelectionGizmoHover();
					if (selectionGizmoHover.component != nullptr)
						ProcessHoverResult(this, selectionGizmoHover);
					else ProcessHoverResult(this, m_hover->TargetSceneHover());
				}
				else {
					const Callback<Object*, ViewportObjectQuery::Result> processCallback(ProcessHoverResult);
					Stopwatch timer;
					ObjectIdRenderer::Reader targetResults(m_hover->TargetSceneIdRenderer());
					ObjectIdRenderer::Reader gizmoResults(m_hover->SelectionGizmoIdRenderer());
					const Reference<Graphics::ImageTexture> combinedTexture = CombineImages(Context()->Graphics()->Device(), cursorRect, gizmoResults, targetResults);
					if (combinedTexture != nullptr) {
						const uint32_t* data = reinterpret_cast<uint32_t*>(combinedTexture->Map());
						const uint32_t rowSize = combinedTexture->Pitch().x;
						const Size2 resultSize = (cursorRect.end - cursorRect.start + 1u);
						static thread_local ObjectSet<Component> components;
						components.Clear();
						{
							static thread_local std::unordered_set<Reference<Component>> gizmos;
							gizmos.clear();
							ExtractComponents(m_processingBlock, gizmoResults, data, resultSize, rowSize, [&](Component* v) { gizmos.insert(v); });
							for (const auto& component : gizmos)
								ProcessResultComponentFromGizmoScene(components, component);
							gizmos.clear();
						}
						{
							data += static_cast<size_t>(rowSize) * resultSize.y;
							ExtractComponents(m_processingBlock, targetResults, data, resultSize, rowSize, [&](const auto& v) { components.Add(v); });
						}
						if (AltPressed(this))
							GizmoContext()->Selection()->Deselect(components.Data(), components.Size());
						else GizmoContext()->Selection()->Select(components.Data(), components.Size());
						combinedTexture->Unmap(false);
						components.Clear();
					}
					//Context()->Log()->Info("SceneViewSelection::Update - Query time: ", timer.Elapsed());
				}
				m_clickStart = std::optional<Vector2>();
			}

			if ((!Context()->Input()->KeyPressed(SELECTION_KEY))) m_clickStart = std::optional<Vector2>();

			Vector2 viewportSize = GizmoContext()->Viewport()->Resolution();
			if ((dynamic_cast<const EditorInput*>(Context()->Input())->Enabled())
				&& (viewportSize.x * viewportSize.y) > std::numeric_limits<float>::epsilon()
				&& (Context()->Input()->KeyDown(SELECTION_KEY))
				&& (m_hover->HandleGizmoHover().component == nullptr))
				m_clickStart = m_hover->CursorPosition();
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SceneViewSelection_GizmoConnection = Gizmo::ComponentConnection::Targetless<SceneViewSelection>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewSelection>() {
		Editor::Gizmo::AddConnection(Editor::SceneViewSelection_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewSelection>() {
		Editor::Gizmo::RemoveConnection(Editor::SceneViewSelection_GizmoConnection);
	}
}
