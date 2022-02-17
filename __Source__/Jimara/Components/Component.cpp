#include "Component.h"
#include "Transform.h"
#include <vector>


namespace Jimara {
	Component::Component(SceneContext* context, const std::string_view& name) : m_context(context), m_name(name), m_parent(nullptr) { 
		if (m_context == nullptr) 
			throw std::runtime_error("Component::Component - Context not provided!");
		m_context->ComponentCreated(this);
	}

	Component::Component(Component* parent, const std::string_view& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() {
		if (!Destroyed())
			Context()->Log()->Error("Component::~Component - Destructor called without destroying the component with Destroy() call; (Direct deletion of components is unsafe!)");
	}

	namespace {
		class BaseComponentSerializer : public ComponentSerializer::Of<Component> {
		public:
			inline BaseComponentSerializer() 
				: ItemSerializer("Jimara/Component", "Base component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
				static const Reference<const FieldSerializer> enabledStateSerializer = Serialization::BoolSerializer::For<Component>(
					"Enabled", "Component enabled/disabled toggle",
					[](Component* target) -> bool { return target->Enabled(); },
					[](const bool& value, Component* target) { target->SetEnabled(value); });
				recordElement(enabledStateSerializer->Serialize(target));

				static const Reference<const FieldSerializer> nameSerializer = Serialization::StringViewSerializer::For<Component>(
					"Name", "Component name",
					[](Component* target) -> std::string_view { return target->Name(); },
					[](const std::string_view& value, Component* target) { target->Name() = value; });
				recordElement(nameSerializer->Serialize(target));
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

	bool Component::Enabled()const { return (m_flags & static_cast<uint8_t>(Flags::ENABLED)) != 0;; }

	void Component::SetEnabled(bool enabled) {
		if (enabled) m_flags |= static_cast<uint8_t>(Flags::ENABLED);
		else m_flags &= (~static_cast<uint8_t>(Flags::ENABLED));
		if (!Destroyed())
			m_context->ComponentEnabledStateDirty(this);
	}

	bool Component::ActiveInHeirarchy()const {
		if (Destroyed()) return false;
		const Component* component = this;
		const Component* rootObject = RootObject();
		while (component != nullptr && component != rootObject) {
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
			std::vector<Reference<Component>>& children = (((Component*)m_parent)->m_children);
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
		m_context->ComponentEnabledStateDirty(this);
		NotifyParentChange();
	}

	size_t Component::IndexInParent()const { return m_childId; }

	void Component::SetIndexInParent(size_t index) {
		if (Parent() == nullptr) return;
		std::vector<Reference<Component>>& children = (((Component*)m_parent)->m_children);
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

	Event<const Component*>& Component::OnParentChanged()const { return m_onParentChanged; }
	
	Transform* Component::GetTransfrom() { return GetComponentInParents<Transform>(); }

	const Transform* Component::GetTransfrom()const { return GetComponentInParents<Transform>(); }

	void Component::Destroy() {
		// Let us ignore this call if the component is already destroyed...
		if (Destroyed()) {
			Context()->Log()->Error("Component::Destroy - Attempting to doubly destroy a component!");
			return;
		}

		// Set active flag to false and invoke OnComponentDisabled()
		m_flags &= (~static_cast<uint8_t>(Flags::ENABLED));
		OnComponentDisabled(); // Let us guarantee OnComponentDisabled() always gets called from here!

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
			std::vector<Reference<Component>>& children = (((Component*)m_parent)->m_children);
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

	void Component::NotifyParentChange()const {
		m_onParentChanged(this);
		m_referenceBuffer.clear();
		GetComponentsInChildren<Component>(m_referenceBuffer, true);
		for (size_t i = 0; i < m_referenceBuffer.size(); i++)
			m_referenceBuffer[i]->m_onParentChanged(m_referenceBuffer[i]);
	}





	ComponentSerializer::Set::Set(const std::map<std::string_view, Reference<const ComponentSerializer>>& typeIndexToSerializer)
		: m_serializers([&]()-> std::vector<Reference<const ComponentSerializer>> {
		std::vector<Reference<const ComponentSerializer>> list;
		for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
			list.push_back(it->second);
		return list;
			}()), m_typeNameToSerializer([&]() -> std::unordered_map<std::string_view, const ComponentSerializer*> {
				std::unordered_map<std::string_view, const ComponentSerializer*> map;
				for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
					map[it->second->TargetComponentType().Name()] = it->second;
				return map;
				}()), m_typeIndexToSerializer([&]() -> std::unordered_map<std::type_index, const ComponentSerializer*> {
					std::unordered_map<std::type_index, const ComponentSerializer*> map;
					for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
						map[it->second->TargetComponentType().TypeIndex()] = it->second;
					return map;
					}()) {
		assert(m_serializers.size() == typeIndexToSerializer.size());
		assert(m_typeNameToSerializer.size() == typeIndexToSerializer.size());
		assert(m_typeIndexToSerializer.size() == typeIndexToSerializer.size());
	}

	Reference<ComponentSerializer::Set> ComponentSerializer::Set::All() {
		static SpinLock allLock;
		static Reference<ComponentSerializer::Set> all;
		static Reference<RegisteredTypeSet> registeredTypes;

		std::unique_lock<SpinLock> lock(allLock);
		{
			const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
			if (currentTypes == registeredTypes) return all;
			else registeredTypes = currentTypes;
		}
		std::map<std::string_view, Reference<const ComponentSerializer>> typeIndexToSerializer;
		for (size_t i = 0; i < registeredTypes->Size(); i++) {
			void(*checkAttribute)(decltype(typeIndexToSerializer)*, const Object*) = [](decltype(typeIndexToSerializer)* serializers, const Object* attribute) {
				const ComponentSerializer* componentSerializer = dynamic_cast<const ComponentSerializer*>(attribute);
				if (componentSerializer != nullptr) 
					(*serializers)[componentSerializer->TargetComponentType().Name()] = componentSerializer;
			};
			registeredTypes->At(i).GetAttributes(Callback<const Object*>(checkAttribute, &typeIndexToSerializer));
		}
		all = new Set(typeIndexToSerializer);
		all->ReleaseRef();
		return all;
	}

	const ComponentSerializer* ComponentSerializer::Set::FindSerializerOf(const std::string_view& typeName)const {
		decltype(m_typeNameToSerializer)::const_iterator it = m_typeNameToSerializer.find(typeName);
		if (it == m_typeNameToSerializer.end()) return nullptr;
		else return it->second;
	}

	const ComponentSerializer* ComponentSerializer::Set::FindSerializerOf(const std::type_index& typeIndex)const {
		decltype(m_typeIndexToSerializer)::const_iterator it = m_typeIndexToSerializer.find(typeIndex);
		if (it == m_typeIndexToSerializer.end()) return nullptr;
		else return it->second;
	}

	const ComponentSerializer* ComponentSerializer::Set::FindSerializerOf(const TypeId& typeId)const {
		return FindSerializerOf(typeId.TypeIndex());
	}

	const ComponentSerializer* ComponentSerializer::Set::FindSerializerOf(const Component* component)const {
		if (component == nullptr) return nullptr;
		else return FindSerializerOf(typeid(*component));
	}

	size_t ComponentSerializer::Set::Size()const {
		return m_serializers.size();
	}

	const ComponentSerializer* ComponentSerializer::Set::At(size_t index)const {
		return m_serializers[index];
	}

	const ComponentSerializer* ComponentSerializer::Set::operator[](size_t index)const {
		return At(index);
	}
}
