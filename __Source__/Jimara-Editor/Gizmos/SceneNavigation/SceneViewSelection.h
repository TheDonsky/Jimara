#pragma once
#include "../Gizmo.h"
#include "../GizmoViewportHover.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about SceneViewSelection </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SceneViewSelection);

		/// <summary>
		/// Basic 'global' gizmo, responsible for scene view selection
		/// </summary>
		class SceneViewSelection : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewSelection(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewSelection();

		protected:
			/// <summary> Update function </summary>
			virtual void Update()override;

		private:
			// Hover query instance
			Reference<GizmoViewportHover> m_hover;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewSelection>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewSelection>();
}
