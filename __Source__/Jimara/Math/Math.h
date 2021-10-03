#pragma once
#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#pragma warning(disable: 26812)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#pragma warning(default: 26812)


namespace Jimara {
#ifndef min
	// Some windows libraries define min&max, so this using is kinda helpful for cross-platform builds
	using std::min;
#endif

#ifndef max
	// Some windows libraries define min&max, so this using is kinda helpful for cross-platform builds
	using std::max;
#endif

#ifndef abs
	// Some windows libraries define abs, so this using is kinda helpful for cross-platform builds
	using std::abs;
#endif

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
	/// 2d Axis-aligned bounding box (floating point vectors)
	/// </summary>
	struct Rect {
		/// <summary> Minimal position </summary>
		Vector2 start;

		/// <summary> Maximal position </summary>
		Vector2 end;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="s"> Start </param>
		/// <param name="e"> End </param>
		inline constexpr Rect(const Vector2& s = Vector2(0.0f), const Vector2& e = Vector2(0.0f)) : start(s), end(e) {}
	};

	/// <summary>
	/// 2d Axis-aligned bounding box (signed integers)
	/// </summary>
	struct SizeRect {
		/// <summary> Minimal position </summary>
		Size2 start;

		/// <summary> Maximal position </summary>
		Size2 end;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="s"> Start </param>
		/// <param name="e"> End </param>
		inline constexpr SizeRect(const Size2& s = Size2(0u), const Size2& e = Size2(0u)) : start(s), end(e) {}
	};

	/// <summary>
	/// Axis-aligned bounding box (floating point vectors)
	/// </summary>
	struct AABB {
		/// <summary> Minimal position </summary>
		Vector3 start;

		/// <summary> Maximal position </summary>
		Vector3 end;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="s"> Start </param>
		/// <param name="e"> End </param>
		inline constexpr AABB(const Vector3& s = Vector3(0.0f), const Vector3& e = Vector3(0.0f)) : start(s), end(e) {}
	};

	/// <summary>
	/// Axis-aligned bounding box (signed integers)
	/// </summary>
	struct SizeAABB {
		/// <summary> Minimal coordinates </summary>
		Size3 start;

		/// <summary> Maximal coordinates </summary>
		Size3 end;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="s"> Start </param>
		/// <param name="e"> End </param>
		inline constexpr SizeAABB(const Size3& s = Size3(0u), const Size3& e = Size3(0u)) : start(s), end(e) {}
	};

	namespace Math {
		/// <summary>
		/// (a % b) for floats 
		/// Note: Always positive; a = k * b + FloatRemainder(a, b) will be true for some integer k.
		/// </summary>
		/// <param name="a"> Left side of '%' </param>
		/// <param name="b"> Right side of '%' </param>
		/// <returns> a % b </returns>
		inline static constexpr float FloatRemainder(float a, float b) {
			if (b < 0.0f) b = -b;
			int d = static_cast<int>(a / b);
			if (a < 0.0f) return FloatRemainder(a - (static_cast<float>(d - 1) * b), b);
			else return a - (d * b);
		}

		/// <summary> PI </summary>
		inline static constexpr float Pi() { return glm::pi<float>(); }

		/// <summary> Up vector (Y) </summary>
		inline static constexpr Vector3 Up() { return Vector3(0.0f, 1.0f, 0.0f); }

		/// <summary> Down vector (-Y) </summary>
		inline static constexpr Vector3 Down() { return Vector3(0.0f, -1.0f, 0.0f); }

		/// <summary> Forward vector (Z) </summary>
		inline static constexpr Vector3 Forward() { return Vector3(0.0f, 0.0f, 1.0f); }

		/// <summary> Back vector (-Z) </summary>
		inline static constexpr Vector3 Back() { return Vector3(0.0f, 0.0f, -1.0f); }

		/// <summary> Right vector (X) </summary>
		inline static constexpr Vector3 Right() { return Vector3(1.0f, 0.0f, 0.0f); }

		/// <summary> Left vector (-X) </summary>
		inline static constexpr Vector3 Left() { return Vector3(-1.0f, 0.0f, 0.0f); }

		/// <summary>
		/// Dot product
		/// </summary>
		/// <typeparam name="VectorType"> Type of the vectors </typeparam>
		/// <param name="a"> First vector </param>
		/// <param name="b"> Second vector </param>
		/// <returns> Dot product </returns>
		template<typename VectorType>
		inline static constexpr float Dot(const VectorType& a, const VectorType& b) { return glm::dot(a, b); }

		/// <summary>
		/// Cross product
		/// </summary>
		/// <param name="a"> First vector </param>
		/// <param name="b"> Second vector </param>
		/// <returns> Cross product </returns>
		inline static constexpr Vector3 Cross(const Vector3& a, const Vector3& b) { return glm::cross(a, b); }

		/// <summary>
		/// Returns a vector with the same direction and magnitude of 1
		/// </summary>
		/// <param name="vector"> Vector </param>
		/// <returns> Normalized vector </returns>
		inline static Vector3 Normalize(const Vector3& vector) { return glm::normalize(vector); }

		/// <summary>
		/// Translates degrees to radians
		/// </summary>
		/// <param name="degrees"> Degrees </param>
		/// <returns> Radians </returns>
		inline static constexpr float Radians(float degrees) { return glm::radians(degrees); }

		/// <summary>
		/// Translates radians to degrees
		/// </summary>
		/// <param name="radians"> Radians </param>
		/// <returns> Degrees </returns>
		inline static constexpr float Degrees(float radians) { return glm::degrees(radians); }

		/// <summary>
		/// Generates rotation matrix from euler angles
		/// (Y->X->Z order)
		/// </summary>
		/// <param name="eulerAngles"> Euler angles </param>
		/// <returns> Rotation matrix </returns>
		inline static Matrix4 MatrixFromEulerAngles(const Vector3& eulerAngles) {
			return glm::eulerAngleYXZ(Radians(eulerAngles.y), Radians(eulerAngles.x), Radians(eulerAngles.z));
		}

		/// <summary>
		/// Extracts euler angles from a rotation matrix
		/// </summary>
		/// <param name="rotation"> Rotation matrix (should be of a valid type) </param>
		/// <returns> Euler angles, that will generate matrix via MatrixFromEulerAngles call </returns>
		inline static Vector3 EulerAnglesFromMatrix(const Matrix4& rotation) {
			Vector3 eulerAngles;
			glm::extractEulerAngleYXZ(rotation, eulerAngles.y, eulerAngles.x, eulerAngles.z);
			return Vector3(Degrees(eulerAngles.x), Degrees(eulerAngles.y), Degrees(eulerAngles.z));
		}

		/// <summary>
		/// Inverts matrix
		/// </summary>
		/// <param name="matrix"> Matrix to invert </param>
		/// <returns> Inverted matrix </returns>
		inline static Matrix4 Inverse(const Matrix4& matrix) {
			return glm::inverse(matrix);
		}

		/// <summary>
		/// Transposes the matrix
		/// </summary>
		/// <param name="matrix"> Matrix to transpose </param>
		/// <returns> Transposed matrix </returns>
		inline static Matrix4 Transpose(const Matrix4& matrix) {
			return glm::transpose(matrix);
		}

		/// <summary>
		/// Makes a rotation matrix, looking in some direction
		/// </summary>
		/// <param name="direction"> Direction to look towards </param>
		/// <param name="up"> "Up" direction </param>
		/// <returns> Rotation matrix </returns>
		inline static Matrix4 LookTowards(const Vector3& direction, const Vector3& up = Up()) {
			Vector3 normalizedDirection = Normalize(direction);
			float upDot = Dot(normalizedDirection, up);
			if (upDot < 0.0f) upDot *= -1;
			Vector3 safeUp = (upDot > 0.999f) ? (up + Vector3(-0.001f * normalizedDirection.y, 0.001f * normalizedDirection.z, 0.001f * normalizedDirection.x)) : up;
			return glm::transpose(glm::lookAt(Vector3(0.0f), normalizedDirection, safeUp));
		}

		/// <summary>
		/// Makes a transformation matrix that positions a subject at some position and makes it "look" towards some target
		/// </summary>
		/// <param name="origin"> Position to put subject at </param>
		/// <param name="target"> Target to look at </param>
		/// <param name="up"> "Up" direction </param>
		/// <returns> Transformation matrix </returns>
		inline static Matrix4 LookAt(const Vector3& origin, const Vector3& target, const Vector3& up = Vector3(0.0f, 1.0f, 0.0f)) {
			Matrix4 look = LookTowards(target - origin);
			look[3] = Vector4(origin, 1.0f);
			return look;
		}

		/// <summary>
		/// Perspective projection matrix
		/// </summary>
		/// <param name="filedOfView"> Field of view </param>
		/// <param name="aspectRatio"> Image aspect ratio </param>
		/// <param name="closePlane"> Close clipping plane </param>
		/// <param name="farPlane"> Far clipping plane </param>
		/// <returns> Projection matrix </returns>
		inline static Matrix4 Perspective(float filedOfView, float aspectRatio, float closePlane, float farPlane) {
			return glm::perspective(filedOfView, aspectRatio, closePlane, farPlane);
		}

		/// <summary> Identity matrix </summary>
		inline static constexpr Matrix4 Identity() {
			return glm::identity<Matrix4>();
		}

		/// <summary>
		/// Given rotation and transformation matrices, extracts a kind of a lossy scale
		/// (Rotation matrix helps only with the signs of the values)
		/// </summary>
		/// <param name="transform"> Arbitrary transformation matrix </param>
		/// <param name="rotation"> 
		/// Rotation matrix, corresponding to the same transformation matrix (transform[0], transform[1] and transform[2] would normally be the same, scaled by, x, y, z) 
		/// </param>
		/// <returns> "Lossy" scale </returns>
		inline static constexpr Vector3 LossyScale(const Matrix4& transform, const Matrix4& rotation) {
			auto scale = [&](const Vector4& scaled, const Vector4& baseDir) {
				float base = sqrt(Math::Dot(scaled, scaled));
				return (
					((baseDir.x * scaled.x) < 0.0f) ||
					((baseDir.y * scaled.y) < 0.0f) ||
					((baseDir.z * scaled.z) < 0.0f) ||
					((baseDir.w * scaled.w) < 0.0f)) ? (-base) : base;
			};
			return Vector3(
				scale(transform[0], rotation[0]),
				scale(transform[1], rotation[1]),
				scale(transform[2], rotation[2]));
		}
	}
}
