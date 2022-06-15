#pragma once
#include "../JimaraApi.h"
#include <atomic>

namespace Jimara {
	/// <summary>
	/// Simple spinlock that can be used to protect some data with a guarantee of no lock-related context switches.
	/// Notes: 
	///		0. If the work inside the critical sections is substantial, I would NOT recommend using this;
	///		1. Interface deliberately resembles that of the std::mutex to make sure std::unique_lock<SpinLock> and such are functional.
	/// </summary>
	class JIMARA_API SpinLock {
	public:
		/// <summary> Locks the spinlock </summary>
		inline void lock() {
			while (true) {
				uint32_t expected = 0;
				if (m_lock.compare_exchange_strong(expected, 1)) break;
			}
		}

		/// <summary> Releases the spinlock </summary>
		inline void unlock() {
			m_lock = 0;
		}

	private:
		// Underlying value
		std::atomic<uint32_t> m_lock = 0;
	};
}
