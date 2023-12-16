#include "SceneViewSelection.h"
#include "../../GUI/ImGuiRenderer.h"
#include <Components/Level/Subscene.h>
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

			inline static void ProcessResultComponentFromTargetScene(ObjectSet<Component>& components, Component* component) {
				while (true) {
					Component* subscene = Subscene::GetSubscene(component);
					if (subscene != nullptr)
						component = subscene;
					else break;
				}
				components.Add(component);
			}

			inline static void ProcessResultComponent(SceneViewSelection* self, Component* component) {
				static thread_local ObjectSet<Component> components;
				components.Clear();
				if (component == nullptr) return;
				else if (component->Context() == self->Context())
					ProcessResultComponentFromGizmoScene(components, component);
				else ProcessResultComponentFromTargetScene(components, component);
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
				Graphics::GraphicsDevice* device, SizeAABB cursorRect, Graphics::TextureSampler* gizmoObjectIndex, Graphics::TextureSampler* sceneObjectIndex) {
				const Size2 resultSize = (cursorRect.end - cursorRect.start + 1u);
				const Reference<Graphics::ImageTexture> resultTexture = device->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, gizmoObjectIndex->TargetView()->TargetTexture()->ImageFormat(),
					Size3(resultSize.x, resultSize.y * 2u, 1u), 1u, false,
					Graphics::ImageTexture::AccessFlags::CPU_READ);
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
				copyImage(gizmoObjectIndex, 0u, 0u);
				copyImage(sceneObjectIndex, 0u, 1u);
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
				const constexpr Graphics::Texture::PixelFormat indexFormat = Graphics::Texture::PixelFormat::R32G32B32A32_UINT;
				auto incorrectFormat = [&](Graphics::TextureSampler* sampler) {
					return sampler->TargetView()->TargetTexture()->ImageFormat() != indexFormat;
				};
				if (incorrectFormat(sceneImages.compoundIndex) ||
					incorrectFormat(gizmoImages.compoundIndex)) {
					device->Log()->Error(
						"SceneViewSelection::CombineImages - instanceIndex, objectIndex and primitiveIndex expected to be of uin32_t type! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return CombineImages(device, cursorRect, gizmoImages.compoundIndex, sceneImages.compoundIndex);
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
				const Size4* data, Size2 size, uint32_t rowSize, Inspect inspect) {

				// Extract components from a single line of the data:
				static const auto processLine = [](
					const ObjectIdRenderer::Reader& results,
					const Size4* row, const Size4* const rowEnd, size_t delta, auto recordComponent) {
					while (row < rowEnd) {
						const Size4& compoundId = (*row);
						row += delta;
						if (compoundId.r == (~(uint32_t(0u)))) 
							continue;
						const GraphicsObjectDescriptor::ViewportData* descriptor = results.Descriptor(compoundId.r).viewportData;
						if (descriptor == nullptr) 
							continue;
						const Reference<Component> component = descriptor->GetComponent(compoundId.b, compoundId.a);
						if (component != nullptr)
							recordComponent(component);
						while ((row < rowEnd) &&
							row->r == compoundId.r &&
							row->g == compoundId.g)
							row += delta;
					}
				};
				
				// Extract all components from data:
				static thread_local std::vector<std::unordered_set<Reference<Component>>> perThreadComponents(std::thread::hardware_concurrency());
				{
					auto executeOnThread = [&](ThreadBlock::ThreadInfo info, void* sets) {
						std::unordered_set<Reference<Component>>& set = reinterpret_cast<std::unordered_set<Reference<Component>>*>(sets)[info.threadId];
						set.clear();
						uint32_t rowsPerThread = (size.y + static_cast<uint32_t>(info.threadCount) - 1) / static_cast<uint32_t>(info.threadCount);
						uint32_t firstRow = rowsPerThread * static_cast<uint32_t>(info.threadId);
						uint32_t lastRow = min(firstRow + rowsPerThread, size.y);
						for (uint32_t y = firstRow; y < lastRow; y++) {
							const Size4* row = data + (static_cast<size_t>(rowSize) * y);
							const Size4* const rowEnd = row + size.x;
							processLine(results, row, rowEnd, 1, [&](Component* component) { set.insert(component); });
						}
					};
					block.Execute(
						perThreadComponents.size(),
						reinterpret_cast<void*>(perThreadComponents.data()),
						Callback<ThreadBlock::ThreadInfo, void*>::FromCall(&executeOnThread));
				}
				
				// Unify all components that were found:
				static thread_local std::unordered_set<Reference<Component>> allComponents;
				{
					allComponents.clear();
					for (size_t i = 0; i < perThreadComponents.size(); i++) {
						std::unordered_set<Reference<Component>>& set = perThreadComponents[i];
						for (const auto& component : set) allComponents.insert(component);
						set.clear();
					};
				}

				// Exclude components that overlap with the selection rect boundary to avoid selecting background:
				if (size.x > 1 && size.y > 1) {
					auto eraseComponent = [&](Component* component) { allComponents.erase(component); };
					const size_t lastRowOffset = (static_cast<size_t>(rowSize) * size_t(size.y - 1u));
					const Size4* const lastRow = (data + lastRowOffset);
					const Size4* const lastColumn = (data + size.x - 1);
					processLine(results, data,			data + size.x,				1,			eraseComponent);
					processLine(results, lastRow,		lastRow + size.x,			1,			eraseComponent);
					processLine(results, data,			lastRow,					rowSize,	eraseComponent);
					processLine(results, lastColumn,	lastColumn + lastRowOffset,	rowSize,	eraseComponent);
				}

				// Report "Findings":
				for (const auto& component : allComponents) inspect(component);
				allComponents.clear();
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
						const Size4* data = reinterpret_cast<Size4*>(combinedTexture->Map());
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
							ExtractComponents(m_processingBlock, targetResults, data, resultSize, rowSize,
								[&](const auto& v) { ProcessResultComponentFromTargetScene(components, v); });
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
