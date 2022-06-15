#pragma once
#include "../Core/JimaraApi.h"
#include <random>


namespace Jimara {
	namespace Random {
		/// <summary> Thread-local random number generator </summary>
		JIMARA_API std::mt19937& ThreadRNG();
	}
}
