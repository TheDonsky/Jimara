#pragma once
#include "../Function.h"
#include "../Object.h"
#include "../Synch/SpinLock.h"
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
			std::unique_lock<SpinLock> lock(m_scheduleLock);
			m_actionBuffers[m_backBufferIndex.load()].push_back(std::make_pair(callback, userData));
		}

		/// <summary>
		/// Executes all currently queued actions
		/// </summary>
		/// <param name="...args"> Arguments to pass to the actions </param>
		inline void Flush(Args... args) {
			std::unique_lock<std::mutex> executionLock(m_executionLock);
			ActionBuffer* backBuffer;
			{
				std::unique_lock<SpinLock> lock(m_scheduleLock);
				backBuffer = m_actionBuffers + m_backBufferIndex.load();
				m_backBufferIndex = ((m_backBufferIndex.load() + 1) & 1);
			}
			for (size_t i = 0; i < backBuffer->size(); i++) {
				const std::pair<Callback<Object*, Args...>, Reference<Object>>& call = backBuffer->operator[](i);
				call.first(call.second.operator->(), args...);
			}
			backBuffer->clear();
		}

		/// <summary>
		/// Executes all currently queued actions
		/// </summary>
		/// <param name="...args"> Arguments to pass to the actions </param>
		inline void operator()(Args... args) { Flush(args...); }


	private:
		// Lock for scheduling new actions
		SpinLock m_scheduleLock;

		// Scheduled action buffers
		typedef std::vector<std::pair<Callback<Object*, Args...>, Reference<Object>>> ActionBuffer;
		ActionBuffer m_actionBuffers[2];

		// Scheduling action buffer index
		std::atomic<size_t> m_backBufferIndex = 0;

		// Lock, used during execution
		std::mutex m_executionLock;
	};
}
