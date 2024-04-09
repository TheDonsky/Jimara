#pragma once
#include "EditorSceneWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneHierarchyView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneHierarchyView);

		/// <summary>
		/// Displays scene graph/tree in an ImGui window
		/// </summary>
		class SceneHierarchyView : public virtual EditorSceneWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			SceneHierarchyView(EditorContext* context);

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

	// TypeIdDetails for SceneHierarchyView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHierarchyView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneHierarchyView>(const Callback<const Object*>& report);
}
