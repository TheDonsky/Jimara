#include "SceneViewClipboardOperations.h"
#include "../../ActionManagement/SelectionClipboardOperations.h"


namespace Jimara {
	namespace Editor {
		SceneViewClipboardOperations::SceneViewClipboardOperations(Scene::LogicContext* context)
			: Component(context, "SceneViewClipboardOperations") {}

		SceneViewClipboardOperations::~SceneViewClipboardOperations() {}

		void SceneViewClipboardOperations::Update() {
			PerformSelectionClipboardOperations(
				GizmoContext()->Clipboard(), GizmoContext()->Selection(), Context()->Input());
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SceneViewClipboardOperations_GizmoConnection =
				Gizmo::ComponentConnection::Targetless<SceneViewClipboardOperations>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewClipboardOperations>() {
		Editor::Gizmo::AddConnection(Editor::SceneViewClipboardOperations_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewClipboardOperations>() {
		Editor::Gizmo::RemoveConnection(Editor::SceneViewClipboardOperations_GizmoConnection);
	}
}
