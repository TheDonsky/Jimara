#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::GameView);

		class GameView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			GameView(EditorContext* context);

		protected:
			virtual void DrawEditorWindow() final override;

		private:

		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::GameView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::GameView>();
}
