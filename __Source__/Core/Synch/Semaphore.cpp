#include "Semaphore.h"



namespace Jimara {
	Semaphore::Semaphore(unsigned int count) {
		m_value = count;
	}
	
	void Semaphore::wait(unsigned int count) {
		std::unique_lock<std::mutex> mutexLock(m_lock);
		while (m_value < count) m_condition.wait(mutexLock);
		m_value -= count;
	}
	
	void Semaphore::post(unsigned int count) {
		std::lock_guard<std::mutex> guard(m_lock);
		m_value += count;
		m_condition.notify_all();
	}

	void Semaphore::set(unsigned int count) {
		std::lock_guard<std::mutex> guard(m_lock);
		m_value = count;
		m_condition.notify_all();
	}
}
