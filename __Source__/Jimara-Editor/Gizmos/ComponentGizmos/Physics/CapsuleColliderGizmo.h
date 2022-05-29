#pragma once
#include "../../Gizmo.h"
#include "../../Handles/DragHandle.h"
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

			// Handles
			struct RadiusHandles {
				const Reference<DragHandle> right;
				const Reference<DragHandle> left;
				const Reference<DragHandle> front;
				const Reference<DragHandle> back;

				RadiusHandles(CapsuleColliderGizmo* parent);
			};
			struct HeightHandles {
				const Reference<DragHandle> top;
				const Reference<DragHandle> bottom;

				HeightHandles(CapsuleColliderGizmo* parent);
			};
			HeightHandles m_heightHandles;
			RadiusHandles m_topRadiusHandles;
			RadiusHandles m_bottomRadiusHandles;
			
			// Basic helper interface for internal logic
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::CapsuleColliderGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::CapsuleColliderGizmo>();
}
