#include "Random.h"
#include <mutex>

namespace Jimara {
	namespace Random {
		namespace {
			static std::random_device::result_type GlobalRNG() {
				static std::random_device rd;
				static std::mutex mutex;
				std::random_device::result_type rv;
				{
					std::unique_lock<std::mutex> lock(mutex);
					rv = rd();
				};
				return rv;
			}
		}
		
		std::mt19937& ThreadRNG() {
			thread_local std::mt19937 generator(GlobalRNG());
			return generator;
		}
	}
}
