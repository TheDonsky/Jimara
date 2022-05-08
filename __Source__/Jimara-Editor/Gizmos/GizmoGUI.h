#pragma once
#include "../Environment/EditorScene.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// __TODO__: Document this crap! 
		/// </summary>
		class GizmoGUI : public virtual Object {
		public:
			class Drawer : public virtual Component {
			protected:
				virtual void OnDrawGizmoGUI() = 0;

				// __TODO__: Add a few parameters for ordering

				friend class GizmoGUI;
			};

			GizmoGUI(Scene::LogicContext* gizmoContext);

			virtual ~GizmoGUI();

			void Draw();

		private:
			const Reference<Scene::LogicContext> m_gizmoContext;

			ObjectSet<Drawer> m_drawers;
			std::vector<Reference<Drawer>> m_drawerList;

			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
		};
	}
}
