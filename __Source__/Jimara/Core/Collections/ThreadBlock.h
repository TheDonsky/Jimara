#pragma once
#include "../Function.h"
#include "../Synch/Semaphore.h"
#include <thread>
#include <vector>
#include <atomic>
#include <memory>


namespace Jimara {
	/// <summary>
	/// A simple utility that helps us run arbitrary tasks on multiple threads
	/// </summary>
	class ThreadBlock {
	public:
		/// <summary> Constructor </summary>
		ThreadBlock();

		/// <summary> Virtual destructor </summary>
		virtual ~ThreadBlock();

		/// <summary> Information about a thread within a block </summary>
		struct ThreadInfo {
			/// <summary> Thread index </summary>
			size_t threadId;

			/// <summary> Number of currently active threads </summary>
			size_t threadCount;
		};

		/// <summary>
		/// Executes a job on a thread block
		/// </summary>
		/// <param name="threadCount"> Number of threads to use </param>
		/// <param name="data"> User data to pass back into the job callback </param>
		/// <param name="job"> Arbitrary "Job", taking thread information and user data as arguments </param>
		void Execute(size_t threadCount, void* data, const Callback<ThreadInfo, void*>& job);


	private:
		// Per-thread data
		struct ThreadData {
			std::thread thread;
			Semaphore semaphore;

			ThreadData(ThreadBlock* block, size_t threadId);
			~ThreadData();
		};

		// We use this to protect Execute() calls
		std::mutex m_callLock;

		// Active thread list
		std::vector<std::unique_ptr<ThreadData>> m_threads;

		// Semaphore we wait on to know that the deed is done
		Semaphore m_callerSemaphore;

		// Arguments, used in Execute() call
		struct ExecutionArgs {
			std::atomic<size_t> threadCount;
			const Callback<ThreadInfo, void*>* job;
			void* userData;
		};

		// Arguments, used in Execute() call (nullptr means termination request for the threads)
		const ExecutionArgs* volatile m_executionArgs;

		// Individual thread logic within the block
		static void BlockThread(ThreadBlock* self, size_t threadId, Semaphore* semaphore);
	};
}
