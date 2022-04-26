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
			component->OnParentChanged() += Callback(&GizmoCreator::OnComponentParentChanged, this);
		}
		void GizmoCreator::EraseComponentState(Component* component) {
			if (component == nullptr) return;
			m_allComponents.Remove(component);
			component->OnDestroyed() -= Callback(&GizmoCreator::OnComponentDestroyed, this);
			component->OnParentChanged() -= Callback(&GizmoCreator::OnComponentParentChanged, this);
		}

		void GizmoCreator::UpdateGizmoStates() {
			static thread_local std::vector<Reference<Component>> componentsToUpdate;
			{
				componentsToUpdate.clear();
				std::unordered_set<Component*> addedParents;
				for (const Reference<Component>& component : m_componentsToUpdate) {
					if (component == nullptr) continue;

					Component* parent = component->Parent();
					while (parent != nullptr) {
						if (m_componentsToUpdate.find(parent) != m_componentsToUpdate.end()) break;
						else parent = parent->Parent();
					}
					if (parent != nullptr) continue;

					component->GetComponentsInChildren<Component>(componentsToUpdate);
					parent = component;
					while (parent != nullptr && parent != m_context->TargetContext()->RootObject()) {
						if (addedParents.find(parent) != addedParents.end()) break;
						addedParents.insert(parent);
						componentsToUpdate.push_back(parent);
						parent = parent->Parent();
					}
				}
				m_componentsToUpdate.clear();
			}

			std::unordered_set<Component*> parentsOfSelectedComponents;
			for (const Reference<Component>& component : componentsToUpdate) {
				if (!m_context->Selection()->Contains(component)) continue;
				Component* node = component;
				while (true) {
					const Gizmo::ComponentConnectionSet::ConnectionList& connections = m_connections->GetGizmosFor(node);
					bool canMoveUpwords = (connections.Size() <= 0);
					for (size_t i = 0; i < connections.Size(); i++)
						if ((connections[i].FilterFlags() & Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED) != 0) {
							canMoveUpwords = true;
							break;
						}
					if (!canMoveUpwords) break;
					node = component->Parent();
					if (node == nullptr) break;
					parentsOfSelectedComponents.insert(node);
				}
			}

			for (const Reference<Component>& component : componentsToUpdate) {
				if (component == nullptr) continue;

				bool destroyed = (component->Destroyed() || (!m_allComponents.Contains(component)));
				bool selected = ((!destroyed) && m_context->Selection()->Contains(component));
				
				auto shouldDrawGizmo = [&](const Gizmo::ComponentConnection& connection) -> bool {
					if (destroyed) return false;
					Gizmo::Filter filter = connection.FilterFlags();
					auto parentSelected = [&]() {
						if ((filter & Gizmo::FilterFlag::CREATE_IF_PARENT_SELECTED) == 0) return false;
						Component* parent = component->Parent();
						while (parent != nullptr) {
							const Gizmo::ComponentConnectionSet::ConnectionList& connections = m_connections->GetGizmosFor(parent);
							bool canMoveUpwords = (connections.Size() <= 0);
							for (size_t i = 0; i < connections.Size(); i++)
								if ((connections[i].FilterFlags() & Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED) != 0) {
									canMoveUpwords = true;
									break;
								}
							if (!canMoveUpwords) return false;
							else if (m_context->Selection()->Contains(parent)) return true;
						}
						return false;
					};
					auto childSelected = [&]() {
						if ((filter & Gizmo::FilterFlag::CREATE_IF_CHILD_SELECTED) == 0) return false;
						else return parentsOfSelectedComponents.find(component) != parentsOfSelectedComponents.end();
					};
					if (selected && (filter & Gizmo::FilterFlag::CREATE_IF_SELECTED) != 0) return true;
					else if ((!selected) && (filter & Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED) != 0) return true;
					else if (parentSelected()) return true;
					else if (childSelected()) return true;
					else return false;
				};
				
				const Gizmo::ComponentConnectionSet::ConnectionList& connections = m_connections->GetGizmosFor(component);
				for (size_t connectionId = 0; connectionId < connections.Size(); connectionId++) {
					const Gizmo::ComponentConnection& connection = connections[connectionId];
					const bool isUnified = (connection.FilterFlags() & Gizmo::FilterFlag::CREATE_ONE_FOR_ALL_TARGETS) != 0;
					if (shouldDrawGizmo(connection)) {
						auto createNewGizmo = [&]() -> Reference<Gizmo> {
							Reference<Gizmo> gizmo = connection.CreateGizmo(m_context->GizmoContext());
							if (gizmo == nullptr || gizmo->Destroyed()) {
								m_context->GizmoContext()->Log()->Error("GizmoCreator::UpdateGizmoStates - Failed to create gizmo for '", connection.GizmoType().Name(), "'!");
								return nullptr;
							}
							else {
								m_allGizmos.insert(gizmo);
								m_componentGizmos[component][connection.GizmoType()] = gizmo;
								return gizmo;
							}
						};
						
						auto getOrCreateGizmo = [&]() -> Reference<Gizmo> {
							if (isUnified) {
								decltype(m_combinedGizmoInstances)::iterator it = m_combinedGizmoInstances.find(connection.GizmoType());
								if (it == m_combinedGizmoInstances.end()) {
									Reference<Gizmo> gizmo = createNewGizmo();
									if (gizmo == nullptr) return nullptr;
									m_combinedGizmoInstances[connection.GizmoType()] = gizmo;
									return gizmo;
								}
								else return it->second;
							}
							else {
								auto componentGizmoSetIt = m_componentGizmos.find(component);
								if (componentGizmoSetIt == m_componentGizmos.end()) return createNewGizmo();
								auto componentGizmoIt = componentGizmoSetIt->second.find(connection.GizmoType());
								if (componentGizmoIt == componentGizmoSetIt->second.end()) return createNewGizmo();
								else return componentGizmoIt->second;
							}
						};

						const Reference<Gizmo> gizmo = getOrCreateGizmo();
						if (gizmo != nullptr)
							gizmo->AddTarget(component);
					}
					else {
						auto componentGizmoSetIt = m_componentGizmos.find(component);
						if (componentGizmoSetIt == m_componentGizmos.end()) continue;
						auto componentGizmoIt = componentGizmoSetIt->second.find(connection.GizmoType());
						if (componentGizmoIt == componentGizmoSetIt->second.end()) continue;
						const Reference<Gizmo> gizmo = componentGizmoIt->second;
						if (gizmo == nullptr) {
							m_context->GizmoContext()->Log()->Error("GizmoCreator::UpdateGizmoStates - Internal error: null gizmo stored! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							continue;
						}
						componentGizmoSetIt->second.erase(componentGizmoIt);
						if (componentGizmoSetIt->second.empty())
							m_componentGizmos.erase(componentGizmoSetIt);
						
						gizmo->RemoveTarget(component);
						if (gizmo->TargetCount() <= 0 && (connection.FilterFlags() & Gizmo::FilterFlag::CREATE_WITHOUT_TARGET) == 0) {
							m_allGizmos.erase(gizmo);
							if (isUnified) m_combinedGizmoInstances.erase(connection.GizmoType());
							if (!gizmo->Destroyed()) gizmo->Destroy();
						}
					}
				}
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
		void GizmoCreator::OnComponentParentChanged(Component::ParentChangeInfo changeInfo) {
			auto track = [&](Component* component) {
				if (component != nullptr) m_componentsToUpdate.insert(component);
			};
			track(changeInfo.component);
			track(changeInfo.oldParent);
			track(changeInfo.newParent);
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
			// Destroy all existing gizmos:
			{
				for (const Reference<Gizmo>& gizmo : m_allGizmos)
					gizmo->Destroy();
				m_allGizmos.clear();
				m_combinedGizmoInstances.clear();
				m_componentGizmos.clear();
			}

			// Forget about components and destroy their gizmos:
			{
				std::vector<Reference<Component>> all(m_allComponents.Data(), m_allComponents.Data() + m_allComponents.Size());
				for (Reference<Component> component : all)
					EraseComponentState(component);
				m_allComponents.Clear();
				m_componentsToUpdate.clear();
			}

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

			// Create no-target gizmos:
			{
				const Gizmo::ComponentConnectionSet::ConnectionList& targetlessGizmos = m_connections->GetTargetlessGizmos();
				for (size_t i = 0; i < targetlessGizmos.Size(); i++) {
					const Gizmo::ComponentConnection& connection = targetlessGizmos[i];
					if (m_combinedGizmoInstances.find(connection.GizmoType()) != m_combinedGizmoInstances.end()) continue;
					const Reference<Gizmo> gizmo = connection.CreateGizmo(m_context->GizmoContext());
					if (gizmo == nullptr || gizmo->Destroyed()) {
						m_context->GizmoContext()->Log()->Error("GizmoCreator::RecreateGizmos - Failed to create gizmo for '", connection.GizmoType().Name(), "'!");
						continue;
					}
					m_allGizmos.insert(gizmo);
					m_combinedGizmoInstances[connection.GizmoType()] = gizmo;
				}
			}

			// Create relevant gizmos:
			UpdateGizmoStates();
		}
	}
}
