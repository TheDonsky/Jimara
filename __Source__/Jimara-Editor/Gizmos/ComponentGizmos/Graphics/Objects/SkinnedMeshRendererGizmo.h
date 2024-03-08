#pragma once
#include "../../../Gizmo.h"
#include <Jimara/Components/GraphicsObjects/SkinnedMeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SkinnedMeshRendererGizmo);

		/// <summary>
		/// Gizmo for skinned mesh renderers
		/// </summary>
		class SkinnedMeshRendererGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			SkinnedMeshRendererGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SkinnedMeshRendererGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying renderer
			const Reference<SkinnedMeshRenderer> m_wireframeRenderer;

			// Bone (mirror) transforms
			std::vector<Reference<Transform>> m_bones;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SkinnedMeshRendererGizmo>(const Callback<const Object*>& report);
}
