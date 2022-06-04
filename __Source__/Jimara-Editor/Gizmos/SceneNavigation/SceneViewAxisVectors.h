#pragma once
#include "../Gizmo.h"
#include "../../GUI/ImGuiRenderer.h"
#include <Environment/Rendering/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneViewAxisVectors </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewAxisVectors);

		/// <summary> Component, responsible for drawing arrows in the corner </summary>
		class SceneViewAxisVectors : 
			public virtual Gizmo,
			public virtual GizmoGUI::Drawer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewAxisVectors(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewAxisVectors();

		protected:
			/// <summary> Lets a Drawer draw on SceneView </summary>
			virtual void OnDrawGizmoGUI() override;

		private:
			// Subscene
			const Reference<Scene> m_subscene;

			// Subscene render stack
			const Reference<RenderStack> m_renderStack;

			// Subscene objects
			Reference<Transform> m_cameraTransform;
			std::vector<Reference<Transform>> m_arrowTransforms;
			Reference<ViewportDescriptor> m_viewport;
			Reference<ViewportObjectQuery> m_query;

			// GUI-transferable render texture
			Reference<Graphics::TextureView> m_guiView;
			Reference<ImGuiTexture> m_guiTexture;

			// Private tools
			struct Tools;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewAxisVectors>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewAxisVectors>();
}
