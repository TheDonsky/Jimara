#include "RegistryEntry.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	struct RegistryEntry::Helpers {
		inline static void RefreshEntryLater(Object* selfPtr) {
			RegistryEntry* self = dynamic_cast<RegistryEntry*>(selfPtr);
			assert(self != nullptr);
			RefreshEntry(self);
		}

		static void OnComponentInvalidated(RegistryEntry* self, Component*) {
			Deactivate(self);
			if (self->m_scheduledRefreshCount.fetch_add(1u) <= 0u)
				self->Context()->ExecuteAfterUpdate(Callback<Object*>(Helpers::RefreshEntryLater), self);
		}

		static void OnComponentParentHierarchyChanged(RegistryEntry* self, Component::ParentChangeInfo) {
			if (self->m_scheduledRefreshCount.fetch_add(1u) <= 0u)
				self->Context()->ExecuteAfterUpdate(Callback<Object*>(Helpers::RefreshEntryLater), self);
		}

		inline static void Activate(RegistryEntry* self) {
			if (self->m_configuration.registry.reference == nullptr)
				Deactivate(self);
			else if (self->m_configuration.key.type == KeyType::STRING) {
				assert(self->m_configuration.key.object == nullptr);
				self->m_entry = Registry::Entry(
					self->m_configuration.registry.reference, 
					self->m_configuration.key.string, 
					self->m_configuration.storedObject.reference);
			}
			else self->m_entry = Registry::Entry(
				self->m_configuration.registry.reference, 
				self->m_configuration.key.object, 
				self->m_configuration.storedObject.reference);
		}

		inline static void Deactivate(RegistryEntry* self) {
			self->m_entry = Registry::Entry();
		}

		inline static void RefreshEntry(RegistryEntry* self) {
			self->m_scheduledRefreshCount.store(0u);
			std::unique_lock<std::mutex> lock(self->m_updateLock);

			// Manage subscriptions:
			auto unsubscribeFrom = [&](Component* elem) {
				elem->OnDestroyed() -=
					Callback<Component*>(Helpers::OnComponentInvalidated, self);
				elem->OnParentChanged() -=
					Callback<Component::ParentChangeInfo>(Helpers::OnComponentParentHierarchyChanged, self);
			};
			auto subscribeToOnDestroyedFn = [&](Component* elem) {
				if (elem == self)
					return;
				elem->OnDestroyed() += Callback<Component*>(Helpers::OnComponentInvalidated, self);
				self->m_subscribedComponents.Push(elem);
			};
			auto subscribeToOnDestroyedOrParentChangedFn = [&](Component* elem) {
				if (elem != self)
					elem->OnDestroyed() += Callback<Component*>(Helpers::OnComponentInvalidated, self);
				elem->OnParentChanged() +=
					Callback<Component::ParentChangeInfo>(Helpers::OnComponentParentHierarchyChanged, self);
				self->m_subscribedComponents.Push(elem);
			};
			{
				for (size_t i = 0u; i < self->m_subscribedComponents.Size(); i++)
					unsubscribeFrom(self->m_subscribedComponents[i]);
				self->m_subscribedComponents.Clear();
			}

			// If destroyed, we just do a cleanup and exit:
			if (self->Destroyed()) {
				self->m_configuration.registry.reference = nullptr;
				self->m_configuration.key.object = nullptr;
				self->m_configuration.storedObject.reference = nullptr;
				Deactivate(self);
				return;
			}

			// Establish registry:
			{
				if (self->m_configuration.registry.type == RegistryType::NONE)
					self->m_configuration.registry.reference = nullptr;
				else if (self->m_configuration.registry.type == RegistryType::PARENT) {
					self->m_configuration.registry.reference = self->GetComponentInParents<Registry>();
					Component* ptr = self;
					while (ptr != nullptr) {
						subscribeToOnDestroyedOrParentChangedFn(ptr);
						if (ptr == (Object*)self->m_configuration.registry.reference.operator->())
							break;
						else ptr = ptr->Parent();
					}
				}
				else if (self->m_configuration.registry.type == RegistryType::CUSTOM) {
					Component* registry = dynamic_cast<Component*>(self->m_configuration.registry.reference.operator->());
					if (registry != nullptr)
						subscribeToOnDestroyedFn(registry);
				}
				else if (self->m_configuration.registry.type == RegistryType::SCENE_WIDE)
					self->m_configuration.registry.reference = Registry::ContextWide(self->Context());
				else if (self->m_configuration.registry.type == RegistryType::GLOBAL)
					self->m_configuration.registry.reference = Registry::Global();
				else self->m_configuration.registry.reference = nullptr;
			}

			// Fix key:
			{
				if (self->m_configuration.key.type == KeyType::STRING)
					self->m_configuration.key.object = nullptr;
				else {
					Component* key = dynamic_cast<Component*>(self->m_configuration.key.object.operator->());
					if (key != nullptr)
						subscribeToOnDestroyedFn(key);
				}
			}

			// Update stored object:
			{
				if (self->m_configuration.storedObject.type == StoredObjectType::PARENT) {
					Component* parent = self->Parent();
					self->m_configuration.storedObject.reference = parent;
					if (parent != nullptr) {
						subscribeToOnDestroyedOrParentChangedFn(self);
						subscribeToOnDestroyedFn(parent);
					}
				}
				else if (self->m_configuration.storedObject.type == StoredObjectType::SELF)
					self->m_configuration.storedObject.reference = self;
			}

			// Make sure we do not hold destroyed references:
			{
				auto checkDestroyed = [&](auto& ref) {
					Component* elem = dynamic_cast<Component*>(ref.operator->());
					if ((elem == nullptr) || (!elem->Destroyed()))
						return;
					unsubscribeFrom(elem);
					ref = nullptr;
				};
				checkDestroyed(self->m_configuration.registry.reference);
				checkDestroyed(self->m_configuration.key.object);
				checkDestroyed(self->m_configuration.storedObject.reference);
			}

			// Update actual entry:
			if (self->ActiveInHeirarchy())
				Activate(self);
			else Deactivate(self);
		}

		static void OnThisEntryDestroyed(Component* selfPtr) {
			RegistryEntry* entry = dynamic_cast<RegistryEntry*>(selfPtr);
			assert(entry != nullptr);
			RefreshEntry(entry);
			selfPtr->OnDestroyed() -= Callback<Component*>(Helpers::OnThisEntryDestroyed);
		}
	};

	RegistryEntry::RegistryEntry(Component* parent, const std::string_view& name) : Component(parent, name) {
		OnDestroyed() += Callback<Component*>(Helpers::OnThisEntryDestroyed);
	}

	RegistryEntry::~RegistryEntry() {
		OnDestroyed() -= Callback<Component*>(Helpers::OnThisEntryDestroyed);
		assert(m_configuration.registry.reference == nullptr);
		assert(m_configuration.key.object == nullptr);
		assert(m_configuration.storedObject.reference == nullptr);
		assert(m_subscribedComponents.Size() <= 0u);
	}

	RegistryEntry::Configuration RegistryEntry::GetConfiguration()const {
		Configuration result;
		{
			std::unique_lock<std::mutex> lock(m_updateLock);
			result = m_configuration;
		}
		if (result.registry.type != RegistryType::CUSTOM)
			result.registry.reference = nullptr;
		if (result.key.type != KeyType::OBJECT)
			result.key.object = nullptr;
		else if (result.key.type != KeyType::STRING)
			result.key.string = "";
		if (result.storedObject.type != StoredObjectType::CUSTOM)
			result.storedObject.reference = nullptr;
		return result;
	}

	void RegistryEntry::Configure(const Configuration& settings) {
		{
			std::unique_lock<std::mutex> lock(m_updateLock);
			m_configuration = settings;
		}
		Helpers::RefreshEntry(this);
	}

	void RegistryEntry::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		Configuration settings;
		{
			std::unique_lock<std::mutex> lock(m_updateLock);
			settings = m_configuration;
		}
		{
			static const Configuration::Serializer serializer("Configuration", "Configuration");
			serializer.GetFields(recordElement, &settings);
		}
		Configure(settings);
	}

	const Object* RegistryEntry::RegistryTypeEnumAttribute() {
		static const Reference<const Serialization::EnumAttribute<std::underlying_type_t<RegistryType>>> attribute =
			Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<RegistryType>>>(false,
				"NONE", RegistryType::NONE,
				"PARENT", RegistryType::PARENT,
				"CUSTOM", RegistryType::CUSTOM,
				"SCENE_WIDE", RegistryType::SCENE_WIDE,
				"GLOBAL", RegistryType::GLOBAL);
		return attribute;
	}

	const Object* RegistryEntry::KeyTypeEnumAttribute() {
		static const Reference<const Serialization::EnumAttribute<std::underlying_type_t<KeyType>>> attribute =
			Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<KeyType>>>(false,
				"OBJECT", KeyType::OBJECT,
				"STRING", KeyType::STRING);
		return attribute;
	}

	const Object* RegistryEntry::StoredObjectTypeEnumAttribute() {
		static const Reference<const Serialization::EnumAttribute<std::underlying_type_t<StoredObjectType>>> attribute =
			Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<StoredObjectType>>>(false,
				"PARENT", StoredObjectType::PARENT,
				"SELF", StoredObjectType::SELF,
				"CUSTOM", StoredObjectType::CUSTOM);
		return attribute;
	}

	void RegistryEntry::OnComponentEnabled() {
		Helpers::RefreshEntry(this);
	}
	void RegistryEntry::OnComponentDisabled() {
		Helpers::RefreshEntry(this);
	}

	void RegistryEntry::EntrySetConfiguration::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, EntrySetConfiguration* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			// Expose Registry:
			{
				RegistryType type = target->registry.type;
				JIMARA_SERIALIZE_FIELD(type, "Registry Type", "Type of the registry", RegistryTypeEnumAttribute());
				if (type != target->registry.type) {
					target->registry.type = type;
					target->registry.reference = nullptr;
				}
				if (target->registry.type == RegistryType::CUSTOM)
					JIMARA_SERIALIZE_FIELD(target->registry.reference, "Registry", "Registry to store object in");
			}

			// Expose key:
			{
				KeyType type = target->key.type;
				JIMARA_SERIALIZE_FIELD(type, "Key Type", "Registry key type", KeyTypeEnumAttribute());
				if (type != target->key.type) {
					target->key.type = type;
					target->key.object = nullptr;
				}
				if (target->key.type == KeyType::OBJECT)
					JIMARA_SERIALIZE_FIELD(target->key.object, "Key", "Key object");
				else if (target->key.type == KeyType::STRING)
					JIMARA_SERIALIZE_FIELD(target->key.string, "Key", "Key string");
			}
		};
	}

	void RegistryEntry::Configuration::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Configuration* target)const {
		// Expose Registry & Key:
		{
			static const EntrySetConfiguration::Serializer baseSerializer("EntrySetConfiguration", "EntrySetConfiguration");
			baseSerializer.GetFields(recordElement, (EntrySetConfiguration*)target);
		}
		// Expose object:
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			StoredObjectType type = target->storedObject.type;
			JIMARA_SERIALIZE_FIELD(type, "Stored Object", "Stored Object type", StoredObjectTypeEnumAttribute());
			if (type != target->storedObject.type) {
				target->storedObject.type = type;
				target->storedObject.reference = nullptr;
			}
			if (target->storedObject.type == StoredObjectType::CUSTOM)
				JIMARA_SERIALIZE_FIELD(target->storedObject.reference, "Item", "Stored Object");
		};
	}

	template<> void TypeIdDetails::GetParentTypesOf<RegistryEntry>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<RegistryEntry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<RegistryEntry> serializer("Jimara/Scene/RegistryEntry", "Registry Entry");
		report(&serializer);
	}
}
