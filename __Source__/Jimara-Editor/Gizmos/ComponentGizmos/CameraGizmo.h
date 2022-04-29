#pragma once
#include "../Gizmo.h"
#include "../Handles/Handle.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::CameraGizmo);

		class CameraGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			CameraGizmo(Scene::LogicContext* context);

			virtual ~CameraGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			const Reference<Handle> m_handle;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::CameraGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::CameraGizmo>();
}
