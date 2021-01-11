#pragma once
#include <chrono>


namespace Jimara {
	/// <summary> A simple 'stopwatch' that can show you elapsed time, as well as offer you the chanse to 'pause' and resume the timer </summary>
	class Stopwatch {
	public:
		/// <summary> Constructor (creates 'active' stopwatch) </summary>
		Stopwatch() : m_start(std::chrono::steady_clock::now()), m_recorded(0.0f), m_stopped(false) {}

		/// <summary>  
		/// Elapsed duration (in seconds) of the timer being active (ei after construction/Reset minus the time spent between Stop() and corresponding Resume() calls) 
		/// Note: Only this call is thread-safe, but only with itself...
		/// </summary>
		float Elapsed()const { return m_recorded + (m_stopped ? 0.0f : std::chrono::duration<float>(std::chrono::steady_clock::now() - m_start).count()); }

		/// <summary>
		/// Stops stopwatch and records elapsed time
		/// </summary>
		/// <returns> Elapsed time so far </returns>
		float Stop() {
			m_recorded = Elapsed();
			m_stopped = true;
			return m_recorded;
		}

		/// <summary>
		/// Resumes stopped stopwatch
		/// </summary>
		/// <returns> Elapsed time so far </returns>
		float Resume() {
			if (m_stopped) {
				m_start = std::chrono::steady_clock::now();
				m_stopped = false;
				return m_recorded;
			}
			else return Elapsed();
		}

		/// <summary>
		/// Resets recorded time (does not resume if stopped)
		/// </summary>
		/// <returns> Elapsed time before reset </returns>
		float Reset() {
			std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
			float elapsed = m_recorded + (m_stopped ? 0.0f : std::chrono::duration<float>(now - m_start).count());
			m_recorded = 0.0f;
			m_start = now;
			return elapsed;
		}


	private:
		// Last activation date
		mutable std::chrono::steady_clock::time_point m_start;

		// Recorded time from last Stop() call
		mutable float m_recorded;

		// True, if stopped
		mutable bool m_stopped;
	};
}
