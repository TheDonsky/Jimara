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
		class SceneViewSelection : 
			public virtual Gizmo, 
			public virtual GizmoGUI::Drawer,
			public virtual Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo context </param>
			SceneViewSelection(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneViewSelection();

		protected:
			/// <summary> Draws selection rectangle </summary>
			virtual void OnDrawGizmoGUI()override;

			/// <summary> Update function </summary>
			virtual void Update()override;

		private:
			// Hover query instance
			Reference<GizmoViewportHover> m_hover;

			// Selected area start
			std::optional<Vector2> m_clickStart;

			// Thread block for parallel selection processing
			ThreadBlock m_processingBlock;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewSelection>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewSelection>();
}
