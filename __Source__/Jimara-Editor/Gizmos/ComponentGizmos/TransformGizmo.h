#pragma once
#include "../Gizmo.h"
#include "../Handles/Handle.h"


namespace Jimara {
	namespace Editor {
		JIMARA_REGISTER_TYPE(Jimara::Editor::TransformGizmo);

		class TransformGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			TransformGizmo(Scene::LogicContext* context);

			virtual ~TransformGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			const Reference<Transform> m_transform;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformGizmo>();
}
