#pragma once
#include "../../../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::HDRILightGizmo);

		/// <summary>
		/// Gizmo for a HDRI light
		/// </summary>
		class HDRILightGizmo : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			HDRILightGizmo(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~HDRILightGizmo();

		protected:
			/// <summary> Updates gizmo </summary>
			virtual void Update()override;

		private:
			// Underlying renderer
			Reference<RenderStack::Renderer> m_renderer;

			// Private stuff resides in here...
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::HDRILightGizmo>(const Callback<const Object*>& report);
}
