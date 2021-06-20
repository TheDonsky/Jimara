#pragma once
#include "../Systems/ActionQueue.h"
#include <condition_variable>
#include <thread>
#include <queue>


namespace Jimara {
	/// <summary>
	/// Thread pool, implementing a completely asynchronous ActionQueue, capable of running queued tasks in paralel
	/// Note: Does not take any task interdependencies into consideration
	/// </summary>
	class ThreadPool : public virtual ActionQueue<> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="numThreads"> Number of threads within the pool </param>
		ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
		
		/// <summary> Virtual destructor </summary>
		virtual ~ThreadPool();

		/// <summary>
		/// Schedules a callback to be executed on the queue
		/// Note: Ordering not necessarily preserved.
		/// </summary>
		/// <param name="callback"> Callback to invoke as the action </param>
		/// <param name="userData"> Arbitrary object, that will be kept alive till the action is queued and passed as an argument during execution </param>
		virtual void Schedule(const Callback<Object*>& callback, Object* userData) override;


	private:
		// Lock for scheduling
		std::mutex m_queueLock;

		// Notified, when we get a new task
		std::condition_variable m_enquedEvent;

		// Scheduled tasks
		std::queue<std::pair<Callback<Object*>, Reference<Object>>> m_queue;

		// Active threads
		std::vector<std::thread> m_threads;

		// Becomes true, when the destructor gets invoked
		std::atomic<bool> m_dead = false;
	};
}
