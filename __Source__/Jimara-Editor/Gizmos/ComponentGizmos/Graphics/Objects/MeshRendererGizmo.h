#pragma once
#include "../../../Gizmo.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::MeshRendererGizmo);

		class MeshRendererGizmo : public virtual Gizmo {
		public:
			MeshRendererGizmo(Scene::LogicContext* context);

			virtual ~MeshRendererGizmo();

		private:
			const Reference<MeshRenderer> m_wireframeRenderer;

			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MeshRendererGizmo>(const Callback<const Object*>& report);
}
