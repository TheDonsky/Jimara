#pragma once
#include "../../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SubsceneGizmo);

		/// <summary>
		/// Gizmo subscenes
		/// </summary>
		class SubsceneGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			SubsceneGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SubsceneGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Transform of visual representation
			const Reference<Transform> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SubsceneGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SubsceneGizmo>();
}
