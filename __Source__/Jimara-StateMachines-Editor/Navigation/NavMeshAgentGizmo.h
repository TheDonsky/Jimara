#pragma once
#include "../Types.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara-Editor/Gizmos/Gizmo.h>

namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::NavMeshAgentGizmo);

		/// <summary>
		/// Gizmo, displaying a navigation mesh agent path geometry in SceneView
		/// </summary>
		class JIMARA_STATE_MACHINES_EDITOR_API NavMeshAgentGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			NavMeshAgentGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~NavMeshAgentGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying renderers
			const Reference<MeshRenderer> m_pathRenderer;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshAgentGizmo>(const Callback<const Object*>& report);
}
