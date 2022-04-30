#pragma once
#include "../Gizmo.h"
#include "../Handles/Handle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::DirectionalLightGizmo);

		class DirectionalLightGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			DirectionalLightGizmo(Scene::LogicContext* context);

			virtual ~DirectionalLightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			const Reference<Handle> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::DirectionalLightGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::DirectionalLightGizmo>();
}
