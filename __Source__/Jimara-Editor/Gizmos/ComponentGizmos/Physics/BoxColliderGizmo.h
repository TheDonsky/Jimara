#pragma once
#include "../../Gizmo.h"
#include "../../Handles/Compound/BoxResizeHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::BoxColliderGizmo);

		/// <summary>
		/// Gizmo for box colliders
		/// </summary>
		class BoxColliderGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			BoxColliderGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~BoxColliderGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		protected:
			// Underlying handle
			const Reference<BoxResizeHandle> m_resizeHandle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::BoxColliderGizmo>(const Callback<const Object*>& report);
}
