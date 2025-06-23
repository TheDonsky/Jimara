#pragma once
#include "../Object.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <cassert>


namespace Jimara {
	/// <summary>
	/// Cache for creating and reusing arbitrary objects
	/// </summary>
	/// <typeparam name="KeyType"> Type of the object identifier within the cache </typeparam>
	/// <typeparam name="KeyHasherType"> Hasher type for KeyType (normally, std::hash, but you can use overrides) </typeparam>
	template<typename KeyType, typename KeyHasherType = ::std::hash<KeyType>>
	class ObjectCache : public virtual Object {
	public:
		/// <summary>
		/// Object that can be stored in a cache of the given type
		/// </summary>
		class StoredObject : public virtual Object {
		public:
			/// <summary> Constructor </summary>
			inline StoredObject() : m_cacheKey(KeyType()) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~StoredObject() {}

		protected:
			/// <summary> Invoked, when Object goes out of scope </summary>
			inline virtual void OnOutOfScope()const override {
				if (m_cacheLock != nullptr) {
					std::shared_ptr<std::mutex> cacheLock = m_cacheLock;
					std::unique_lock<std::mutex> lock(*cacheLock);
					if (RefCount() > 0u || m_cache == nullptr)
						return;
					m_cache->m_cachedObjects.erase(m_cacheKey);
					m_cache = nullptr;
				}
				Object::OnOutOfScope();
			}

		private:
			// "Owner" cache
			std::shared_ptr<std::mutex> m_cacheLock;
			mutable Reference<ObjectCache> m_cache;

			// Storage key within the cache
			KeyType m_cacheKey;

			// Object cache has to access fields defined above, so it's a friend
			friend class ObjectCache;
		};


		/// <summary> Virtual destructor for permanent storage cleanup </summary>
		inline virtual ~ObjectCache() {
			assert(m_cachedObjects.empty());
			for (typename decltype(m_cachedObjects)::const_iterator it = m_cachedObjects.begin(); it != m_cachedObjects.end(); ++it)
				delete it->second;
		}


	protected:
		/// <summary>
		/// Tries to get an object stored in the cache and, if not found, instantiates a new one
		/// </summary>
		/// <typeparam name="ObjectCreateFn"> Some thing, which, when called (via operator() or a direct call if it's a function), creates a new instance of a storeed object (has to return a Reference) </typeparam>
		/// <param name="key"> Object key within the cache </param>
		/// <param name="createObject"> Some thing, which, when called (via operator() or a direct call if it's a function), creates a new instance of a storeed object (has to return a Reference) </param>
		/// <returns> Cached object instance </returns>
		template<typename ObjectCreateFn>
		inline Reference<StoredObject> GetCachedOrCreate(const KeyType& key, const ObjectCreateFn& createObject) {
			auto tryGetCached = [&]() -> StoredObject* {
				typename std::unordered_map<KeyType, StoredObject*>::const_iterator it = m_cachedObjects.find(key);
				if (it != m_cachedObjects.end()) {
					assert(it->second->m_cacheLock == m_cacheLock);
					assert(it->second->m_cache == this);
					return it->second;
				}
				else return nullptr;
			};

			{
				std::unique_lock<std::mutex> lock(*m_cacheLock);
				Reference<StoredObject> cached = tryGetCached();
				if (cached != nullptr)
					return cached;
			}

			Reference<StoredObject> newObject = createObject();

			Reference<StoredObject> returnValue;
			{
				std::unique_lock<std::mutex> lock(*m_cacheLock);
				StoredObject* cached = tryGetCached();
				if (cached != nullptr) 
					returnValue = cached;
				else if (newObject != nullptr) {
					assert(newObject->m_cacheLock == nullptr);
					assert(newObject->m_cache == nullptr);
					newObject->m_cacheLock = m_cacheLock;
					newObject->m_cache = this;
					newObject->m_cacheKey = key;
					m_cachedObjects[key] = newObject;
					returnValue = newObject;
				}
			}

			return returnValue;
		}


	private:
		// Lock for cache content
		const std::shared_ptr<std::mutex> m_cacheLock = std::make_shared<std::mutex>();

		// Cached objects
		std::unordered_map<KeyType, StoredObject*, KeyHasherType> m_cachedObjects;
	};
}
