#pragma once
#include "../../Gizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::CapsuleColliderGizmo);

		/// <summary>
		/// Gizmo for capsule colliders
		/// </summary>
		class CapsuleColliderGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			CapsuleColliderGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~CapsuleColliderGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		protected:
			// Underlying renderer and last capsule shape
			const Reference<MeshRenderer> m_renderer;
			float m_lastRadius = 0.0f;
			float m_lastHeight = 0.0f;

			// Cache of capsule-meshes
			class MeshCache;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::CapsuleColliderGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::CapsuleColliderGizmo>();
}
