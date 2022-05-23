#pragma once
#include "../../../Gizmo.h"
#include <Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::MeshRendererGizmo);

		class MeshRendererGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			MeshRendererGizmo(Scene::LogicContext* context);

			virtual ~MeshRendererGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			const Reference<MeshRenderer> m_wireframeRenderer;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::MeshRendererGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::MeshRendererGizmo>();
}
