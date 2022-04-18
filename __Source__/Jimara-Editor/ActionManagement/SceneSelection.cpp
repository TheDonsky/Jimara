#include "SceneSelection.h"

namespace Jimara {
	namespace Editor {
		SceneSelection::SceneSelection(Scene::LogicContext* context) : m_context(context) {}

		SceneSelection::~SceneSelection() { DeselectAll(); }

		void SceneSelection::Select(Component* component) {
			if (component == nullptr) return;
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			if (component->Destroyed() || component->Context() != m_context || component == m_context->RootObject()) {
				Deselect(component);
				return;
			}
			else if (m_selection.find(component) != m_selection.end()) return;
			else {
				Component* parent = component->Parent();
				while (parent != m_context->RootObject()) {
					if (parent == nullptr) return;
					else parent = parent->Parent();
				}
			}
			m_selection.insert(component);
			component->OnDestroyed() += Callback(&SceneSelection::OnComponentDestroyed, this);
			m_onComponentSelected(component);
		}

		void SceneSelection::SelectSubtree(Component* root) {
			if (root == nullptr) return;
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			Select(root);
			std::vector<Component*> children = root->RootObject()->GetComponentsInChildren<Component>(true);
			Select(children.data(), children.size());
		}

		void SceneSelection::SelectAll() {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			SelectSubtree(m_context->RootObject());
		}

		void SceneSelection::Deselect(Component* component) {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			decltype(m_selection)::const_iterator it = m_selection.find(component);
			if (it == m_selection.end()) return;
			it->operator->()->OnDestroyed() -= Callback(&SceneSelection::OnComponentDestroyed, this);
			m_selection.erase(it);
			m_onComponentDeselected(component);
		}

		void SceneSelection::DeselectSubtree(Component* root) {
			if (root == nullptr) return;
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			Deselect(root);
			std::vector<Component*> children = root->RootObject()->GetComponentsInChildren<Component>(true);
			Deselect(children.data(), children.size());
		}

		void SceneSelection::DeselectAll() {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			std::vector<Reference<Component>> selection = Current();
			Deselect(selection.data(), selection.size());
		}

		std::vector<Reference<Component>> SceneSelection::Current()const {
			std::vector<Reference<Component>> rv;
			Iterate([&](Component* v) { rv.push_back(v); });
			return rv;
		}

		Event<Component*>& SceneSelection::OnComponentSelected()const { 
			return m_onComponentSelected; 
		}

		Event<Component*>& SceneSelection::OnComponentDeselected()const { 
			return m_onComponentDeselected; 
		}

		void SceneSelection::OnComponentDestroyed(Component* component) { Deselect(component); }
	}
}
