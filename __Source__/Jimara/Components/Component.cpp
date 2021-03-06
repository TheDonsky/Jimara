#include "Component.h"
#include "Transform.h"
#include <vector>


namespace Jimara {
	Component::Component(SceneContext* context, const std::string& name) : m_context(context), m_name(name), m_parent(nullptr) { }

	Component::Component(Component* parent, const std::string& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() { Destroy(); }

	std::string& Component::Name() { return m_name; }

	const std::string& Component::Name()const { return m_name; }

	SceneContext* Component::Context()const { return m_context; }

	Component* Component::RootObject() { 
		Component* root = this;
		while (root->m_parent != nullptr)
			root = root->m_parent;
		return root;
	}

	const Component* Component::RootObject()const { 
		const Component* root = this;
		while (root->m_parent != nullptr)
			root = root->m_parent;
		return root;
	}

	Component* Component::Parent()const { return m_parent; }

	void Component::SetParent(Component* newParent) {
		// First, let us make sure, we don't end up orphaned after this operation:
		if (newParent == nullptr) newParent = RootObject();
		if (m_parent == newParent || newParent == this) return;
		
		// To make sure, the parent is not the only one holding the reference:
		Reference<Component> self(this);
		Reference<Component> parent(newParent);

		// This lets us avoid circular dependencies:
		{
			Component* ptr = newParent->m_parent;
			while (ptr != nullptr) {
				if (ptr == this) {
					newParent->SetParent(m_parent);
					break;
				}
				else ptr = ptr->m_parent;
			}
		}

		// Main reparenting operation:
		if (m_parent != nullptr) ((Component*)m_parent)->m_children.erase(this);
		m_parent = newParent;
		if (m_parent != nullptr) newParent->m_children.insert(this);

		// Inform heirarchy change listeners:
		NotifyParentChange();
	}

	void Component::ClearParent() { SetParent(RootObject()); }

	Event<const Component*>& Component::OnParentChanged()const { return m_onParentChanged; }
	
	Transform* Component::GetTransfrom() { return GetComponentInParents<Transform>(); }

	const Transform* Component::GetTransfrom()const { return GetComponentInParents<Transform>(); }

	void Component::Destroy() {
		// Signal listeners that this object is no longer valid (we may actually prefer to keep the call after child Destroy() calls, but whatever...)
		m_onDestroyed(this);

		// But what about children?
		std::vector<Reference<Component>> children;
		for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
			children.push_back(*it);
		for (size_t i = 0; i < children.size(); i++)
			children[i]->Destroy();
		
		// Let's tell the parents...
		const bool hadParent = (m_parent != nullptr);
		if (hadParent) {
			AddRef();
			((Component*)m_parent)->m_children.erase(this);
			m_parent = nullptr;
		}

		// Just in case... We won't get wrecked the second time :)
		m_onDestroyed.Clear();
		if (hadParent) ReleaseRef();
	}

	Event<const Component*>& Component::OnDestroyed()const { return m_onDestroyed; }

	void Component::NotifyParentChange()const {
		m_onParentChanged(this);
		m_referenceBuffer.clear();
		GetComponentsInChildren<Component>(m_referenceBuffer, true);
		for (size_t i = 0; i < m_referenceBuffer.size(); i++)
			m_referenceBuffer[i]->m_onParentChanged(m_referenceBuffer[i]);
	}
}
