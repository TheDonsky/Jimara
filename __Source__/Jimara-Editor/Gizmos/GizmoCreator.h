#pragma once
#include "GizmoScene.h"

namespace Jimara {
	namespace Editor {
		class GizmoCreator : public virtual Object {
		public:
			GizmoCreator(GizmoScene::Context* context);

			virtual ~GizmoCreator();

		private:
			const Reference<GizmoScene::Context> m_context;

			void OnUpdate();
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
			void OnComponentSelected(Component* component);
			void OnComponentDeselected(Component* component);
		};
	}
}
