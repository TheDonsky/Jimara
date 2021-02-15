#pragma once
#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Jimara {
	/// <summary> 2d floating point vector </summary>
	typedef glm::vec2 Vector2;

	/// <summary> 3d floating point vector </summary>
	typedef glm::vec3 Vector3;

	/// <summary> 4d floating point vector </summary>
	typedef glm::vec4 Vector4;


	/// <summary> 2d integer vector </summary>
	typedef glm::ivec2 Int2;

	/// <summary> 3d integer vector </summary>
	typedef glm::ivec3 Int3;

	/// <summary> 4d integer vector </summary>
	typedef glm::ivec4 Int4;


	/// <summary> 2d unsigned integer vector </summary>
	typedef glm::uvec2 Size2;

	/// <summary> 2d unsigned integer vector </summary>
	typedef glm::uvec3 Size3;

	/// <summary> 2d unsigned integer vector </summary>
	typedef glm::uvec4 Size4;


	/// <summary> 2X2 floating point matrix </summary>
	typedef glm::mat2x2 Matrix2;

	/// <summary> 3X3 floating point matrix </summary>
	typedef glm::mat3x3 Matrix3;

	/// <summary> 4X4 floating point matrix </summary>
	typedef glm::mat4x4 Matrix4;

	/// <summary>
	/// Translates degrees to radians
	/// </summary>
	/// <param name="degrees"> Degrees </param>
	/// <returns> Radians </returns>
	inline static float Radians(float degrees) { return glm::radians(degrees); }

	/// <summary>
	/// Translates radians to degrees
	/// </summary>
	/// <param name="radians"> Radians </param>
	/// <returns> Degrees </returns>
	inline static float Degrees(float radians) { return glm::degrees(radians); }
}
