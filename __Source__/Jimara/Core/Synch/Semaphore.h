#pragma once
#include <mutex>
#include <condition_variable>

namespace Jimara {
	/// <summary> Semaphore </summary>
	class Semaphore {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="count"> Initial count </param>
		Semaphore(unsigned int count = 0);

		/// <summary>
		/// Waits for given number
		/// </summary>
		/// <param name="count"> count to wait for </param>
		void wait(unsigned int count = 1);

		/// <summary>
		/// Appends to count
		/// </summary>
		/// <param name="count"> Count to uppend </param>
		void post(unsigned int count = 1);

		/// <summary>
		/// Directly sets count (not waiting for anything)
		/// </summary>
		/// <param name="count"> Count to set </param>
		void set(unsigned int count);




	private:
		// Current count
		unsigned volatile int m_value;
		
		// Underlying mutex
		std::mutex m_lock;

		// Wait condition
		std::condition_variable m_condition;
	};
}
