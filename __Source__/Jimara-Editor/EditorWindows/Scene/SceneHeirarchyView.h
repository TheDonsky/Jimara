#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneHeirarchyView);

		class SceneHeirarchyView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			SceneHeirarchyView(EditorContext* context);

			virtual void DrawEditorWindow() final override;
		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>();
}
