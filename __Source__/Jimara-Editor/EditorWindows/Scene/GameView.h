#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about GameView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::GameView);

		/// <summary>
		/// Draws whatever the game renderer will render to ImGui window
		/// </summary>
		class GameView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			GameView(EditorContext* context);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;
		};
	}

	// TypeIdDetails for GameView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::GameView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::GameView>();
}
