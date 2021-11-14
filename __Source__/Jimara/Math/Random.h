#pragma once
#include <random>


namespace Jimara {
	namespace Random {
		/// <summary> Thread-local random number generator </summary>
		std::mt19937& ThreadRNG();
	}
}
