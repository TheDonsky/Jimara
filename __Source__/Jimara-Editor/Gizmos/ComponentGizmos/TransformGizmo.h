#pragma once
#include "../Gizmo.h"
#include "../Handles/Compound/TripleAxisMoveHandle.h"
#include "../Handles/Compound/TripleAxisScalehandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformGizmo);

		/// <summary>
		/// Gizmo for TRansform components
		/// </summary>
		class TransformGizmo 
			: public virtual Gizmo
			, public virtual GizmoGUI::Drawer
			, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			TransformGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~TransformGizmo();

		protected:
			/// <summary> Lets the user select between transform, scale and rotation gizmos </summary>
			virtual void OnDrawGizmoGUI()override;

			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying transform handles
			const Reference<TripleAxisMoveHandle> m_moveHandle;
			const Reference<TripleAxisScalehandle> m_scaleHandle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformGizmo>();
}
