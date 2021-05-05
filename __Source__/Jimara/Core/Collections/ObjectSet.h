#pragma once
#include "../Reference.h"
#include <unordered_map>
#include <cstdint>


namespace Jimara {
	/// <summary>
	/// Set of Object(or any other reference-counted type) instances
	/// (You chould just use a set of Reference-es, but this one lets us access entries with indices and, therefore, provides some options for paralel processing)
	/// </summary>
	/// <typeparam name="ObjectType"> Element reference type </typeparam>
	/// <typeparam name="StoredType"> 
	/// Stored element type 
	/// (generally, this is something like Reference<ObjectType>, but if you want your Data() and indexed accessors to contain additional information, 
	/// any type that takes ObjectType* as constructor and can be cast back to original ObjectType* will suffice) 
	/// </typeparam>
	template<typename ObjectType, typename StoredType = Reference<ObjectType>>
	class ObjectSet {
	public:
		/// <summary>
		/// Adds an object reference to the set
		/// </summary>
		/// <param name="object"> Object to add </param>
		/// <returns> True, if and only if the object was not nullptr and it was not already a part of the set </returns>
		inline bool Add(ObjectType* object) {
			if (object == nullptr) return false;
			else if (m_indexMap.find(object) != m_indexMap.end()) return false;
			m_indexMap.insert(std::make_pair(object, m_objects.size()));
			m_indexToData.push_back(object);
			m_objects.push_back(StoredType(object));
			return true;
		}

		/// <summary>
		/// Adds multiple objects to the set
		/// Note: selectNewEntries is not allawed to further modify the set for safety reasons (the set might still be in undefined state when it comes to the safe modifications)
		/// </summary>
		/// <typeparam name="ObjectRefType"> Type of the object reference (Reference<ObjectType> or ObjectType* will work just fine) </typeparam>
		/// <typeparam name="SelectNewEntries"> When all new objects get added to the set, this callback will be invoked with added entries as parameters (const StoredType added, size_t count) </typeparam>
		/// <param name="objects"> List of objects to add </param>
		/// <param name="count"> Number of objects to add </param>
		/// <param name="selectNewEntries"> When all new objects get added to the set, this callback will be invoked with added entries as parameters (const StoredType added, size_t count) </param>
		template<typename ObjectRefType, typename SelectNewEntries>
		inline void Add(const ObjectRefType* objects, size_t count, SelectNewEntries selectNewEntries) {
			size_t startIndex = m_objects.size();
			for (size_t i = 0; i < count; i++)
				Add(objects[i]);
			selectNewEntries(Data() + startIndex, m_objects.size() - startIndex);
		}

		/// <summary>
		/// Adds multiple objects to the set
		/// </summary>
		/// <param name="objects"> List of objects to add </param>
		/// <param name="count"> Number of objects to add </param>
		inline void Add(const Reference<ObjectType>* objects, size_t count) { Add(objects, count, [](const StoredType*, size_t) {}); }

		/// <summary>
		/// Adds multiple objects to the set
		/// </summary>
		/// <param name="objects"> List of objects to add </param>
		/// <param name="count"> Number of objects to add </param>
		inline void Add(ObjectType* const * objects, size_t count) { Add(objects, count, [](const StoredType*, size_t) {}); }


		/// <summary>
		/// Removes an object reference from the set
		/// </summary>
		/// <param name="object"> Object to remove </param>
		/// <returns> True, if and only if the object was a part of the set </returns>
		inline bool Remove(ObjectType* object) { 
			bool rv;
			Remove(&object, 1, [&](const StoredType*, size_t count) { rv = (count > 0); });
			return rv; 
		}

		/// <summary>
		/// Removes multiple objects from the set
		/// Note: selectRemovedEntries is not allawed to further modify the set for safety reasons (the set might still be in undefined state when it comes to the safe modifications)
		/// </summary>
		/// <typeparam name="ObjectRefType"> Type of the object reference (Reference<ObjectType> or ObjectType* will work just fine) </typeparam>
		/// <typeparam name="SelectRemovedEntries"> When objects get removed from the set, this callback will be invoked with removed entries as parameters (const StoredType removed, size_t count) </typeparam>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		/// <param name="selectRemovedEntries"> When objects get removed from the set, this callback will be invoked with removed entries as parameters (const StoredType removed, size_t count) </param>
		template<typename ObjectRefType, typename SelectRemovedEntries>
		inline void Remove(const ObjectRefType* objects, size_t count, SelectRemovedEntries selectRemovedEntries) {
			size_t numRemoved = 0;
			for (size_t i = 0; i < count; i++) {
				ObjectType* object = objects[i];
				if (object == nullptr) continue;
				typename std::unordered_map<Reference<ObjectType>, size_t>::iterator it = m_indexMap.find(object);
				if (it == m_indexMap.end()) continue;
				const size_t index = it->second;
				m_indexMap.erase(it);
				numRemoved++;
				const size_t lastIndex = (m_objects.size() - numRemoved);
				if (index < lastIndex) {
					ObjectType*& lastObject = m_indexToData[index];
					std::swap(lastObject, m_indexToData[lastIndex]);
					std::swap(m_objects[index], m_objects[lastIndex]);
					m_indexMap[lastObject] = index;
				}
			}
			const size_t sizeLeft = (m_objects.size() - numRemoved);
			selectRemovedEntries(Data() + sizeLeft, numRemoved);
			m_objects.resize(sizeLeft);
			m_indexToData.resize(sizeLeft);
		}

		/// <summary>
		/// Removes multiple objects from the set
		/// </summary>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		inline void Remove(const Reference<ObjectType>* objects, size_t count) { Remove(objects, count, [](const StoredType*, size_t) {}); }

		/// <summary>
		/// Removes multiple objects from the set
		/// </summary>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		inline void Remove(ObjectType* const * objects, size_t count) { Remove(objects, count, [](const StoredType*, size_t) {}); }

		/// <summary> Removes all entries </summary>
		inline void Clear() {
			m_indexMap.clear();
			m_indexToData.clear();
			m_objects.clear();
		}

		
		/// <summary>
		/// Checks if an object is a part of the set
		/// </summary>
		/// <param name="object"> Object to check </param>
		/// <returns> True, if the set contains given object </returns>
		inline bool Contains(ObjectType* object)const { return m_indexMap.find(object) != m_indexMap.end(); }

		/// <summary>
		/// Searches for the stored object
		/// </summary>
		/// <param name="object"> Object to search for </param>
		/// <returns> Stored object reference (nullptr if not found) </returns>
		inline const StoredType* Find(ObjectType* object)const {
			typename std::unordered_map<Reference<ObjectType>, size_t>::const_iterator it = m_indexMap.find(object);
			if (it == m_indexMap.end()) return nullptr;
			else return &m_objects[it->second];
		}

		/// <summary> Number of elements within the set </summary>
		inline size_t Size()const { return m_indexMap.size(); }

		/// <summary>
		/// Element by index
		/// Note: If you modify the set, indices can change drastically, so do not rely on this if the set is not constant
		/// </summary>
		/// <param name="index"> Object index </param>
		/// <returns> Index'th object </returns>
		inline const StoredType& operator[](size_t index)const { return m_objects[index]; }

		/// <summary> Currently held objects as a good old array </summary>
		inline const StoredType* Data()const { return m_objects.data(); }


	private:
		// Object pointer to data index map
		std::unordered_map<Reference<ObjectType>, size_t> m_indexMap;

		// Index to data map
		std::vector<ObjectType*> m_indexToData;

		// Actual object references
		std::vector<StoredType> m_objects;
	};
}
