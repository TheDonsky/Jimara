#pragma once
#include "../Function.h"
#include "../Object.h"
#include <mutex>
#include <vector>

namespace Jimara {
	/// <summary>
	/// A generic action queue, that can schedule events for a one time execution
	/// </summary>
	/// <typeparam name="...Args"> Arguments, the action queue will pass to the callbacks </typeparam>
	template<typename... Args>
	class ActionQueue {
	public:
		/// <summary>
		/// Schedules a callback to be executed on the queue
		/// Note: Depending on the implementation, orgering may or may not be preserved.
		/// </summary>
		/// <param name="callback"> Callback to invoke as the action </param>
		/// <param name="userData"> Arbitrary object, that will be kept alive till the action is queued and passed as an argument during execution </param>
		virtual void Schedule(const Callback<Object*, Args...>& callback, Object* userData) = 0;
	};


	/// <summary>
	/// ActionQueue, that executes actions only when "Flushed" manually
	/// </summary>
	/// <typeparam name="...Args"> Arguments, the action queue will pass to the callbacks </typeparam>
	template<typename... Args>
	class SynchronousActionQueue : public virtual ActionQueue<Args...> {
	public:
		/// <summary>
		/// Schedules a callback to be executed on the queue 
		/// Note: Ordering always preserved.
		/// </summary>
		/// <param name="callback"> Callback to invoke as the action </param>
		/// <param name="userData"> Arbitrary object, that will be kept alive till the action is queued and passed as an argument during execution </param>
		inline virtual void Schedule(const Callback<Object*, Args...>& callback, Object* userData) override {
			std::unique_lock<std::mutex> lock(m_scheduleLock);
			m_queue.push_back(std::make_pair(callback, userData));
		}

		/// <summary>
		/// Executes all currently queued actions
		/// </summary>
		/// <param name="...args"> Arguments to pass to the actions </param>
		inline void Flush(Args... args) {
			std::unique_lock<std::mutex> executionLock(m_executionLock);
			{
				std::unique_lock<std::mutex> lock(m_scheduleLock);
				for (size_t i = 0; i < m_queue.size(); i++)
					m_immediateActions.push_back(m_queue[i]);
				m_queue.clear();
			}
			for (size_t i = 0; i < m_immediateActions.size(); i++) {
				const std::pair<Callback<Object*, Args...>, Reference<Object>>& call = m_immediateActions[i];
				call.first(call.second.operator->(), args...);
			}
			m_immediateActions.clear();
		}

		/// <summary>
		/// Executes all currently queued actions
		/// </summary>
		/// <param name="...args"> Arguments to pass to the actions </param>
		inline void operator()(Args... args) { Flush(args...); }


	private:
		// Lock for scheduling new actions
		std::mutex m_scheduleLock;
		
		// Scheduled actions
		std::vector<std::pair<Callback<Object*, Args...>, Reference<Object>>> m_queue;

		// Lock, used during execution
		std::mutex m_executionLock;

		// A buffer, the queued actions get copied to during execution (enables safe scheduling from within the actions)
		std::vector<std::pair<Callback<Object*, Args...>, Reference<Object>>> m_immediateActions;
	};
}
