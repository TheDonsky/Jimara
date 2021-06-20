#include "ThreadPool.h"


namespace Jimara {
	namespace {
		inline static void Thread(
			std::mutex* lock, std::condition_variable* condition, std::atomic<bool>* dead, 
			std::queue<std::pair<Callback<Object*>, Reference<Object>>>* queue) {
			while (true) {
				std::pair<Callback<Object*>, Reference<Object>> call(Callback<Object*>([](Object*) {}), nullptr);
				{
					std::unique_lock<std::mutex> mutexLock(*lock);
					while (queue->size() <= 0) {
						if (dead->load()) return;
						else condition->wait(mutexLock);
					}
					call = queue->front();
					queue->pop();
				}
				call.first(call.second);
			}
		}
	}

	ThreadPool::ThreadPool(size_t numThreads) {
		for (size_t i = 0; i < numThreads; i++) 
			m_threads.push_back(std::thread(Thread, &m_queueLock, &m_enquedEvent, &m_dead, &m_queue));
	}

	ThreadPool::~ThreadPool() {
		{
			std::lock_guard<std::mutex> guard(m_queueLock);
			m_dead = true;
			m_enquedEvent.notify_all();
		}
		for (size_t i = 0; i < m_threads.size(); i++)
			m_threads[i].join();
	}

	void ThreadPool::Schedule(const Callback<Object*>& callback, Object* userData) {
		std::lock_guard<std::mutex> guard(m_queueLock);
		m_queue.push(std::make_pair(callback, userData));
		m_enquedEvent.notify_one();
	}
}
