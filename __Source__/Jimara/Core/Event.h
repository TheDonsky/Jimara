#pragma once
#include <queue>
#include <mutex>
#include <unordered_set>
#include <assert.h>
#include <cstring>
#include "Function.h"

namespace Jimara {
	/// <summary>
	/// Generic event, one can subscribe to and unsubscribe from
	/// Note: This is an interface for the users and does not provide firing capabilities; actual class that fires events should have an instance of EventInstance or something like that.
	/// </summary>
	/// <typeparam name="...Args"> Arguments, provided each time the event is fired </typeparam>
	template<typename... Args>
	class Event {
	public:
		/// <summary> Virtual destructor </summary>
		virtual inline ~Event() { }

		/// <summary>
		/// Subscribes callback to event (thread safety is not guaranteed, but most implementations will at least try to provide some amount of it under most circumstances)
		/// </summary>
		/// <param name="callback"> Callback to subscribe </param>
		virtual void operator+=(Callback<Args...> callback) = 0;

		/// <summary>
		/// Unsubscribes callback from event (thread safety is not guaranteed, but most implementations will at least try to provide some amount of it under most circumstances)
		/// </summary>
		/// <param name="callback"> Callback to unsubscribe </param>
		virtual void operator-=(Callback<Args...> callback) = 0;
	};






	/// <summary>
	/// Event that can be fired 
	/// (does not implement Event interface, but can be casted to it; this provides some amount of safeguard, if someone tries to invoke the event, in a way it's not supposed to be fired)
	/// </summary>
	/// <typeparam name="...Args"> Arguments, provided each time the event is fired </typeparam>
	template<typename... Args>
	class EventInstance {
	public:
		/// <summary> Creates empty event instance </summary>
		inline EventInstance() : m_dirty(false), m_event(this) {}

		/// <summary> Type cast to Event </summary>
		inline operator Event<Args...>& () { return m_event; }

		/// <summary>
		/// Fires the event
		/// </summary>
		/// <param name="...args"> Callback arguments </param>
		inline void operator()(Args... args)const {
			std::unique_lock<std::recursive_mutex> lock(m_lock);
			m_dirty = false;
			for (typename std::unordered_set<Callback<Args...>>::const_iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it)
				m_queue.push(*it);
			while (m_queue.size() > 0) {
				Callback<Args...> callback = m_queue.front();
				m_queue.pop();
				if (m_dirty) if (m_callbacks.find(callback) == m_callbacks.end()) continue;
				callback(args...);
			}
		}

		/// <summary> Removes all subscriptions </summary>
		inline void Clear() {
			std::unique_lock<std::recursive_mutex> lock(m_lock);
			m_callbacks.clear();
		}


		
	private:
		// Collection lock
		mutable std::recursive_mutex m_lock;

		// Collection of callbacks
		std::unordered_set<Callback<Args...>> m_callbacks;

		// Internal queue for invokation safety
		mutable std::queue<Callback<Args...>> m_queue;

		// True, if the collection gets altered mid-firing
		mutable bool m_dirty;

		// 'Event' type wrapper
		class EventWrapper : public virtual Event<Args...> {
		private:
			// Underlying instance
			EventInstance* m_instance;

		public:
			// Constructor
			EventWrapper(EventInstance* instance) : m_instance(instance) { }

			// Adds callback
			virtual void operator+=(Callback<Args...> callback) override {
				std::unique_lock<std::recursive_mutex> lock(m_instance->m_lock);
				m_instance->m_callbacks.insert(callback);
			}

			// Removes callback
			virtual void operator-=(Callback<Args...> callback) override {
				std::unique_lock<std::recursive_mutex> lock(m_instance->m_lock);
				m_instance->m_callbacks.erase(callback);
				m_instance->m_dirty = true;
			}
		};

		// 'Event' type wrapper
		EventWrapper m_event;
	};
}
