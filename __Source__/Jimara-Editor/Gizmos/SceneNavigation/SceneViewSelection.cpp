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
				|| (m_hover->GizmoSceneHover().component != nullptr)) return;

			const ViewportObjectQuery::Result hover = m_hover->TargetSceneHover();
			if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL)
				|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))
				GizmoContext()->Selection()->Select(hover.component);
			else if (Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_ALT)
				|| Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_ALT))
				GizmoContext()->Selection()->Deselect(hover.component);
			else {
				GizmoContext()->Selection()->DeselectAll();
				GizmoContext()->Selection()->Select(hover.component);
			}
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
