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
	template<typename ObjectType>
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
			m_objects.push_back(object);
			return true;
		}

		/// <summary>
		/// Adds multiple objects to the set
		/// Note: selectNewEntries is not allawed to further modify the set for safety reasons (the set might still be in undefined state when it comes to the safe modifications)
		/// </summary>
		/// <typeparam name="ObjectRefType"> Type of the object reference (Reference<ObjectType> or ObjectType* will work just fine) </typeparam>
		/// <typeparam name="SelectNewEntries"> When all new objects get added to the set, this callback will be invoked with added entries as parameters (const Reference<ObjectType> added, size_t count) </typeparam>
		/// <param name="objects"> List of objects to add </param>
		/// <param name="count"> Number of objects to add </param>
		/// <param name="selectNewEntries"> When all new objects get added to the set, this callback will be invoked with added entries as parameters (const Reference<ObjectType> added, size_t count) </param>
		template<typename ObjectRefType, typename SelectNewEntries>
		inline void Add(ObjectRefType* objects, size_t count, SelectNewEntries selectNewEntries) {
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
		inline void Add(Reference<ObjectType>* objects, size_t count) { Add(objects, count, [](Reference<ObjectType>*, size_t) {}); }

		/// <summary>
		/// Adds multiple objects to the set
		/// </summary>
		/// <param name="objects"> List of objects to add </param>
		/// <param name="count"> Number of objects to add </param>
		inline void Add(ObjectType** objects, size_t count) { Add(objects, count, [](Reference<ObjectType>*, size_t) {}); }


		/// <summary>
		/// Removes an object reference from the set
		/// </summary>
		/// <param name="object"> Object to remove </param>
		/// <returns> True, if and only if the object was a part of the set </returns>
		inline bool Remove(ObjectType* object) { 
			bool rv; 
			Remove(object, 1, [&](Reference<ObjectType>*, size_t count) { rv = (count > 0); });
			return rv; 
		}

		/// <summary>
		/// Removes multiple objects from the set
		/// Note: selectRemovedEntries is not allawed to further modify the set for safety reasons (the set might still be in undefined state when it comes to the safe modifications)
		/// </summary>
		/// <typeparam name="ObjectRefType"> Type of the object reference (Reference<ObjectType> or ObjectType* will work just fine) </typeparam>
		/// <typeparam name="SelectRemovedEntries"> When objects get removed from the set, this callback will be invoked with removed entries as parameters (const Reference<ObjectType> removed, size_t count) </typeparam>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		/// <param name="selectRemovedEntries"> When objects get removed from the set, this callback will be invoked with removed entries as parameters (const Reference<ObjectType> removed, size_t count) </param>
		template<typename ObjectRefType, typename SelectRemovedEntries>
		inline void Remove(ObjectRefType* objects, size_t count, SelectRemovedEntries selectRemovedEntries) {
			size_t numRemoved = 0;
			for (size_t i = 0; i < count; i++) {
				ObjectType* object = objects[i];
				if (object == nullptr) continue;
				typename std::unordered_map<ObjectType*, size_t>::iterator it = m_indexMap.find(object);
				if (it == m_indexMap.end()) continue;
				const size_t index = it->second;
				Reference<ObjectType>& reference = m_objects[index];
				m_indexMap.erase(it);
				numRemoved++;
				const size_t lastIndex = (m_objects.size() - numRemoved);
				if (index < lastIndex) {
					std::swap(reference, m_objects[lastIndex]);
					m_indexMap[reference] = index;
				}
			}
			const size_t sizeLeft = (m_objects.size() - numRemoved);
			selectRemovedEntries(Data() + sizeLeft, numRemoved);
			m_objects.resize(sizeLeft);
		}

		/// <summary>
		/// Removes multiple objects from the set
		/// </summary>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		inline void Remove(Reference<ObjectType>* objects, size_t count) { Remove(objects, count, [](const Reference<ObjectType>*, size_t) {}); }

		/// <summary>
		/// Removes multiple objects from the set
		/// </summary>
		/// <param name="objects"> List of objects to remove </param>
		/// <param name="count"> Number of objects to remove </param>
		inline void Remove(ObjectType** objects, size_t count) { Remove(objects, count, [](const Reference<ObjectType>*, size_t) {}); }

		
		/// <summary>
		/// Checks if an object is a part of the set
		/// </summary>
		/// <param name="object"> Object to check </param>
		/// <returns> True, if the set contains given object </returns>
		inline bool Contains(Object* object)const { return m_indexMap.find(object) != m_indexMap.end(); }

		/// <summary> Number of elements within the set </summary>
		inline size_t Size()const { return m_indexMap.size(); }

		/// <summary>
		/// Element by index
		/// Note: If you modify the set, indices can change drastically, so do not rely on this if the set is not constant
		/// </summary>
		/// <param name="index"> Object index </param>
		/// <returns> Index'th object </returns>
		inline ObjectType* operator[](size_t index)const { return m_objects[index]; }

		/// <summary> Currently held objects as a good old reference array </summary>
		inline const Reference<ObjectType>* Data()const { return m_objects.data(); }


	private:
		// Object pointer to data index map
		std::unordered_map<ObjectType*, size_t> m_indexMap;

		// Actual object references
		std::vector<Reference<ObjectType>> m_objects;
	};
}
