#pragma once
#include "ObjectSet.h"
#include <cassert>


namespace Jimara {
	/// <summary>
	/// ObjectSet, but Add/Remove/Clear do not take effect immediately and have to be flushed manually
	/// Notes: 
	///		0. This may appear useless at the first glance, but if you wish for some collections to remain intact for some duration and 
	///		apply updates at the end of frame or something, this may come useful (one example would be the collection of existing object in scene data)
	///		1. Collection is not thread safe by design, user is responsible that no schedule/flush actions are running in parallel with each other or any of the read operations;
	///		Reads are pretty safe, though.
	/// </summary>
	/// <typeparam name="ObjectType"> Type of the Object stored </typeparam>
	/// <typeparam name="StoredType"> 
	/// Stored element type 
	/// (generally, this is something like Reference<ObjectType>, but if you want your Data() and indexed accessors to contain additional information, 
	/// any type that takes ObjectType* as constructor and can be cast back to original ObjectType* will suffice) 
	/// </typeparam>
	template<typename ObjectType, typename StoredType = Reference<ObjectType>>
	class DelayedObjectSet {
	public:
		/// <summary>
		/// Schedules addition of the object to the set
		/// </summary>
		/// <param name="object"> Object to add </param>
		inline void ScheduleAdd(ObjectType* object) {
			if (object == nullptr) return;
			assert(!m_scheduling.load());
			m_scheduling = true;
			{
				m_added.Add(object);
				m_removed.Remove(object);
			}
			m_scheduling = false;
		}

		/// <summary>
		/// Schedules removal of the object to the set
		/// </summary>
		/// <param name="object"> Object to remove </param>
		inline void ScheduleRemove(ObjectType* object) {
			if (object == nullptr) return;
			assert(!m_scheduling.load());
			m_scheduling = true;
			{
				m_added.Remove(object);
				m_removed.Add(object);
			}
			m_scheduling = false;
		}

		/// <summary> 
		/// Schedules removal of all stored elements
		/// Note: If anything is scheduled to be added, it will not be cancelled
		/// </summary>
		inline void ScheduleClear() {
			assert(!m_flushing.load());
			for (size_t i = 0; i < m_active.Size(); i++)
				ScheduleRemove(m_active[i]);
		}

		/// <summary> Removed all scheduled changes </summary>
		inline void ClearScheduledChanges() {
			assert(!m_scheduling.load());
			m_scheduling = true;
			{
				m_added.Clear();
				m_removed.Clear();
			}
			m_scheduling = false;
		}

		/// <summary> Removes all currently stored objects immediately (does not remove scheduled additions/removals) </summary>
		inline void ClearCurrentImmediate() {
			assert(!m_flushing.load());
			m_active.Clear();
		}

		/// <summary> Clears all scheduled changes, alongside the currently stored data </summary>
		inline void ClearAllImmediate() {
			ClearScheduledChanges();
			ClearCurrentImmediate();
		}

		/// <summary>
		/// Flushes scheduled changes
		/// Note: 
		///		First, some elements get removed, then some get added; 
		///		onRemoved is invoked right after removal and onAdded right after addition.
		///		Scheduled buffers are cleared only after the onRemoved and onAdded callbacks are executed,
		///		and therefore, it is generally not safe to modify the collection from those callbacks.
		/// </summary>
		/// <typeparam name="OnRemovedCallback"></typeparam>
		/// <typeparam name="OnAddedCallback"></typeparam>
		/// <param name="onRemoved"> Invoked with removed elements as arguments (const StoredType* removed, size_t count) </param>
		/// <param name="onAdded"> Invoked with newly inserted elements as arguments (const StoredType* removed, size_t count) </param>
		template<typename OnRemovedCallback, typename OnAddedCallback>
		inline void Flush(OnRemovedCallback onRemoved, OnAddedCallback onAdded) {
			assert(!m_flushing.load());
			assert(!m_scheduling.load());
			m_flushing = true;
			m_scheduling = true;
			if (m_removed.Size() > 0) {
				m_active.Remove(m_removed.Data(), m_removed.Size(), onRemoved);
				m_removed.Clear();
			}
			if (m_added.Size() > 0) {
				m_active.Add(m_added.Data(), m_added.Size(), onAdded);
				m_added.Clear();
			}
			m_scheduling = false;
			m_flushing = false;
		}

		/// <summary>
		/// Checks if an object is a part of the flushed set
		/// </summary>
		/// <param name="object"> Object to check </param>
		/// <returns> True, if the set contains given object </returns>
		inline bool Contains(ObjectType* object)const {
#ifndef NDEBUG
			assert(!m_flushing.load());
#endif
			return m_active.Contains(object);
		}

		/// <summary>
		/// Searches for the stored object inside the flushed set
		/// </summary>
		/// <param name="object"> Object to search for </param>
		/// <returns> Stored object reference (nullptr if not found) </returns>
		inline const StoredType* Find(ObjectType* object)const {
#ifndef NDEBUG
			assert(!m_flushing.load());
#endif
			return m_active.Find(object);
		}

		/// <summary> Number of elements stored in the flushed set </summary>
		inline size_t Size()const { 
#ifndef NDEBUG
			assert(!m_flushing.load());
#endif
			return m_active.Size(); 
		}

		/// <summary>
		/// Element by index in flushed set
		/// Note: If you modify the set, indices can change drastically, so do not rely on this if the set is not constant
		/// </summary>
		/// <param name="index"> Object index </param>
		/// <returns> Index'th object </returns>
		inline const StoredType& operator[](size_t index)const {
#ifndef NDEBUG
			assert(!m_flushing.load());
#endif
			return m_active[index]; 
		}

		/// <summary> Currently held objects from the flushed set as a good old array </summary>
		inline const StoredType* Data()const {
#ifndef NDEBUG
			assert(!m_flushing.load());
#endif
			return m_active.Data(); 
		}

	private:
		// Flushed set
		ObjectSet<ObjectType, StoredType> m_active;

		// Set of objects to be added on flush
		ObjectSet<ObjectType, StoredType> m_added;

		// Set of objects to be removed on flush
		ObjectSet<ObjectType, StoredType> m_removed;

		// Flag set when any of the schedule/flush-type call is happening
		std::atomic<bool> m_scheduling = false;

		// Flag set when flush is happening
		std::atomic<bool> m_flushing = false;
	};
}
