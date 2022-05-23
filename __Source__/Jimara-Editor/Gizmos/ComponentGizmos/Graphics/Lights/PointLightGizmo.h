#pragma once
#include "../../../Gizmo.h"
#include "../../../Handles/Handle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::PointLightGizmo);

		class PointLightGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			PointLightGizmo(Scene::LogicContext* context);

			virtual ~PointLightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			const Reference<Handle> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::PointLightGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::PointLightGizmo>();
}
