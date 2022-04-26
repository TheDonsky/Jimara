#include "GizmoCreator.h"


namespace Jimara {
	namespace Editor {
		GizmoCreator::GizmoCreator(GizmoScene::Context* context) : m_context(context) {
			std::unique_lock<std::recursive_mutex> lock(m_context->TargetContext()->UpdateLock());
			m_context->GizmoContext()->OnUpdate() += Callback(&GizmoCreator::OnUpdate, this);
			m_context->TargetContext()->OnComponentCreated() += Callback(&GizmoCreator::OnComponentCreated, this);
			m_context->Selection()->OnComponentSelected() += Callback(&GizmoCreator::OnComponentSelected, this);
			m_context->Selection()->OnComponentDeselected() += Callback(&GizmoCreator::OnComponentDeselected, this);
			RecreateGizmos();
		}

		GizmoCreator::~GizmoCreator() {
			std::unique_lock<std::recursive_mutex> lock(m_context->TargetContext()->UpdateLock());
			m_context->GizmoContext()->OnUpdate() -= Callback(&GizmoCreator::OnUpdate, this);
			m_context->TargetContext()->OnComponentCreated() -= Callback(&GizmoCreator::OnComponentCreated, this);
			m_context->Selection()->OnComponentSelected() -= Callback(&GizmoCreator::OnComponentSelected, this);
			m_context->Selection()->OnComponentDeselected() -= Callback(&GizmoCreator::OnComponentDeselected, this);
			ClearGizmos();
		}

		namespace {
			inline static bool IsInRootHeirarchy(Component* component) {
				if (component == nullptr) return false;
				Component* rootObject = component->Context()->RootObject();
				while (true) {
					component = component->Parent();
					if (component == rootObject) return true;
					else if (component == nullptr) return false;
				}
			}
		}


		void GizmoCreator::StoreComponentState(Component* component) {
			if (component == nullptr || component->Destroyed()) return;
			m_allComponents.Add(component);
			component->OnDestroyed() += Callback(&GizmoCreator::OnComponentDestroyed, this);
		}
		void GizmoCreator::EraseComponentState(Component* component) {
			if (component == nullptr) return;
			m_allComponents.Remove(component);
			component->OnDestroyed() -= Callback(&GizmoCreator::OnComponentDestroyed, this);
		}

		void GizmoCreator::UpdateGizmoStates() {
			static thread_local std::vector<Reference<Component>> componentsToUpdate;
			{
				componentsToUpdate.clear();
				for (const Reference<Component>& component : m_componentsToUpdate)
					componentsToUpdate.push_back(component);
				m_componentsToUpdate.clear();
			}
			for (const Reference<Component>& component : componentsToUpdate) {
				bool destroyed = (component->Destroyed() || (!m_allComponents.Contains(component)));
				bool selected = ((!destroyed) && m_context->Selection()->Contains(component));
				
				auto shouldDrawGizmo = [&](const Gizmo::ComponentConnection& connection) -> bool {
					if (destroyed) return false;
					Gizmo::Filter filter = connection.FilterFlags();
					if (selected && (filter & Gizmo::FilterFlag::CREATE_IF_SELECTED) != 0) return true;
					else if ((!selected) && (filter & Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) != 0) return true;
					// __TODO__: Make sure CREATE_IF_PARENT_SELECTED works
					// __TODO__: Make sure CREATE_IF_CHILD_SELECTED works
					// __TODO__: Make sure CREATE_CHILD_GIZMOS_IF_SELECTED works
					else return false;
				};
				
				const Gizmo::ComponentConnectionSet::ConnectionList& connections = m_connections->GetGizmosFor(component);
				for (size_t connectionId = 0; connectionId < connections.Size(); connectionId++) {
					const Gizmo::ComponentConnection& connection = connections[connectionId];
					const bool isUnified = (connection.FilterFlags() & Gizmo::FilterFlag::CREATE_ONE_FOR_ALL_TARGETS) != 0;
					if (shouldDrawGizmo(connection)) {
						if (isUnified) {
							// __TODO__: Find unified gizmo and add target to it
						}
						else {
							// __TODO__: Create new gizmo with target
						}
					}
					else {
						// __TODO__: Remove gizmo instance or remove from multi-target gizmo
					}
				}
				// __TODO__: Implement this crap!
			}
			componentsToUpdate.clear();
		}

		void GizmoCreator::OnUpdate() {
			Reference<const Gizmo::ComponentConnectionSet> connections = Gizmo::ComponentConnectionSet::Current();
			if (connections != m_connections) RecreateGizmos();
			else UpdateGizmoStates();
		}
		void GizmoCreator::OnComponentCreated(Component* component) {
			if (!IsInRootHeirarchy(component)) return;
			StoreComponentState(component);
			m_componentsToUpdate.insert(component);
		}
		void GizmoCreator::OnComponentDestroyed(Component* component) {
			if (component == nullptr) return;
			EraseComponentState(component);
			m_componentsToUpdate.insert(component);
		}
		void GizmoCreator::OnComponentSelected(Component* component) {
			if (component == nullptr) return;
			if (!m_allComponents.Contains(component))
				StoreComponentState(component);
			m_componentsToUpdate.insert(component);
		}
		void GizmoCreator::OnComponentDeselected(Component* component) {
			if (component == nullptr) return;
			m_componentsToUpdate.insert(component);
		}

		void GizmoCreator::ClearGizmos() {
			// Forget about components and destroy their gizmos:
			{
				std::vector<Reference<Component>> all(m_allComponents.Data(), m_allComponents.Data() + m_allComponents.Size());
				for (Reference<Component> component : all) {
					EraseComponentState(component);
					m_componentsToUpdate.insert(component);
				}
			}
			UpdateGizmoStates();
			
			// __TODO__: Destroy no-target gizmos

			m_connections = nullptr;
		}
		void GizmoCreator::RecreateGizmos() {
			ClearGizmos();

			// Refetch connections:
			m_connections = Gizmo::ComponentConnectionSet::Current();
			if (m_connections == nullptr) {
				m_context->GizmoContext()->Log()->
					Fatal("GizmoCreator::RecreateGizmos - Gizmo::ComponentConnectionSet::Current() returned false! [File: ", __FILE__ "; Line: ", __LINE__, "]");
				return;
			}

			// Collect all components:
			{
				static thread_local std::vector<Component*> all;
				all.clear();
				m_context->TargetContext()->RootObject()->GetComponentsInChildren<Component>(all, true);
				for (Component* component : all) {
					StoreComponentState(component);
					m_componentsToUpdate.insert(component);
				}
				all.clear();
			}

			// __TODO__: Create no-target gizmos


			// Create relevant gizmos:
			UpdateGizmoStates();
		}
	}
}
