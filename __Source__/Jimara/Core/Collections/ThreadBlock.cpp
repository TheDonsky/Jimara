#include "ThreadBlock.h"


namespace Jimara {
	ThreadBlock::ThreadBlock() : m_executionArgs(nullptr) { }

	ThreadBlock::~ThreadBlock() {
		m_executionArgs = nullptr;
		m_threads.clear();
	}

	void ThreadBlock::Execute(size_t threadCount, void* data, const Callback<ThreadInfo, void*>& job) {
		std::unique_lock<std::mutex> lock(m_callLock);
		ExecutionArgs args = {};
		args.threadCount = threadCount;
		args.job = &job;
		args.userData = data;
		m_executionArgs = &args;
		for (size_t i = 0; i < threadCount; i++) {
			if (m_threads.size() <= i) m_threads.push_back(std::make_unique<ThreadData>(this, i));
			m_threads[i]->semaphore.post();
		}
		m_callerSemaphore.wait(threadCount);
	}

	ThreadBlock::ThreadData::ThreadData(ThreadBlock* block, size_t threadId) {
		thread = std::thread(ThreadBlock::BlockThread, block, threadId, &semaphore);
	}

	ThreadBlock::ThreadData::~ThreadData() {
		semaphore.post();
		thread.join();
	}

	void ThreadBlock::BlockThread(ThreadBlock* self, size_t threadId, Semaphore* semaphore) {
		while (true) {
			semaphore->wait();
			const ExecutionArgs* args = self->m_executionArgs;
			const bool shouldQuit = (args == nullptr);
			if (!shouldQuit) {
				ThreadInfo info = {};
				info.threadId = threadId;
				info.threadCount = args->threadCount;
				(*args->job)(info, args->userData);
			}
			self->m_callerSemaphore.post();
			if (shouldQuit) break;
		}
	}
}
