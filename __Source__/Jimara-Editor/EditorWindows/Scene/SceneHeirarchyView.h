#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneHeirarchyView);

		class SceneHeirarchyView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			SceneHeirarchyView(EditorContext* context);

		protected:
			virtual void DrawEditorWindow() final override;

		private:
			const std::string m_addComponentPopupName;
			Reference<Component> m_addChildTarget;
		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>();
}
