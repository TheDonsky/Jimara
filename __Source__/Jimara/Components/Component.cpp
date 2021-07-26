#include "Component.h"
#include "Transform.h"
#include <vector>


namespace Jimara {
	Component::Component(SceneContext* context, const std::string_view& name) : m_context(context), m_name(name), m_parent(nullptr) { m_context->ComponentInstantiated(this); }

	Component::Component(Component* parent, const std::string_view& name) : Component(parent->Context(), name) { SetParent(parent); }

	Component::~Component() { Destroy(); }

	namespace {
		class BaseComponentSerializer : public virtual ComponentSerializer {
		public:
			inline BaseComponentSerializer() 
				: ItemSerializer("Component", "Base component"), ComponentSerializer("Jimara/Component") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override { 
				return Object::Instantiate<Component>(parent, "Component"); 
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				const std::string_view(*getName)(void*) = [](void* target) -> const std::string_view { return ((Component*)target)->Name(); };
				void(*setName)(const std::string_view&, void*) = [](const std::string_view& value, void* target) { ((Component*)target)->Name() = value; };
				static const Reference<const Serialization::StringViewSerializer> nameSerializer = Serialization::StringViewSerializer::Create(
					"Name", "Component name", Function(getName), Callback(setName));
				recordElement(Serialization::SerializedObject(nameSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const BaseComponentSerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry BASE_COMPONENT_SERIALIZER;
	}

	Reference<const ComponentSerializer> Component::GetSerializer()const { return BaseComponentSerializer::Instance(); }

	JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(Component, { BASE_COMPONENT_SERIALIZER = BaseComponentSerializer::Instance(); }, { BASE_COMPONENT_SERIALIZER = nullptr; });

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




	ComponentSerializer::ComponentSerializer(const std::string_view& path) : m_path(path) {}

	const std::string& ComponentSerializer::Path()const { return m_path; }

	namespace {
		static std::recursive_mutex& ComponentSerializerRegistryLock() {
			static std::recursive_mutex lock;
			return lock;
		}

		typedef std::unordered_map<Reference<const ComponentSerializer>, size_t> ComponentSerializerRegistry;
		static ComponentSerializerRegistry& SerializerRegistry() {
			static ComponentSerializerRegistry registry;
			return registry;
		}

		typedef std::unordered_map<std::string, std::unordered_set<Reference<const ComponentSerializer>>> ComponentSerializerIndex;
		static ComponentSerializerIndex& SerializerPathIndex() {
			static ComponentSerializerIndex index;
			return index;
		}
	}

	ComponentSerializer::RegistryEntry::RegistryEntry(const Reference<const ComponentSerializer>& componentSerializer) { (*this) = componentSerializer.operator->(); }

	ComponentSerializer::RegistryEntry::~RegistryEntry() { (*this) = nullptr; }

	Reference<const ComponentSerializer> ComponentSerializer::RegistryEntry::Serializer()const {
		std::unique_lock<SpinLock> serializerLock(m_serializerLock);
		Reference<const ComponentSerializer> serializer(m_serializer);
		return serializer;
	}

	void ComponentSerializer::RegistryEntry::operator=(const ComponentSerializer* componentSerializer) {
		std::unique_lock<std::recursive_mutex> lock(ComponentSerializerRegistryLock());
		if (m_serializer == componentSerializer) return;
		if (m_serializer != nullptr) {
			ComponentSerializerRegistry::iterator it = SerializerRegistry().find(m_serializer);
			if (it == SerializerRegistry().end()) return;
			it->second--;
			if (it->second == 0) {
				SerializerRegistry().erase(it);
				std::unordered_set<Reference<const ComponentSerializer>>& set = SerializerPathIndex()[m_serializer->TargetName()];
				set.erase(m_serializer);
				if (set.empty()) SerializerPathIndex().erase(m_serializer->Path());
			}
		}
		{
			std::unique_lock<SpinLock> serializerLock(m_serializerLock);
			m_serializer = componentSerializer;
		}
		if (m_serializer != nullptr) {
			ComponentSerializerRegistry::iterator it = SerializerRegistry().find(m_serializer);
			if (it == SerializerRegistry().end()) {
				SerializerRegistry()[m_serializer] = 1;
				SerializerPathIndex()[m_serializer->TargetName()].insert(m_serializer);
			}
			else it->second++;
		}
	}

	void ComponentSerializer::RegistryEntry::GetAll(Callback<const ComponentSerializer*> recordEntry) {
		std::unique_lock<std::recursive_mutex> lock(ComponentSerializerRegistryLock());
		static std::queue<Reference<const ComponentSerializer>> serializers;
		size_t count = SerializerRegistry().size();
		for (ComponentSerializerRegistry::const_iterator it = SerializerRegistry().begin(); it != SerializerRegistry().end(); ++it) 
			serializers.push(it->first);
		for (size_t i = 0; i < count; i++) {
			Reference<const ComponentSerializer> ref = serializers.front();
			serializers.pop();
			recordEntry(ref);
		}
	}

	Reference<const ComponentSerializer> ComponentSerializer::RegistryEntry::FindWithPath(const std::string& path) {
		std::unique_lock<std::recursive_mutex> lock(ComponentSerializerRegistryLock());
		ComponentSerializerIndex::const_iterator it = SerializerPathIndex().find(path);
		if (it == SerializerPathIndex().end()) return nullptr;
		else return *it->second.begin();
	}
}
