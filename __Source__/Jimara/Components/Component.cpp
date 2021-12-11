#include "Component.h"
#include "Transform.h"
#include <vector>


namespace Jimara {
	Component::Component(SceneContext* context, const std::string_view& name) : m_context(context), m_name(name), m_parent(nullptr) { m_context->ComponentInstantiated(this); }

	Component::Component(Component* parent, const std::string_view& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() { Destroy(); }

	namespace {
		class BaseComponentSerializer : public virtual ComponentSerializer::Of<Component> {
		public:
			inline BaseComponentSerializer() 
				: ItemSerializer("Jimara/Component", "Base component") {}

			inline virtual void SerializeTarget(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
				static const Reference<const Serialization::StringViewSerializer> nameSerializer = Serialization::StringViewSerializer::Create<Component>(
					"Name", "Component name",
					[](Component* target) -> const std::string_view { return target->Name(); },
					[](const std::string_view& value, Component* target) { target->Name() = value; });
				recordElement(Serialization::SerializedObject(nameSerializer, target));
			}

			inline static const ComponentSerializer* Instance() {
				static const BaseComponentSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Component>(const Callback<const Object*>& report) { report(BaseComponentSerializer::Instance()); }

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

	namespace {
		template<typename SetChildIdFn>
		inline static void EraseChildAt(std::vector<Reference<Component>>& children, size_t childId, SetChildIdFn setChildId) {
#ifdef DEBUG
			assert(children.size() > childId);
#endif
			for (size_t i = (childId + 1); i < children.size(); i++) {
				Component* child = children[i];
				size_t prev = (i - 1);
				setChildId(child, prev);
				children[prev] = child;
			}
			children.pop_back();
		}
	}

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
		if (m_parent != nullptr) {
			std::vector<Reference<Component>>& children = (((Component*)m_parent)->m_children);
#ifdef DEBUG
			assert(children[m_childId] == this);
#endif
			EraseChildAt(children, m_childId, [](Component* child, size_t index) { child->m_childId = index; });
		}
		m_parent = newParent;
		if (m_parent != nullptr) {
			m_childId = newParent->m_children.size();
			newParent->m_children.push_back(this);
		}
		else m_childId = 0;

		// Inform heirarchy change listeners:
		NotifyParentChange();
	}

	void Component::ClearParent() { SetParent(RootObject()); }

	size_t Component::ChildCount()const { return m_children.size(); }

	Component* Component::GetChild(size_t index)const { return m_children[index]; }

	Event<const Component*>& Component::OnParentChanged()const { return m_onParentChanged; }
	
	Transform* Component::GetTransfrom() { return GetComponentInParents<Transform>(); }

	const Transform* Component::GetTransfrom()const { return GetComponentInParents<Transform>(); }

	void Component::Destroy() {
		// Signal listeners that this object is no longer valid (we may actually prefer to keep the call after child Destroy() calls, but whatever...)
		m_onDestroyed(this);

		// But what about children?
		std::vector<Reference<Component>> children;
		for (size_t i = 0; i < m_children.size(); i++)
			children.push_back(m_children[i]);
		for (size_t i = 0; i < children.size(); i++)
			children[i]->Destroy();
		
		// Let's tell the parents...
		const bool hadParent = (m_parent != nullptr);
		if (hadParent) {
			AddRef();
			std::vector<Reference<Component>>& children = (((Component*)m_parent)->m_children);
#ifdef DEBUG
			assert(children[m_childId] == this);
#endif
			EraseChildAt(children, m_childId, [](Component* child, size_t index) { child->m_childId = index; });
			m_parent = nullptr;
			m_childId = 0;
		}

		// Just in case... We won't get wrecked the second time :)
		m_onDestroyed.Clear();
		if (hadParent) ReleaseRef();
	}

	Event<Component*>& Component::OnDestroyed()const { return m_onDestroyed; }

	void Component::NotifyParentChange()const {
		m_onParentChanged(this);
		m_referenceBuffer.clear();
		GetComponentsInChildren<Component>(m_referenceBuffer, true);
		for (size_t i = 0; i < m_referenceBuffer.size(); i++)
			m_referenceBuffer[i]->m_onParentChanged(m_referenceBuffer[i]);
	}
}
