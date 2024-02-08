#pragma once
#include "../../../Gizmo.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::PointLightGizmo);

		/// <summary>
		/// Gizmo for a point light
		/// </summary>
		class PointLightGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			PointLightGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~PointLightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Handle transform
			const Reference<Transform> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::PointLightGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::PointLightGizmo>();
}
