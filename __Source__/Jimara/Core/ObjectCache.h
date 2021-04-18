#pragma once
#include "Object.h"
#include <mutex>
#include <unordered_map>


namespace Jimara {
	/// <summary>
	/// Cache for creating and reusing arbitrary objects
	/// </summary>
	/// <typeparam name="KeyType"> Type of the object identifier within the cache </typeparam>
	template<typename KeyType> 
	class ObjectCache : public virtual Object {
	public:
		/// <summary>
		/// Object that can be stored in a cache of the given type
		/// </summary>
		class StoredObject : public virtual Object {
		public:
			/// <summary> Constructor </summary>
			inline StoredObject() : m_cache(nullptr), m_cacheKey(KeyType()), m_permanentStorage(false) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~StoredObject() {}

		protected:
			/// <summary> Invoked, when Object goes out of scope </summary>
			inline virtual void OnOutOfScope()const override {
				bool shouldDelete;
				if (m_cache != nullptr) {
					std::unique_lock<std::mutex> lock(m_cache->m_cacheLock);
					if (RefCount() > 0) shouldDelete = false;
					else {
						if (m_permanentStorage) shouldDelete = false;
						else {
							m_cache->m_cachedObjects.erase(m_cacheKey);
							shouldDelete = true;
						}
						m_cache = nullptr;
					}
				}
				else shouldDelete = true;

				if (shouldDelete) delete this;
			}

		private:
			// "Owner" cache
			mutable Reference<ObjectCache> m_cache;

			// Storage key within the cache
			KeyType m_cacheKey;

			// If true, the object will not go out of the scope till the moment the cache itself does
			bool m_permanentStorage;

			// Object cache has to access fields defined above, so it's a friend
			friend class ObjectCache;
		};


		/// <summary> Virtual destructor for permanent storage cleanup </summary>
		inline virtual ~ObjectCache() {
			for (typename std::unordered_map<KeyType, StoredObject*>::const_iterator it = m_cachedObjects.begin(); it != m_cachedObjects.end(); ++it)
				delete it->second;
		}


	protected:
		/// <summary>
		/// Tries to get an object stored in the cache and, if not found, instantiates a new one
		/// </summary>
		/// <typeparam name="ObjectCreateFn"> Some thing, which, when called (via operator() or a direct call if it's a function), creates a new instance of a storeed object (has to return a Reference) </typeparam>
		/// <param name="key"> Object key within the cache </param>
		/// <param name="storePermanently"> If true, the object will stay in cache permanently, regardless if anyone is using it or not </param>
		/// <param name="createObject"> Some thing, which, when called (via operator() or a direct call if it's a function), creates a new instance of a storeed object (has to return a Reference) </param>
		/// <returns> Cached object instance </returns>
		template<typename ObjectCreateFn>
		inline Reference<StoredObject> GetCachedOrCreate(const KeyType& key, bool storePermanently, const ObjectCreateFn& createObject) {
			auto tryGetCached = [&]() -> StoredObject* {
				typename std::unordered_map<KeyType, StoredObject*>::const_iterator it = m_cachedObjects.find(key);
				if (it != m_cachedObjects.end()) {
					it->second->m_permanentStorage |= storePermanently;
					return it->second;
				}
				else return nullptr;
			};

			{
				std::unique_lock<std::mutex> lock(m_cacheLock);
				Reference<StoredObject> cached = tryGetCached();
				if (cached != nullptr) {
					if (cached->m_cache == nullptr)
						cached->m_cache = this;
					return cached;
				}
			}

			Reference<StoredObject> newObject = createObject();

			Reference<StoredObject> returnValue;
			{
				std::unique_lock<std::mutex> lock(m_cacheLock);
				StoredObject* cached = tryGetCached();
				if (cached != nullptr) returnValue = cached;
				else if (newObject != nullptr) {
					newObject->m_cacheKey = key;
					newObject->m_permanentStorage = storePermanently;
					m_cachedObjects[key] = newObject;
					returnValue = newObject;
				}
				if (returnValue->m_cache == nullptr)
					returnValue->m_cache = this;
			}

			return returnValue;
		}


	private:
		// Lock for cache content
		std::mutex m_cacheLock;

		// Cached objects
		std::unordered_map<KeyType, StoredObject*> m_cachedObjects;
	};
}
