#pragma once
#include "GizmoScene.h"
#include "Gizmo.h"

namespace Jimara {
	namespace Editor {
		class GizmoCreator : public virtual Object {
		public:
			GizmoCreator(GizmoScene::Context* context);

			virtual ~GizmoCreator();

		private:
			const Reference<GizmoScene::Context> m_context;

			Reference<const Gizmo::ComponentConnectionSet> m_connections;

			void OnUpdate();
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
			void OnComponentSelected(Component* component);
			void OnComponentDeselected(Component* component);

			void ClearGizmos();
			void RecreateGizmos();
		};
	}
}
