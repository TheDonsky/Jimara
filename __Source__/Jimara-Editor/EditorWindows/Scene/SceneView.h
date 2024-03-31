#pragma once
#include "../../Gizmos/GizmoScene.h"
#include "EditorSceneWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneView </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneView);

		/// <summary>
		/// Scene view with in-editor navigation and editing
		/// </summary>
		class SceneView : public virtual EditorSceneWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			SceneView(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneView();

			/// <summary> Gizmo scene context </summary>
			GizmoScene::Context* GizmoContext();

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

			/// <summary> Invoked, whenever the editor window is hidden and DrawEditorWindow() is not invoked </summary>
			virtual void OnEditorWindowDrawSkipped() final override;

		private:
			// Input, dedicated to this particular view
			const Reference<EditorInput> m_input;

			// Last size of the screen and number of times it already appeared
			Size2 m_lastResolution = Size2(0);
			size_t m_sameResolutionCount = 0;

			// Editor scene from the last update
			Reference<EditorScene> m_editorScene;

			// Scene context form the last update
			Reference<Scene::LogicContext> m_viewContext;

			// Gizmo scene
			Reference<GizmoScene> m_gizmoScene;

			// Last image
			Reference<Graphics::TextureView> m_lastImage;
			Reference<ImGuiTexture> m_lastSampler;
		};
	}

	// TypeIdDetails for SceneView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneView>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneView>(const Callback<const Object*>& report);
}
