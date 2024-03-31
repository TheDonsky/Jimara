#pragma once
#include "EditorSceneWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about GameView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::GameView);

		/// <summary>
		/// Draws whatever the game renderer will render to ImGui window
		/// </summary>
		class GameView : public virtual EditorSceneWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			GameView(EditorContext* context);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Last image
			Reference<Graphics::TextureView> m_lastImage;
			Reference<ImGuiTexture> m_lastSampler;
		};
	}

	// TypeIdDetails for GameView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::GameView>(const Callback<const Object*>& report);
}
