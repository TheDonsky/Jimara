#include "Registry.h"


namespace Jimara {
	struct Registry::Helpers {
#pragma warning(disable: 4250)
		struct CachedEntries
			: public virtual Entries
			, public virtual Jimara::ObjectCache<Reference<const Object>>::StoredObject {
			inline CachedEntries() {}
			inline virtual ~CachedEntries() {}
		};

		struct EntryCache : public virtual Jimara::ObjectCache<Reference<const Object>> {
			inline Reference<Entries> GetEntries(const Object* key) {
				if (key == nullptr)
					return nullptr;
				return GetCachedOrCreate(key, false, [&]() {
					return Object::Instantiate<CachedEntries>();
					});
			}
		};

		struct StringKey : public virtual Jimara::ObjectCache<std::string>::StoredObject {
			inline StringKey() {}
			inline virtual ~StringKey() {}
		};

		struct StringKeyCache : public virtual Jimara::ObjectCache<std::string> {
			static Reference<const StringKey> GetKey(const std::string& key) {
				if (key.size() <= 0u)
					return nullptr;
				static StringKeyCache cache;
				return cache.GetCachedOrCreate(key, false, [&]() {
					return Object::Instantiate<StringKey>();
					});
			}
		};

		struct ContextWideRegistryStorage
			: public virtual Jimara::ObjectCache<Reference<const Object>>::StoredObject {
			const Reference<Registry> registry = Object::Instantiate<Registry>();
			inline ContextWideRegistryStorage() {}
			inline virtual ~ContextWideRegistryStorage() {}
		};

		struct ContextWideRegistryCache : public virtual Jimara::ObjectCache<Reference<const Object>> {
			static Reference<ContextWideRegistryStorage> Get(Jimara::SceneContext* context) {
				if (context == nullptr)
					return nullptr;
				static ContextWideRegistryCache cache;
				static std::mutex creationLock;
				std::unique_lock<std::mutex> lock(creationLock);
				return cache.GetCachedOrCreate(context, false, [&]() {
					Reference<ContextWideRegistryStorage> instance = Object::Instantiate<ContextWideRegistryStorage>();
					context->StoreDataObject(instance);
					return instance;
					});
			}
		};
#pragma warning(default: 4250)





		inline static bool ClearEntry(Entry* entry) {
			const Reference<Entries> entries = entry->m_entries;
			const Reference<Object> item = entry->m_storedObject;
			entry->m_storedObject = nullptr;
			entry->m_entries = nullptr;

			if (entries == nullptr || item == nullptr)
				return false;
			auto it = entries->m_entries.find(item);
			if (it == entries->m_entries.end())
				return false;
			it->second.registryEntries.erase(entry);
			if (it->second.registryEntries.size() > 0u)
				return false;

			const size_t index = it->second.index;
			entries->m_entries.erase(it);
			it = entries->m_entries.end();
			const size_t lastIndex = entries->m_objects.Size() - 1u;
			if (index < lastIndex) {
				entries->m_entries[entries->m_objects[lastIndex]].index = index;
				std::swap(entries->m_objects[lastIndex], entries->m_objects[index]);
			}
			entries->m_objects.Pop();
			return true;
		}

		inline static bool StoreEntry(Entry* entry) {
			if (entry->m_entries == nullptr || entry->m_storedObject == nullptr)
				return false;
			auto it = entry->m_entries->m_entries.find(entry->m_storedObject);
			if (it == entry->m_entries->m_entries.end()) {
				Entries::EntryInfo& info = entry->m_entries->m_entries[entry->m_storedObject];
				info.index = entry->m_entries->m_objects.Size();
				info.registryEntries.insert(entry);
				entry->m_entries->m_objects.Push(entry->m_storedObject);
				return true;
			}
			else {
				it->second.registryEntries.insert(entry);
				return false;
			}
		}

		template<typename OnEntryStoredFn>
		inline static void CopyEntry(Entry* dst, const Entry* src, const OnEntryStoredFn& onEntryStored) {
			if (dst == src)
				return;
			Reference<Entries> oldEntries;
			{
				std::unique_lock<Jimara::SpinLock> lockA((src < dst) ? src->m_lock : dst->m_lock);
				std::unique_lock<Jimara::SpinLock> lockB((src > dst) ? src->m_lock : dst->m_lock);
				if (dst->m_storedObject == src->m_storedObject && dst->m_entries == src->m_entries)
					return;
				if (dst->m_entries == src->m_entries) {
					if (src->m_entries == nullptr)
						return;
					Reference<Entries> entries = src->m_entries;
					std::unique_lock<std::shared_mutex> lock(entries->m_lock);
					if (Helpers::ClearEntry(dst))
						oldEntries = entries;
					dst->m_entries = entries;
					dst->m_storedObject = src->m_storedObject;
					if (Helpers::StoreEntry(dst))
						assert(false); // There should be no way to actually store new entry in this case!
					onEntryStored();
				}
				else {
					if (dst->m_entries != nullptr) {
						oldEntries = dst->m_entries;
						const bool didNotRemove = [&]() {
							std::unique_lock<std::shared_mutex> lock(oldEntries->m_lock);
							const bool removed = Helpers::ClearEntry(dst);
							return !removed;
						}();
						if (didNotRemove)
							oldEntries = nullptr;
					}
					if (src->m_entries != nullptr) {
						Reference<Entries> entries = src->m_entries;
						std::unique_lock<std::shared_mutex> lock(entries->m_lock);
						dst->m_entries = entries;
						dst->m_storedObject = src->m_storedObject;
						if (Helpers::StoreEntry(dst))
							assert(false); // There should be no way to actually store new entry in this case!
						onEntryStored();
					}
				}
			}
			if (oldEntries != nullptr)
				oldEntries->m_onDirty(oldEntries);
		}
	};





	Registry::Registry() : m_entryCache(Object::Instantiate<Helpers::EntryCache>()) {}

	Registry::~Registry() {}

	Registry* Registry::Global() {
		static Registry global;
		return &global;
	}

	Reference<Registry> Registry::ContextWide(Jimara::SceneContext* context) {
		const Reference<Helpers::ContextWideRegistryStorage> storage = Helpers::ContextWideRegistryCache::Get(context);
		if (storage == nullptr)
			return nullptr;
		else return storage->registry;
	}

	Reference<Registry::Entries> Registry::GetEntries(const Object* key) {
		return dynamic_cast<Helpers::EntryCache*>(m_entryCache.operator->())->GetEntries(key);
	}

	Reference<Registry::Entries> Registry::GetEntries(const std::string& key) {
		Reference<const Helpers::StringKey> stringKey = Helpers::StringKeyCache::GetKey(key);
		return GetEntries(stringKey);
	}





	Registry::Entries::Entries() {}

	Registry::Entries::~Entries() {}





	Registry::Entry::Entry() {}

	Registry::Entry::Entry(Registry* registry, const Object* key, Object* item)
		: Entry((registry == nullptr ? Global() : registry)->GetEntries(key), item) {}

	Registry::Entry::Entry(Registry* registry, const std::string& key, Object* item)
		: Entry((registry == nullptr ? Global() : registry)->GetEntries(key), item) {}

	Registry::Entry::Entry(const Reference<Entries>& entries, Object* item)
		: m_storedObject(item), m_entries(entries) {
		if (m_storedObject == nullptr || m_entries == nullptr)
			return;
		const bool added = [&]() {
			std::unique_lock<std::shared_mutex> lock(m_entries->m_lock);
			bool rv = Helpers::StoreEntry(this);
			return rv;
		}();
		if (added)
			m_entries->m_onDirty(m_entries);
	}

	Registry::Entry::Entry(const Entry& other) {
		(*this) = other;
	}

	Registry::Entry::Entry(Entry&& other)noexcept {
		(*this) = std::move(other);
	}

	Registry::Entry::~Entry() {
		auto tryRemove = [&]() {
			if (m_entries == nullptr)
				return false;
			std::unique_lock<std::shared_mutex> lock(m_entries->m_lock);
			const bool removed = Helpers::ClearEntry(this);
			return removed;
		};
		if (tryRemove())
			m_entries->m_onDirty(m_entries);
	}

	Registry::Entry& Registry::Entry::operator=(const Entry& other) {
		Helpers::CopyEntry(this, &other, [&]() {});
		return *this;
	}

	Registry::Entry& Registry::Entry::operator=(Entry&& other)noexcept {
		Helpers::CopyEntry(this, &other, [&]() { Helpers::ClearEntry(&other); });
		return *this;
	}





	Registry::Reader::Reader(Registry* registry, const Object* key)
		: Reader((registry == nullptr ? Global() : registry)->GetEntries(key)) {}

	Registry::Reader::Reader(Registry* registry, const std::string& key)
		: Reader((registry == nullptr ? Global() : registry)->GetEntries(key)) {}

	Registry::Reader::Reader(const Reference<const Entries>& entries)
		: m_entries(entries)
		, m_lock([&]() -> std::shared_mutex& {
		static std::shared_mutex fallback;
		return (entries == nullptr) ? fallback : entries->m_lock;
			}()) {
		m_itemCount = (m_entries == nullptr) ? 0u : m_entries->m_objects.Size();
	}

	Registry::Reader::~Reader() {}

	size_t Registry::Reader::ItemCount()const {
		return m_itemCount;
	}

	Object* Registry::Reader::Item(size_t index)const {
		return m_entries->m_objects[index];
	}
}
