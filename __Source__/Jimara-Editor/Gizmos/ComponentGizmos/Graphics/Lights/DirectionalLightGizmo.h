#pragma once
#include "../../../Gizmo.h"
#include "../../../Handles/Handle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::DirectionalLightGizmo);

		/// <summary>
		/// Gizmo for a directional light
		/// </summary>
		class DirectionalLightGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			DirectionalLightGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~DirectionalLightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Handle transform
			const Reference<Transform> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::DirectionalLightGizmo>(const Callback<const Object*>& report);
}
