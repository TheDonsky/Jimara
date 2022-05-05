#include "SceneViewSelection.h"


namespace Jimara {
	namespace Editor {
		SceneViewSelection::SceneViewSelection(Scene::LogicContext* context)
			: Component(context, "SceneViewSelection") {
			m_hover = GizmoViewportHover::GetFor(GizmoContext()->Viewport());
		}

		SceneViewSelection::~SceneViewSelection() {}

		void SceneViewSelection::Update() {
			Vector2 viewportSize = GizmoContext()->Viewport()->Resolution();
			if ((!dynamic_cast<const EditorInput*>(Context()->Input())->Enabled())
				|| (viewportSize.x * viewportSize.y) <= std::numeric_limits<float>::epsilon()
				|| (!Context()->Input()->KeyDown(OS::Input::KeyCode::MOUSE_LEFT_BUTTON)) 
				|| (m_hover->HandleGizmoHover().component != nullptr)) return;

			static thread_local std::vector<Component*> components;
			{
				components.clear();
				const ViewportObjectQuery::Result selectionGizmoHover = m_hover->SelectionGizmoHover();
				if (selectionGizmoHover.component != nullptr) {
					Gizmo* gizmo = selectionGizmoHover.component->GetComponentInParents<Gizmo>();
					if (gizmo != nullptr) for (size_t i = 0; i < gizmo->TargetCount(); i++)
						components.push_back(gizmo->TargetComponent(i));
				}
				else {
					const ViewportObjectQuery::Result sceneHover = m_hover->TargetSceneHover();
					if (sceneHover.component != nullptr) components.push_back(sceneHover.component);
				}
			}
			if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
				|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
				GizmoContext()->Selection()->Select(components.data(), components.size());
			else if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
				|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT))
				GizmoContext()->Selection()->Deselect(components.data(), components.size());
			else {
				GizmoContext()->Selection()->DeselectAll();
				GizmoContext()->Selection()->Select(components.data(), components.size());
			}
			components.clear();
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
