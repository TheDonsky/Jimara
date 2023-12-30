#include "SceneViewHotKeyOperations.h"
#include "../../ActionManagement/SelectionClipboardOperations.h"


namespace Jimara {
	namespace Editor {
		SceneViewHotKeyOperations::SceneViewHotKeyOperations(Scene::LogicContext* context)
			: Component(context, "SceneViewHotKeyOperations") {}

		SceneViewHotKeyOperations::~SceneViewHotKeyOperations() {}

		void SceneViewHotKeyOperations::Update() {
			PerformSelectionClipboardOperations(
				GizmoContext()->Clipboard(), GizmoContext()->Selection(), Context()->Input());
			if (HotKey::Delete().Check(Context()->Input())) {
				const auto currentSelection = GizmoContext()->Selection()->Current();
				for (Component* component : currentSelection)
					if (!component->Destroyed())
						component->Destroy();
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SceneViewHotKeyOperations_GizmoConnection =
				Gizmo::ComponentConnection::Targetless<SceneViewHotKeyOperations>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewHotKeyOperations>() {
		Editor::Gizmo::AddConnection(Editor::SceneViewHotKeyOperations_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewHotKeyOperations>() {
		Editor::Gizmo::RemoveConnection(Editor::SceneViewHotKeyOperations_GizmoConnection);
	}
}
