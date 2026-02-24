#include "MainThreadCallbacks.h"
#ifdef __APPLE__
#include "../../Core/Systems/ActionQueue.h"
#include "../../Core/Synch/Semaphore.h"
#include <thread>
#include <atomic>


namespace Jimara {
	namespace OS {
		namespace MainThreadCallbacks {
			namespace {
				static std::mutex mainThreadLock;
				static std::atomic_bool secondaryThreadRunning = false;
				static SynchronousActionQueue<> mainThreadActionQueue;
			}

			int JIMARA_API RunOnSecondaryThread(const Function<int, int, char**>& process, int argc, char** argv) {
				std::unique_lock<decltype(mainThreadLock)> lock(mainThreadLock);
				secondaryThreadRunning = true;
				std::thread secondaryThread([&]() {
					process(argc, argv);
					secondaryThreadRunning = false;
				});
				while (secondaryThreadRunning)
					mainThreadActionQueue.Flush();
				secondaryThread.join();
			}

			void JIMARA_API ExecuteOnMainThread(const Callback<>& action) {
				static thread_local Semaphore sem(0u);
				auto callback = [&](Object*) {
					action();
					sem.post(1u);
				};
				mainThreadActionQueue.Schedule(Callback<Object*>::FromCall(&callback), nullptr);
				sem.wait(1u);
			}
		}
	}
}
#endif
