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
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneViewHotKeyOperations>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Targetless<Editor::SceneViewHotKeyOperations>();
		report(connection);
	}
}
