#pragma once
#include "../Gizmo.h"
#include "../Handles/Handle.h"
#include <Jimara/Components/Camera.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::CameraGizmo);

		/// <summary>
		/// Gizmo for Camera component
		/// </summary>
		class CameraGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			CameraGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~CameraGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying transform handle
			const Reference<Transform> m_handle;

			// Frustrum renderer data
			Reference<MeshRenderer> m_frustrumRenderer;
			Camera::ProjectionMode m_projectionMode = Camera::ProjectionMode::PERSPECTIVE;
			float m_fieldOfView = 0.0f;
			float m_orthographicSize = 0.0f;
			float m_closePlane = 0.0f;
			float m_farPlane = 0.0f;
			float m_aspectRatio = 0.0f;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::CameraGizmo>(const Callback<const Object*>& report);
}
