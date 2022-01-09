#include "Component.h"
#include "Transform.h"
#include <vector>


namespace Jimara {
	Component::Component(SceneContext* context, const std::string_view& name) : m_context(context), m_name(name), m_parent(nullptr) { 
		if (m_context == nullptr) 
			throw std::runtime_error("Component::Component - Context not provided!");
#ifndef USE_REFACTORED_SCENE
		m_context->ComponentInstantiated(this); 
#else
		m_context->ComponentCreated(this);
#endif
	}

	Component::Component(Component* parent, const std::string_view& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() { Destroy(); }

	namespace {
		class BaseComponentSerializer : public ComponentSerializer::Of<Component> {
		public:
			inline BaseComponentSerializer() 
				: ItemSerializer("Jimara/Component", "Base component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
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

	bool Component::Enabled()const { return m_enabled; }

	void Component::SetEnabled(bool enabled) {
		m_enabled = enabled;
		// __TODO__: Notify the context that component has been disabled or enabled
#ifdef USE_REFACTORED_SCENE
		m_context->ComponentEnabledStateDirty(this);
#endif
	}

	bool Component::ActiveInHeirarchy()const {
		const Component* component = this;
		const Component* rootObject = RootObject();
		while (component != nullptr && component != rootObject) {
			if (!component->m_enabled.load()) return false;
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
#ifdef USE_REFACTORED_SCENE
		// __TODO__: Maybe be careful about this call... (can be invoked from destructor and that is not OK)
		m_context->ComponentDestroyed(this);
#endif
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
