#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneHeirarchyView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneHeirarchyView);

		/// <summary>
		/// Displays scene graph/tree in an ImGui window
		/// </summary>
		class SceneHeirarchyView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			SceneHeirarchyView(EditorContext* context);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Active "Add component" popup identifier
			const std::string m_addComponentPopupName;

			// Currently selected component, we're adding a popup to
			Reference<Component> m_addChildTarget;
		};
	}

	// TypeIdDetails for SceneHeirarchyView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>();
}
