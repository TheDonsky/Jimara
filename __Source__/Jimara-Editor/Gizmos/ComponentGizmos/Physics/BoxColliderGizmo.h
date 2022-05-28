#pragma once
#include "../../Gizmo.h"
#include "../../Handles/DragHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::BoxColliderGizmo);

		/// <summary>
		/// Gizmo for box colliders
		/// </summary>
		class BoxColliderGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
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
			// Renderer transform
			const Reference<Transform> m_poseTransform;

			// Handles
			const Reference<DragHandle> m_resizeRight;
			const Reference<DragHandle> m_resizeLeft;
			const Reference<DragHandle> m_resizeUp;
			const Reference<DragHandle> m_resizeDown;
			const Reference<DragHandle> m_resizeFront;
			const Reference<DragHandle> m_resizeBack;

			// Basic helper interface for internal logic
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::BoxColliderGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::BoxColliderGizmo>();
}
