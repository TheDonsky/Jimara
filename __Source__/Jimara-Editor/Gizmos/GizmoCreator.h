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
			
			ObjectSet<Component> m_allComponents;
			std::unordered_set<Reference<Component>> m_componentsToUpdate;

			std::unordered_set<Reference<Gizmo>> m_allGizmos;
			std::unordered_map<TypeId, Reference<Gizmo>> m_combinedGizmoInstances;
			std::unordered_map<Reference<Component>, std::unordered_map<TypeId, Reference<Gizmo>>> m_componentGizmos;
			
			void StoreComponentState(Component* component);
			void EraseComponentState(Component* component);

			void UpdateGizmoStates();

			void OnUpdate();
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
			void OnComponentParentChanged(Component::ParentChangeInfo changeInfo);
			void OnComponentSelected(Component* component);
			void OnComponentDeselected(Component* component);

			void ClearGizmos();
			void RecreateGizmos();
		};
	}
}
