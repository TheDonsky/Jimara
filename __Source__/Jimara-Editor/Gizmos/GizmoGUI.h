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
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="priority"> Priority for sorting </param>
				inline Drawer(float priority = 0.0f) : m_priority(priority) {}

				/// <summary>
				/// Lets a Drawer draw on SceneView
				/// </summary>
				virtual void OnDrawGizmoGUI() = 0;

			private:
				// Priority for sorting
				const float m_priority;

				// GizmoGUI has access to OnDrawGizmoGUI() and piority
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
