#pragma once
#include "EditorSceneController.h"
#include "../EditorWindow.h"
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneView);

		/// <summary>
		/// Scene view with in-editor navigation and editing
		/// </summary>
		class SceneView : public virtual EditorSceneController, public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			SceneView(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneView();

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Input, dedicated to this particular view
			const Reference<EditorInput> m_input;

			// Editor scene from the last update
			Reference<EditorScene> m_editorScene;

			// Scene context form the last update
			Reference<Scene::LogicContext> m_viewContext;

			// Scene view camera update job
			Reference<JobSystem::Job> m_updateJob;
		};
	}

	// TypeIdDetails for SceneView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneView>(const Callback<const Object*>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneView>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneView>();
}
