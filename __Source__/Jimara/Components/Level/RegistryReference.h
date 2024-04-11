#pragma once
#include "RegistryEntry.h"


namespace Jimara {
	/// <summary>
	/// Component that retrieves a reference from a registry
	/// </summary>
	/// <typeparam name="Type"> Retrieved object type </typeparam>
	template<typename Type>
	class RegistryReference : public virtual Component {
	public:
		/// <summary> Type of the registry to read element from </summary>
		using RegistryType = RegistryEntry::RegistryType;

		/// <summary> Type of the registry key </summary>
		using KeyType = RegistryEntry::KeyType;

		/// <summary> Registry reference configuration settings </summary>
		using Configuration = RegistryEntry::EntrySetConfiguration;

		/// <summary> Constructor </summary>
		inline RegistryReference();

		/// <summary> Virtual destructor </summary>
		inline virtual ~RegistryReference() = 0;

		/// <summary> Object reference extracted from the registry </summary>
		inline Reference<Type> StoredObject()const;

		/// <summary> Retrieves current configuration of RegistryReference </summary>
		inline Configuration GetConfiguration()const;

		/// <summary>
		/// Sets configuration of RegistryReference
		/// </summary>
		/// <param name="settings"> Configuration settings </param>
		inline void Configure(const Configuration& settings);

		/// <summary> Event, that gets fired whenever the entry gets invalidated </summary>
		inline Event<RegistryReference*>& OnReferenceDirty() { return m_onDirty; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	private:
		// Lock for configuration changes
		mutable std::mutex m_updateLock;

		// Current configuration
		Configuration m_configuration;

		// Components, events of which the RegistryEntry is subscribed to
		Stacktor<Reference<Component>, 4u> m_subscribedComponents;

		// Current entries
		Reference<Registry::Entries> m_subscribedEntries;

		// Some updates happen asynchronously; this value is used to determine if there's anything scheduled or not
		std::atomic<size_t> m_scheduledRefreshCount = 0u;

		// Lock for current retrieved reference
		mutable SpinLock m_storedObjectReferenceLock;

		// Entry set for StoredObject retrieval
		Reference<Registry::Entries> m_storedEntries;

		// Last value of StoredObject
		mutable Reference<Object> m_storedObject;

		// True, if StoredObject is dirty and re-fetch has to happen
		mutable volatile bool m_dirty = true;

		// Invoked, whenever the entry gets invalidated
		EventInstance<RegistryReference*> m_onDirty;

		// Most of implementation is hidden behind this
		struct Helpers;
	};

	/// <summary>
	/// Type details for RegistryReference<Type>
	/// </summary>
	/// <typeparam name="Type"> Referenced type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<RegistryReference<Type>> {
		/// <summary>
		/// Reports parent type (Component)
		/// </summary>
		/// <param name="reportParentType"> Type will be reported through this </param>
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
		}
		/// <summary> Reports attributes </summary>
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};





	template<typename Type>
	struct RegistryReference<Type>::Helpers {
		inline static void RefreshLater(Object* selfPtr) {
			RegistryReference* self = dynamic_cast<RegistryReference*>(selfPtr);
			assert(self != nullptr);
			RefreshReference(self);
			self->m_onDirty(self);
		}

		static void OnComponentInvalidated(RegistryReference* self, Component*) {
			{
				const std::unique_lock<SpinLock> lock(self->m_storedObjectReferenceLock);
				self->m_storedEntries = nullptr;
				self->m_storedObject = nullptr;
				self->m_dirty = true;
			}
			if (self->m_scheduledRefreshCount.fetch_add(1u) <= 0u)
				self->Context()->ExecuteAfterUpdate(Callback<Object*>(Helpers::RefreshLater), self);
		}

		static void OnComponentParentHierarchyChanged(RegistryReference* self, Component::ParentChangeInfo) {
			if (self->m_scheduledRefreshCount.fetch_add(1u) <= 0u)
				self->Context()->ExecuteAfterUpdate(Callback<Object*>(Helpers::RefreshLater), self);
		}

		static void OnEntriesDirty(RegistryReference* self, Registry::Entries* entries) {
			{
				const std::unique_lock<SpinLock> lock(self->m_storedObjectReferenceLock);
				self->m_storedObject = nullptr;
				self->m_dirty = true;
			}
			self->m_onDirty(self);
		}

		static void RefreshReference(RegistryReference* self) {
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
			auto setSubscribedEntries = [&](Registry::Entries* entries) {
				if (self->m_subscribedEntries == entries)
					return;
				if (self->m_subscribedEntries != nullptr)
					self->m_subscribedEntries->OnDirty() -= Callback<Registry::Entries*>(Helpers::OnEntriesDirty, self);
				self->m_subscribedEntries = entries;
				if (self->m_subscribedEntries != nullptr)
					self->m_subscribedEntries->OnDirty() += Callback<Registry::Entries*>(Helpers::OnEntriesDirty, self);
				{
					const std::unique_lock<SpinLock> referenceLock(self->m_storedObjectReferenceLock);
					self->m_storedEntries = self->m_subscribedEntries;
					self->m_storedObject = nullptr;
					self->m_dirty = true;
				}
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
				setSubscribedEntries(nullptr);
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
			}

			// Subscribe to entries:
			{
				Reference<Registry::Entries> entries;
				if (self->m_configuration.registry.reference != nullptr) {
					if (self->m_configuration.key.type == KeyType::STRING)
						entries = self->m_configuration.registry.reference->GetEntries(self->m_configuration.key.string);
					else entries = self->m_configuration.registry.reference->GetEntries(self->m_configuration.key.object);
				}
				setSubscribedEntries(entries);
			}
		}

		static void OnThisReferenceDestroyed(Component* selfPtr) {
			RegistryReference* reference = dynamic_cast<RegistryReference*>(selfPtr);
			assert(reference != nullptr);
			RefreshReference(reference);
			selfPtr->OnDestroyed() -= Callback<Component*>(Helpers::OnThisReferenceDestroyed);
		}

		static void OnThisReferenceParentHierarchyChanged(typename Component::ParentChangeInfo info) {
			RegistryReference* reference = dynamic_cast<RegistryReference*>(info.component);
			assert(reference != nullptr);
			RefreshReference(reference);
			reference->m_onDirty(reference);
		}
	};

	template<typename Type>
	inline RegistryReference<Type>::RegistryReference() {
		OnDestroyed() += Callback<Component*>(Helpers::OnThisReferenceDestroyed);
		OnParentChanged() += Callback<Component::ParentChangeInfo>(Helpers::OnThisReferenceParentHierarchyChanged);
	}

	template<typename Type>
	inline RegistryReference<Type>::~RegistryReference() {
		OnDestroyed() -= Callback<Component*>(Helpers::OnThisReferenceDestroyed);
		OnParentChanged() -= Callback<Component::ParentChangeInfo>(Helpers::OnThisReferenceParentHierarchyChanged);
		assert(m_configuration.registry.reference == nullptr);
		assert(m_configuration.key.object == nullptr);
		assert(m_subscribedEntries == nullptr);
		assert(m_storedEntries == nullptr);
		assert(m_storedObject == nullptr);
		assert(m_subscribedComponents.Size() <= 0u);
	}

	template<typename Type>
	inline Reference<Type> RegistryReference<Type>::StoredObject()const {
		const std::unique_lock<SpinLock> lock(m_storedObjectReferenceLock);
		if (Destroyed()) {
			assert(m_storedEntries == nullptr);
			assert(m_storedObject == nullptr);
			return nullptr;
		}
		else if (m_dirty) {
			assert(m_storedObject == nullptr);
			m_dirty = false;
			if (m_storedEntries != nullptr) {
				Registry::Reader reader(m_storedEntries);
				for (size_t i = 0u; i < reader.ItemCount(); i++) {
					Type* elem = dynamic_cast<Type*>(reader.Item(i));
					if (elem == nullptr)
						continue;
					m_storedObject = elem;
					break;
				}
			}
		}
		const Reference<Type> item = m_storedObject;
		return item;
	}

	template<typename Type>
	inline typename RegistryReference<Type>::Configuration RegistryReference<Type>::GetConfiguration()const {
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
		return result;
	}

	template<typename Type>
	inline void RegistryReference<Type>::Configure(const Configuration& settings) {
		{
			std::unique_lock<std::mutex> lock(m_updateLock);
			m_configuration = settings;
		}
		Helpers::RefreshReference(this);
		m_onDirty(this);
	}

	template<typename Type>
	inline void RegistryReference<Type>::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);

		Configuration configuration;
		{
			std::unique_lock<std::mutex> lock(m_updateLock);
			configuration = m_configuration;
		}
		{
			static const Configuration::Serializer serializer("Configuration", "Configuration");
			serializer.GetFields(recordElement, &configuration);
		}
		Configure(configuration);
	}
}
