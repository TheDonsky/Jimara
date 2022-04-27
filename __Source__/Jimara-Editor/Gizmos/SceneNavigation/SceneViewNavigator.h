#pragma once
#include "../Gizmo.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneViewNavigator </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewNavigator);

		/// <summary>
		/// Basic 'global' gizmo, responsible for scene view navigation
		/// </summary>
		class SceneViewNavigator : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewNavigator(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewNavigator();

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;

		private:
			// Hover query instance
			Reference<GizmoViewportHover> m_hover;

			// Whenevr any action starts, we store initial mouse position
			Vector2 m_actionMousePositionOrigin = Vector2(0.0f);

			// Drag/Move action state
			struct {
				Vector3 startPosition = Vector3(0.0f);
				float speed = 0.0f;
			} m_drag;

			// Rotation action state
			struct {
				Vector3 target = Vector3(0.0f);
				Vector3 startAngles = Vector3(0.0f);
				Vector3 startOffset = Vector3(0.0f);
				float speed = 180.0f;
			} m_rotation;

			// Zoom action state
			struct {
				float speed = 0.125f;
			} m_zoom;

			// Drag action processor
			bool Drag(const ViewportObjectQuery::Result& hover, Vector2 viewportSize);

			// Rotation action processor
			bool Rotate(const ViewportObjectQuery::Result& hover, Vector2 viewportSize);

			// Zoom action processor
			bool Zoom(const ViewportObjectQuery::Result& hover);
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewNavigator>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewNavigator>();
}
