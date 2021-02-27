#include "Semaphore.h"



namespace Jimara {
	Semaphore::Semaphore(size_t count) {
		m_value = count;
	}
	
	void Semaphore::wait(size_t count) {
		std::unique_lock<std::mutex> mutexLock(m_lock);
		while (m_value < count) m_condition.wait(mutexLock);
		m_value -= count;
	}
	
	void Semaphore::post(size_t count) {
		std::lock_guard<std::mutex> guard(m_lock);
		m_value += count;
		m_condition.notify_all();
	}

	void Semaphore::set(size_t count) {
		std::lock_guard<std::mutex> guard(m_lock);
		m_value = count;
		m_condition.notify_all();
	}
}
