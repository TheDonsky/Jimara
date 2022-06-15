#pragma once
#include "../JimaraApi.h"
#include <mutex>
#include <cstdint>
#include <condition_variable>

namespace Jimara {
	/// <summary> Semaphore </summary>
	class JIMARA_API Semaphore {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="count"> Initial count </param>
		Semaphore(size_t count = 0);

		/// <summary>
		/// Waits for given number
		/// </summary>
		/// <param name="count"> count to wait for </param>
		void wait(size_t count = 1);

		/// <summary>
		/// Appends to count
		/// </summary>
		/// <param name="count"> Count to uppend </param>
		void post(size_t count = 1);

		/// <summary>
		/// Directly sets count (not waiting for anything)
		/// </summary>
		/// <param name="count"> Count to set </param>
		void set(size_t count);




	private:
		// Current count
		volatile size_t m_value;
		
		// Underlying mutex
		std::mutex m_lock;

		// Wait condition
		std::condition_variable m_condition;
	};
}
