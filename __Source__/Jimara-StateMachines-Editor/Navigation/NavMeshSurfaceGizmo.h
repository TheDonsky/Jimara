#pragma once
#include "../Types.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara-Editor/Gizmos/Gizmo.h>

namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::NavMeshSurfaceGizmo);

		/// <summary>
		/// Gizmo, displaying a navigation mesh surface geometry in SceneView
		/// </summary>
		class JIMARA_STATE_MACHINES_EDITOR_API NavMeshSurfaceGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			NavMeshSurfaceGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~NavMeshSurfaceGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying renderers
			Reference<MeshRenderer> m_areaRenderer;
			Reference<MeshRenderer> m_wireRenderer;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceGizmo>(const Callback<const Object*>& report);
}
