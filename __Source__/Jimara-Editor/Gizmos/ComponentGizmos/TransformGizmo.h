#pragma once
#include "../Gizmo.h"
#include "../Handles/FreeMoveHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformGizmo);

		/// <summary>
		/// Gizmo for TRansform components
		/// </summary>
		class TransformGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			TransformGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~TransformGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying transform handle
			const Reference<FreeMoveHandle> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformGizmo>();
}
