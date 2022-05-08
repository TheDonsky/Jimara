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
		}

		void SceneViewSelection::OnDrawGizmoGUI() {
			if (!m_clickStart.has_value()) return;
			const Vector2 basePosition = dynamic_cast<const EditorInput*>(Context()->Input())->MouseOffset();
			const Vector2 clickStart = m_clickStart.value();
			const Vector2 clickEnd = m_hover->CursorPosition();
			ImDrawList* const draw_list = ImGui::GetWindowDrawList();
			auto toPos = [&](const Vector2& vec) { return ImVec2(basePosition.x + vec.x, basePosition.y + vec.y); };
			draw_list->AddRectFilled(
				toPos(Vector2(min(clickStart.x, clickEnd.x), min(clickStart.y, clickEnd.y))),
				toPos(Vector2(max(clickStart.x, clickEnd.x), max(clickStart.y, clickEnd.y))),
				IM_COL32(200, 200, 200, 100));
		}

		void SceneViewSelection::Update() {
			if (Context()->Input()->KeyUp(SELECTION_KEY) && m_clickStart.has_value()) {
				static const auto ctrlPressed = [](SceneViewSelection* self) {
					return self->Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
						|| self->Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL);
				};
				static const auto altPressed = [](SceneViewSelection* self) {
					return self->Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
						|| self->Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT);
				};
				void(*processHoverResult)(Object*, ViewportObjectQuery::Result) =
					[](Object* selfPtr, ViewportObjectQuery::Result result) {
					SceneViewSelection* const self = dynamic_cast<SceneViewSelection*>(selfPtr);
					static thread_local std::vector<Component*> components;
					components.clear();
					if (result.component == nullptr) return;
					else if (result.component->Context() == self->Context()) {
						Gizmo* gizmo = result.component->GetComponentInParents<Gizmo>();
						if (gizmo != nullptr) for (size_t i = 0; i < gizmo->TargetCount(); i++)
							components.push_back(gizmo->TargetComponent(i));
					}
					else components.push_back(result.component);
					if (altPressed(self)) 
						self->GizmoContext()->Selection()->Deselect(components.data(), components.size());
					else self->GizmoContext()->Selection()->Select(components.data(), components.size());
					components.clear();
				};
				const Vector2 mouseStart = m_clickStart.value();
				const Vector2 mouseEnd = m_hover->CursorPosition();
				const Size2 queryStart = Size2(static_cast<uint32_t>(min(mouseStart.x, mouseEnd.x)), static_cast<uint32_t>(min(mouseStart.y, mouseEnd.y)));
				const Size2 queryEnd = Size2(static_cast<uint32_t>(max(mouseStart.x, mouseEnd.x)), static_cast<uint32_t>(max(mouseStart.y, mouseEnd.y)));
				if ((!ctrlPressed(this)) && (!altPressed(this)))
					GizmoContext()->Selection()->DeselectAll();
				if (queryStart == queryEnd) {
					const ViewportObjectQuery::Result selectionGizmoHover = m_hover->SelectionGizmoHover();
					if (selectionGizmoHover.component != nullptr)
						processHoverResult(this, selectionGizmoHover);
					else processHoverResult(this, m_hover->TargetSceneHover());
				}
				else {
					const Callback<Object*, ViewportObjectQuery::Result> processCallback(processHoverResult);
					
					// __TODO__: This is rather slow... Come up with a faster solution...

					Stopwatch timer;
					for (uint32_t i = queryStart.x; i <= queryEnd.x; i++)
						for (uint32_t j = queryStart.y; j <= queryEnd.y; j++) {
							const Size2 position(i, j);
							m_hover->SelectionGizmoQuery()->QueryAsynch(position, processCallback, this);
							m_hover->TargetSceneQuery()->QueryAsynch(position, processCallback, this);
						}
					Context()->Log()->Debug("SceneViewSelection::Update - Query time: ", timer.Elapsed());
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
