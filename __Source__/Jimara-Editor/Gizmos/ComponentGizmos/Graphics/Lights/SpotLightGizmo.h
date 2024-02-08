#pragma once
#include "../../../Gizmo.h"
#include "../../../Handles/Handle.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SpotLightGizmo);

		/// <summary>
		/// Gizmo for a spotlight
		/// </summary>
		class SpotLightGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SpotLightGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SpotLightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Handle transform
			const Reference<Transform> m_handle;

			// Some private suff resides here...
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SpotLightGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SpotLightGizmo>();
}
