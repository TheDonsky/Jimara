#pragma once
#include "EditorSceneWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneHeirarchyView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneHeirarchyView);

		/// <summary>
		/// Displays scene graph/tree in an ImGui window
		/// </summary>
		class SceneHeirarchyView : public virtual EditorSceneWindow {
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

			// Components of interest:
			Reference<Component> m_addChildTarget;
			struct {
				Reference<Component> reference;
				bool justStartedRenaming = false;
			} m_componentBeingRenamed;
			Reference<Component> m_rightClickMenuTarget;

			// Helper toolbox:
			struct Tools;
		};
	}

	// TypeIdDetails for SceneHeirarchyView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneHeirarchyView>(const Callback<const Object*>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>();
}
