#pragma once
#include "../../Gizmo.h"
#include "../../Handles/Compound/CapsuleResizeHandle.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::CapsuleColliderGizmo);

		/// <summary>
		/// Gizmo for capsule colliders
		/// </summary>
		class CapsuleColliderGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
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
			// Underlying handle
			const Reference<CapsuleResizeHandle> m_resizeHandle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::CapsuleColliderGizmo>(const Callback<const Object*>& report);
}
