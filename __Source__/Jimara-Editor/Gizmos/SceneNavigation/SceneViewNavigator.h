#pragma once
#include "../Gizmo.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewNavigator);

		class SceneViewNavigator : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			SceneViewNavigator(Scene::LogicContext* context);
			virtual ~SceneViewNavigator();

		protected:
			virtual void Update()override;

		private:
			Reference<GizmoViewportHover> m_hover;

			Vector2 m_actionMousePositionOrigin = Vector2(0.0f);

			struct {
				Vector3 startPosition = Vector3(0.0f);
				float speed = 0.0f;
			} m_drag;

			struct {
				Vector3 target = Vector3(0.0f);
				Vector3 startAngles = Vector3(0.0f);
				Vector3 startOffset = Vector3(0.0f);
				float speed = 180.0f;
			} m_rotation;

			struct {
				float speed = 0.125f;
			} m_zoom;

			bool Drag(const ViewportObjectQuery::Result& hover, Vector2 viewportSize);

			bool Rotate(const ViewportObjectQuery::Result& hover, Vector2 viewportSize);

			bool Zoom(const ViewportObjectQuery::Result& hover);
		};
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewNavigator>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewNavigator>();
}
