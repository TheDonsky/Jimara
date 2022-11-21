#pragma once
#include "../Core/JimaraApi.h"
#include "Math.h"
#include <random>


namespace Jimara {
	namespace Random {
		/// <summary> Thread-local random number generator </summary>
		JIMARA_API std::mt19937& ThreadRNG();

		/// <summary> Random unsigned integer </summary>
		JIMARA_API unsigned int Uint();

		/// <summary>
		/// Generates a random unsigned integer in given range
		/// </summary>
		/// <param name="minimum"> Min value (inclusive) </param>
		/// <param name="maximum"> Max value (exclusive) </param>
		/// <returns> Random unsigned integer between minimum and maximum </returns>
		JIMARA_API unsigned int Uint(unsigned int minimum, unsigned int maximum);

		/// <summary> Random size_t </summary>
		JIMARA_API size_t Size();

		/// <summary>
		/// Generates a random size_t in given range
		/// </summary>
		/// <param name="minimum"> Min value (inclusive) </param>
		/// <param name="maximum"> Max value (exclusive) </param>
		/// <returns> Random unsigned integer between minimum and maximum </returns>
		JIMARA_API size_t Size(size_t minimum, size_t maximum);

		/// <summary> Random signed integer </summary>
		JIMARA_API int Int();

		/// <summary>
		/// Generates a random signed integer in given range
		/// </summary>
		/// <param name="minimum"> Min value (inclusive) </param>
		/// <param name="maximum"> Max value (exclusive) </param>
		/// <returns> Random unsigned integer between minimum and maximum </returns>
		JIMARA_API int Int(int minimum, int maximum);

		/// <summary> Generates a random floating point in (0-1) range </summary>
		JIMARA_API float Float();

		/// <summary> 
		/// Generates a random floating point in given range 
		/// </summary>
		/// <param name="minimum"> Min value </param>
		/// <param name="maximum"> Max value </param>
		/// <returns> Random floating point between minimum and maximum </returns>
		JIMARA_API float Float(float minimum, float maximum);

		/// <summary> Random boolean value (with 50:50 chance) </summary>
		JIMARA_API bool Boolean();

		/// <summary> 
		/// Weighted random boolean value 
		/// </summary>
		/// <param name="chance"> Probability of true result </param>
		/// <returns> Random boolean value </returns>
		JIMARA_API bool Boolean(float chance);

		/// <summary> Random 2d direction (a point on unit circle) </summary>
		JIMARA_API Vector2 PointOnCircle();

		/// <summary> Random 2d point inside an unit circle) </summary>
		JIMARA_API Vector2 PointInCircle();

		/// <summary> Random 3d direction (a point on unit sphere) </summary>
		JIMARA_API Vector3 PointOnSphere();

		/// <summary> Random 3d point inside an unit sphere </summary>
		JIMARA_API Vector3 PointInSphere();
	}
}
