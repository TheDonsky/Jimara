#pragma once
#include "../../Gizmo.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::MeshColliderGizmo);

		/// <summary>
		/// Gizmo for mesh colliders
		/// </summary>
		class MeshColliderGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			MeshColliderGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~MeshColliderGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		protected:
			// Underlying renderer
			const Reference<MeshRenderer> m_renderer;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MeshColliderGizmo>(const Callback<const Object*>& report);
}
