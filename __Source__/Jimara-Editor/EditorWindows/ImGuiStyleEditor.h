#pragma once
#include "EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about ImGuiStyleEditor </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::ImGuiStyleEditor);

		/// <summary>
		/// Lets the user edit ImGuiStyle (mostly for internal use)
		/// </summary>
		class ImGuiStyleEditor : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			ImGuiStyleEditor(EditorContext* context);

			/// <summary> ImGui style serializer, the editor uses </summary>
			static const Serialization::ItemSerializer::Of<ImGuiStyle>* StyleSerializer();

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;
		};
	}

	// TypeIdDetails for GameView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::ImGuiStyleEditor>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::ImGuiStyleEditor>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::ImGuiStyleEditor>();
}
