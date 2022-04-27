#pragma once
#include "GizmoScene.h"
#include "Gizmo.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Class, reponsible for Gizmo spowning
		/// </summary>
		class GizmoCreator : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Gizmo scene context </param>
			GizmoCreator(GizmoScene::Context* context);

			/// <summary> Virtual destructor </summary>
			virtual ~GizmoCreator();

		private:
			// Gizmo scene context
			const Reference<GizmoScene::Context> m_context;

			// Last known registered GizmoConnections
			Reference<const Gizmo::ComponentConnectionSet> m_connections;
			
			// All tracked components from target scene
			ObjectSet<Component> m_allComponents;

			// Set of components, that got selected/undelected/discovered/destroyed or otherwise need gizmo refresh
			std::unordered_set<Reference<Component>> m_componentsToUpdate;

			// All spawned gizmos
			std::unordered_set<Reference<Gizmo>> m_allGizmos;

			// Single/Shared instance gizmos, as well as no target ones
			std::unordered_map<TypeId, Reference<Gizmo>> m_combinedGizmoInstances;

			// Mapping from components to maps of their gizmos by type
			std::unordered_map<Reference<Component>, std::unordered_map<TypeId, Reference<Gizmo>>> m_componentGizmos;
			

			// Adds to m_allComponents and subscribes to lifecycle events
			void StoreComponentState(Component* component);

			// Removes from m_allComponents and unsubscribes to lifecycle events
			void EraseComponentState(Component* component);

			// Flushes m_allComponents
			void UpdateGizmoStates();

			// Invoked on each gizmo scene update
			void OnUpdate();

			// Lefecycle events
			void OnComponentCreated(Component* component);
			void OnComponentDestroyed(Component* component);
			void OnComponentParentChanged(Component::ParentChangeInfo changeInfo);

			// Selection events
			void OnComponentSelected(Component* component);
			void OnComponentDeselected(Component* component);

			// Clear/Recreate gizmos
			void ClearGizmos();
			void RecreateGizmos();
		};
	}
}
