#pragma once
#include "../../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SphereColliderGizmo);

		/// <summary>
		/// Gizmo for sphere colliders
		/// </summary>
		class SphereColliderGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			SphereColliderGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SphereColliderGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		protected:
			// Renderer transform
			const Reference<Transform> m_poseTransform;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SphereColliderGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SphereColliderGizmo>();
}
