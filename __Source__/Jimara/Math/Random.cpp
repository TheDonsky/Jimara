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

		unsigned int Uint() {
			return ThreadRNG()(); 
		}

		unsigned int Uint(unsigned int minimum, unsigned int maximum) {
			return Uint() % (maximum - minimum) + minimum; 
		}

		size_t Size() {
			size_t result;
			static_assert(sizeof(size_t) >= sizeof(unsigned int));
			static_assert((sizeof(size_t) % sizeof(unsigned int)) == 0u);
			const constexpr size_t count = (sizeof(size_t) / sizeof(unsigned int));
			unsigned int* uints = reinterpret_cast<unsigned int*>(&result);
			for (size_t i = 0; i < count; i++)
				uints[i] = Uint();
			return result;
		}

		size_t Size(size_t minimum, size_t maximum) {
			return Size() % (maximum - minimum) + minimum;
		}

		int Int() {
			return static_cast<int>(ThreadRNG()());
		}

		int Int(int minimum, int maximum) {
			return Int() % (maximum - minimum) + minimum;
		}

		float Float() {
			return float(Uint()) / float(~uint32_t(0));
		}

		float Float(float minimum, float maximum) {
			return Float() * (maximum - minimum) + minimum;
		}

		bool Bool() {
			return (Uint() & 1u) == 1u;
		}

		bool Bool(float chance) {
			return Float() <= chance;
		}

		Vector2 PointOnCircle() {
			float theta = (2.0f * Math::Pi() * Float());
			return Vector2(std::cos(theta), std::sin(theta));
		}

		Vector3 PointOnSphere() {
			float theta = (2.0f * Math::Pi() * Float());
			float cosPhi = (1.0f - (2.0f * Float()));
			float sinPhi = std::sqrt(1.0f - (cosPhi * cosPhi));
			return Vector3(
				(sinPhi * std::cos(theta)),
				(sinPhi * std::sin(theta)),
				cosPhi);
		}
	}
}
