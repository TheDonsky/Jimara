#include "Component.h"
#include "Transform.h"
#include "../Data/Serialization/Helpers/SerializerMacros.h"
#include <vector>
#include <algorithm>


namespace Jimara {
	class Component::StrongReferenceProvider : public virtual WeaklyReferenceable::StrongReferenceProvider, public virtual BulkAllocated {
	private:
		mutable SpinLock m_lock;
		mutable Reference<WeaklyReferenceable> m_component;

	public:
		inline StrongReferenceProvider(WeaklyReferenceable* component) : m_component(component) {}

		inline virtual ~StrongReferenceProvider() {}

		virtual Reference<WeaklyReferenceable> RestoreStrongReference() final override {
			Reference<WeaklyReferenceable> rv;
			{
				const std::unique_lock<SpinLock> lock(m_lock);
				rv = m_component;
			}
			return rv;
		}

		void Clear() {
			const std::unique_lock<SpinLock> lock(m_lock);
			m_component = nullptr;
		}

	protected:
		virtual void OnOutOfScope()const final override {
			BulkAllocated::OnOutOfScope();
		}
	};


	Component::Component(SceneContext* context, const std::string_view& name) 
		: m_context(context), m_name(name), m_parent(nullptr)
		, m_weakObj(BulkAllocated::Allocate<StrongReferenceProvider>(this)) {
		if (m_context == nullptr) 
			throw std::runtime_error("Component::Component - Context not provided!");
		m_context->ComponentCreated(this);
	}

	Component::Component(Component* parent, const std::string_view& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() {
		if (!Destroyed())
			Context()->Log()->Error("Component::~Component - Destructor called without destroying the component with Destroy() call; (Direct deletion of components is unsafe!)");
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Component>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Component>(
			"Component", "Jimara/Component", "Base Component");
		report(factory);
	}

	std::string& Component::Name() { return m_name; }

	const std::string& Component::Name()const { return m_name; }

	bool Component::Enabled()const { return (m_flags & static_cast<uint8_t>(Flags::ENABLED)) != 0; }

	void Component::SetEnabled(bool enabled) {
		if (((m_flags & static_cast<uint8_t>(Flags::ENABLED)) != 0) == enabled) return;
		else if (enabled) m_flags |= static_cast<uint8_t>(Flags::ENABLED);
		else m_flags &= (~static_cast<uint8_t>(Flags::ENABLED));
		if (!Destroyed())
			m_context->ComponentStateDirty(this, false);
	}

	bool Component::ActiveInHierarchy()const {
		if (Destroyed()) return false;
		const Component* component = this;
		const Component* sceneRootObject = Context()->RootObject();
		while (component != nullptr && component != sceneRootObject) {
			if (!component->Enabled()) return false;
			component = component->Parent();
		}
		return true;
	}

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
		inline static void EraseChildAt(std::vector<Component*>& children, size_t childId, SetChildIdFn setChildId) {
#ifndef NDEBUG
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
		Component* oldParent = m_parent;

		// Let's make sure we are not trying to parent a destroyed component...
		if (Destroyed())
			Context()->Log()->Error("Component::SetParent - Trying to add a destroyed component as a child!");
		else if (newParent->Destroyed())
			Context()->Log()->Error("Component::SetParent - Trying to add a child to a destroyed component!");

		// Let us make sure both components are from the same context
		if (m_context != newParent->m_context)
			throw std::runtime_error("Component::SetParent - Parent has to be from the same context as the child!");
		
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
			std::vector<Component*>& children = (((Component*)m_parent)->m_children);
#ifndef NDEBUG
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
		m_context->ComponentStateDirty(this, true);
		{
			ParentChangeInfo parentChangeInfo;
			parentChangeInfo.component = this;
			parentChangeInfo.oldParent = oldParent;
			parentChangeInfo.newParent = newParent;
			m_onParentChanged(parentChangeInfo);
		}
	}

	size_t Component::IndexInParent()const { return m_childId; }

	void Component::SetIndexInParent(size_t index) {
		if (Parent() == nullptr) return;
		std::vector<Component*>& children = (((Component*)m_parent)->m_children);
#ifndef NDEBUG
		assert(children[m_childId] == this);
#endif
		if (index >= children.size())
			index = children.size() - 1;
#ifndef NDEBUG
		assert(index < children.size());
#endif
		if (m_childId < index) {
			while (m_childId < index) {
				std::swap(children[m_childId], children[m_childId + 1]);
				children[m_childId]->m_childId.store(m_childId);
				m_childId++;
#ifndef NDEBUG
				assert(children[m_childId] == this);
#endif
			}
		}
		else while (m_childId > index) {
			std::swap(children[m_childId], children[m_childId - 1]);
			children[m_childId]->m_childId.store(m_childId);
			m_childId--;
#ifndef NDEBUG
			assert(children[m_childId] == this);
#endif
		}
	}

	void Component::ClearParent() { SetParent(RootObject()); }

	size_t Component::ChildCount()const { return m_children.size(); }

	Component* Component::GetChild(size_t index)const { return m_children[index]; }

	void Component::SortChildren(const Function<bool, Component*, Component*>& less) {
		std::sort(m_children.begin(), m_children.end(), [&](const Reference<Component>& a, const Reference<Component>& b) -> bool { return less(a, b); });
		for (size_t i = 0; i < m_children.size(); i++)
			m_children[i]->m_childId.store(i);
	}

	Event<Component::ParentChangeInfo>& Component::OnParentChanged()const { return m_onParentChanged; }
	
	Transform* Component::GetTransform() { return GetComponentInParents<Transform>(); }

	const Transform* Component::GetTransform()const { return GetComponentInParents<Transform>(); }

	void Component::Destroy() {
		// Let us ignore this call if the component is already destroyed...
		if (Destroyed()) {
			Context()->Log()->Error("Component::Destroy - Attempting to doubly destroy a component!");
			return;
		}

		// Set active flag to false and invoke OnComponentDisabled()
		m_flags &= (~static_cast<uint8_t>(Flags::ENABLED));
		OnComponentDisabled(); // Let us guarantee OnComponentDisabled() always gets called from here!

		// Make sure the component can no longer be weakly-referenced:
		if (m_weakObj != nullptr) {
			Reference<StrongReferenceProvider> provider = m_weakObj;
			provider->Clear();
			m_weakObj = nullptr;
		}

		// Set destroyed flag to make sure nobody adds random children
		m_flags |= static_cast<uint8_t>(Flags::DESTROYED);

		// But what about children?
		static thread_local std::vector<Component*> children;
		size_t firstChild = children.size();
		for (size_t i = 0; i < m_children.size(); i++)
			children.push_back(m_children[i]);
		if (m_children.size() > 0) {
			size_t i = children.size() - 1;
			while (true) {
				children[i]->Destroy();
				if (i == firstChild) break;
				i--;
			}
		}
		children.resize(firstChild);
		
		// Let's tell the parents...
		const bool hadParent = (m_parent != nullptr);
		if (hadParent) {
			AddRef();
			std::vector<Component*>& children = (((Component*)m_parent)->m_children);
#ifndef NDEBUG
			assert(children[m_childId] == this);
#endif
			EraseChildAt(children, m_childId, [](Component* child, size_t index) { child->m_childId = index; });
			m_parent = nullptr;
			m_childId = 0;
		}

		// Signal listeners that this object is no longer valid
		m_context->ComponentDestroyed(this);
		OnComponentDestroyed();
		m_onDestroyed(this);
		m_onDestroyed.Clear();
		if (hadParent) ReleaseRef();
	}

	Event<Component*>& Component::OnDestroyed()const { return m_onDestroyed; }

	bool Component::Destroyed()const { return (m_flags & static_cast<uint8_t>(Flags::DESTROYED)) != 0; }

	void Component::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Enabled, SetEnabled, "Enabled", "Component enabled/disabled toggle");
			JIMARA_SERIALIZE_FIELD(Name(), "Name", "Component name");
		};
	}

	void Component::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		// Enable/Disable
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create(
				"Enabled", "If true, upon invokation, component will be enabled");
			report(Serialization::SerializedCallback::Create<bool>::From(
				"SetEnabled", Callback<bool>(&Component::SetEnabled, this), serializer));
		}

		// Set Name:
		{
			static const auto serializer = Serialization::DefaultSerializer<std::string>::Create(
				"Name", "Name for the component");
			typedef void (*SetFn)(Component*, const std::string&);
			static const SetFn setFn = [](Component* self, const std::string& name) { self->Name() = name; };
			report(Serialization::SerializedCallback::Create<const std::string&>::From(
				"SetName", Callback<const std::string&>(setFn, this), serializer));
		}

		// Destroy:
		{
			report(Serialization::SerializedCallback::Create<>::From(
				"Destroy", Callback<>(&Component::Destroy, this)));
		}
	}



	void Component::FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder) {
		ClearWeakReferenceHolder(holder);
#ifndef NDEBUG
		assert(this != nullptr);
#endif
		if (Destroyed())
			return;
		holder = m_weakObj;
	}

	void Component::ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder) {
		if (holder == nullptr)
			return;
		holder = nullptr;
	}
}
