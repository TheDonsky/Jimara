#pragma once
#include "../Intersections.h"
#include "../../Core/JimaraApi.h"


namespace Jimara {
	/// <summary> A simple tetrahedron </summary>
	struct Tetrahedron {
		/// <summary> Vertices </summary>
		Vector3 a = {}, b = {}, c = {}, d = {};

		/// <summary> Constructor </summary>
		inline constexpr Tetrahedron() {};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="A"> First Vertex </param>
		/// <param name="B"> Second Vertex </param>
		/// <param name="C"> Third Vertex </param>
		/// <param name="D"> Fourth Vertex </param>
		inline constexpr Tetrahedron(const Vector3& A, const Vector3& B, const Vector3& C, const Vector3& D) : a(A), b(B), c(C), d(D) {}

		/// <summary>
		/// Vertex by index
		/// </summary>
		/// <param name="index"> Vertex index </param>
		/// <returns> Vertex </returns>
		inline Vector3& operator[](size_t index) { return ((Vector3*)this)[index]; }

		/// <summary>
		/// Vertex by index
		/// </summary>
		/// <param name="index"> Vertex index </param>
		/// <returns> Vertex </returns>
		inline const Vector3& operator[](size_t index)const { return ((const Vector3*)this)[index]; }

		/// <summary>
		/// Compares to other Tetrahedron
		/// (returns true if and only if all vertices and their order match)
		/// </summary>
		/// <param name="other"> Other tetrahedron </param>
		/// <returns> this == other </returns>
		inline bool operator==(const Tetrahedron& other)const {
			return a == other.a && b == other.b && c == other.c && d == other.d;
		}

		/// <summary>
		/// Compares to other Tetrahedron
		/// (returns false if and only if any of the vertices or their order do not match)
		/// </summary>
		/// <param name="other"> Other tetrahedron </param>
		/// <returns> this != other </returns>
		inline bool operator!=(const Tetrahedron& other)const { return !((*this) == other); }

		/// <summary> Bounding box of a tetrahedron </summary>
		inline AABB BoundingBox()const {
			const auto maxAxis = [&](uint32_t axis) { return Math::Max(a[axis], b[axis], c[axis], d[axis]); };
			const auto minAxis = [&](uint32_t axis) { return Math::Min(a[axis], b[axis], c[axis], d[axis]); };
			return AABB(
				Vector3(minAxis(0u), minAxis(1u), minAxis(2u)),
				Vector3(maxAxis(0u), maxAxis(1u), maxAxis(2u)));
		}

		/// <summary> Mass center of the tetrahedron </summary>
		inline Vector3 Center()const { return (a + b + c + d) * 0.25f; }

		/// <summary> Tetrahedron volume </summary>
		inline float Volume()const {
			const Vector3 cross = Math::Cross(b - a, c - a);
			const float crossMagn = Math::Magnitude(cross);
			const float height = std::abs(Math::Dot(cross / ((crossMagn > 0.0f) ? crossMagn : 1.0f), d - a));
			return height * crossMagn * (1.0f / 6.0f);
		}

		/// <summary>
		/// Creates a new tetrahedron with the same shape, but with the vertices reordered based on the weights (ascending order)
		/// </summary>
		/// <param name="weights"> Sorting weights </param>
		/// <returns> Sorted tetrahedron </returns>
		inline constexpr Tetrahedron SortByWeight(Vector4 weights)const {
			Tetrahedron res = *this;
			const auto order = [](float& iW, float& jW, Vector3& iV, Vector3& jV) {
				if (iW > jW) {
					const constexpr auto swp = [](auto& a, auto& b) {
						const std::remove_reference_t<decltype(a)> tmp = a;
						a = b;
						b = tmp;
					};
					swp(iW, jW);
					swp(iV, jV);
				}
			};
			order(weights.x, weights.y, res.a, res.b);
			order(weights.y, weights.z, res.b, res.c);
			order(weights.z, weights.w, res.c, res.d);
			order(weights.x, weights.y, res.a, res.b);
			order(weights.y, weights.z, res.b, res.c);
			order(weights.x, weights.y, res.a, res.b);
			return res;
		}

		/// <summary>
		/// Creates a new tetrahedron with the same shape, but with the vertices reordered based on the weights (ascending order)
		/// </summary>
		/// <param name="wA"> Weight of a </param>
		/// <param name="wB"> Weight of b </param>
		/// <param name="wC"> Weight of c </param>
		/// <param name="wD"> Weight of d </param>
		/// <returns> Sorted tetrahedron </returns>
		inline constexpr Tetrahedron SortByWeight(float wA, float wB, float wC, float wD)const {
			constexpr auto checkAllConfigs = []() -> bool {
				const auto val = [](float a, float b, float c, float d) {
					return Tetrahedron(
						Math::MakeVector3(a, a, a), 
						Math::MakeVector3(b, b, b), 
						Math::MakeVector3(c, c, c), 
						Math::MakeVector3(d, d, d));
				};
				const auto sortedWithWeights = [&](float a, float b, float c, float d) {
					return val(a, b, c, d).SortByWeight(Math::MakeVector4(a, b, c, d));
				};
				for (uint32_t i = 0u; i < 4u; i++)
					for (uint32_t j = 0u; j < 4u; j++)
						if (i != j) for (uint32_t k = 0u; k < 4u; k++)
							if (i != k && j != k) for (uint32_t p = 0u; p < 4u; p++)
								if (i != p && j != p && k != p) {
									const Tetrahedron sorted = sortedWithWeights(float(i), float(j), float(k), float(p));
									const Tetrahedron expected = val(0.0f, 1.0f, 2.0f, 3.0f);
									const auto ne = [](const Vector3& a, const Vector3& b) {
										return a.x != b.x || a.y != b.y || a.z != b.z;
									};
									if (ne(sorted.a, expected.a) ||
										ne(sorted.b, expected.b) ||
										ne(sorted.c, expected.c) ||
										ne(sorted.d, expected.d))
										return false;
								}
				return true;
			};
			static_assert(checkAllConfigs());
			return SortByWeight(Vector4(wA, wB, wC, wD));
		}

		/// <summary>
		/// Checks overlap against a bounding box
		/// </summary>
		/// <typeparam name="InspectOverlappingBitFn"> Overlapping tetrahedral parts will be reported through this callback </typeparam>
		/// <param name="bbox"> Bounding box to check against </param>
		/// <param name="inspectOverlappingBit"> Overlapping tetrahedral parts will be reported through this callback </param>
		/// <returns> True, if any overlap is found </returns>
		template<typename InspectOverlappingBitFn>
		inline bool CheckOverlap(const AABB& bbox, const InspectOverlappingBitFn& inspectOverlappingBit)const {
			const constexpr auto crossPoint = [](const Vector3& from, const Vector3& to, float fromV, float toV, float barrier) {
				return from + (to - from) * ((barrier - fromV) / (toV - fromV));
			};

			const auto overlapHalfPlane = [&](const Tetrahedron& t, float a, float b, float c, float d, float threshold, const auto& overlapPart) -> bool {
				const Vector3& A = t[0u];
				const Vector3& B = t[1u];
				const Vector3& C = t[2u];
				const Vector3& D = t[3u];
				if (d < threshold)
					return false; // a b c d |
				else if (c < threshold) {
					// a b c | d
					return overlapPart(Tetrahedron(
						crossPoint(A, D, a, d, threshold),
						crossPoint(B, D, b, d, threshold),
						crossPoint(C, D, c, d, threshold),
						D));
				}
				else if (b < threshold) {
					// a b | c d
					const Vector3 ac = crossPoint(A, C, a, c, threshold);
					const Vector3 ad = crossPoint(A, D, a, d, threshold);
					const Vector3 bc = crossPoint(B, C, b, c, threshold);
					const Vector3 bd = crossPoint(B, D, b, d, threshold);
					if (overlapPart(Tetrahedron(ac, bc, ad, C)))
						return true;
					else if (overlapPart(Tetrahedron(bc, ad, bd, C)))
						return true;
					else return overlapPart(Tetrahedron(ad, bd, C, D));
				}
				else if (a < threshold) {
					// a | b c d
					const Vector3 ab = crossPoint(A, B, a, b, threshold);
					const Vector3 ac = crossPoint(A, C, a, c, threshold);
					const Vector3 ad = crossPoint(A, D, a, d, threshold);
					if (overlapPart(Tetrahedron(ab, ac, ad, B)))
						return true;
					else if (overlapPart(Tetrahedron(ac, ad, B, C)))
						return true;
					else return overlapPart(Tetrahedron(ad, B, C, D));
				}
				else {
					// | a b c d
					return overlapPart(t);
				}
			};

			const auto overlapAxis = [&](Tetrahedron t, uint8_t axis, const auto& nextAxis) -> bool {
				t = t.SortByWeight(t[0u][axis], t[1u][axis], t[2u][axis], t[3u][axis]);
				const float start = bbox.start[axis];
				const float end = bbox.end[axis];
				if (t[0u][axis] > end)
					return false; // || a b c d
				return overlapHalfPlane(t, t[0u][axis], t[1u][axis], t[2u][axis], t[3u][axis], start, [&](Tetrahedron part) -> bool {
					part = Tetrahedron(-part[3u], -part[2u], -part[1u], -part[0u]);
					return overlapHalfPlane(part, part[0u][axis], part[1u][axis], part[2u][axis], part[3u][axis], -end,
						[&](const Tetrahedron& pt) -> bool {
							return nextAxis(Tetrahedron(-pt[3u], -pt[2u], -pt[1u], -pt[0u]));
						});
					});
			};

			bool intersects = false;
			intersects |= overlapAxis(*this, 2u, [&](const Tetrahedron& t0) {
				return overlapAxis(t0, 0u, [&](const Tetrahedron& t1) {
					return overlapAxis(t1, 1u, [&](const Tetrahedron& t) {
						intersects = true;
						return inspectOverlappingBit(t);
						});
					});
				});
			return intersects;
		}

		/// <summary>
		/// Checks overlap against a bounding box
		/// </summary>
		/// <param name="bbox"> Bounding box to check against </param>
		/// <returns> True, if any overlap is found </returns>
		inline bool CheckOverlap(const AABB& bbox)const {
			return CheckOverlap(bbox, [&](const auto&) { return true; });
		}

		/// <summary>
		/// Checks overlap against a bounding box
		/// </summary>
		/// <param name="bbox"> Bounding box to check against </param>
		/// <returns> Overlap information </returns>
		inline Math::ShapeOverlapResult<Tetrahedron, AABB> Overlap(const AABB& bbox)const {
			Math::ShapeOverlapResult<Tetrahedron, AABB> result;
			result.volume = 0.0f;
			result.center = (bbox.start + bbox.end) * 0.5f;
			const auto inspectPart = [&](const Tetrahedron& t) {
				const float volume = t.Volume();
				if (volume > 0.0f) {
					result.volume += volume;
					result.center = Math::Lerp(result.center, t.Center(), volume / result.volume);
				}
				return false;
			};
			if (!CheckOverlap(bbox, inspectPart))
				return {};
			return result;
		}
	};

	
	/// <summary>
	/// Transformed Tetrahedron
	/// </summary>
	/// <param name="transform"> Transformation matrix </param>
	/// <param name="shape"> Tetrahedron </param>
	/// <returns> Transformed tetrahedron </returns>
	inline Tetrahedron operator*(const Matrix4& transform, const Tetrahedron& shape) {
		return Tetrahedron(
			transform * Vector4(shape[0u], 1.0f),
			transform * Vector4(shape[1u], 1.0f),
			transform * Vector4(shape[2u], 1.0f),
			transform * Vector4(shape[3u], 1.0f));
	}

	namespace Math {
		/// <summary>
		/// Calculates tetrahedron and bbox overlap information
		/// </summary>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="t"> Triangle </param>
		/// <returns> Overlap information </returns>
		template<>
		inline ShapeOverlapResult<AABB, Tetrahedron> Overlap<AABB, Tetrahedron>(const AABB& bbox, const Tetrahedron& t) {
			return t.Overlap(bbox);
		}
	}

	// Static asserts to make sure operator[] works as intended
	static_assert(offsetof(Tetrahedron, a) == (sizeof(Vector3) * 0u));
	static_assert(offsetof(Tetrahedron, b) == (sizeof(Vector3) * 1u));
	static_assert(offsetof(Tetrahedron, c) == (sizeof(Vector3) * 2u));
	static_assert(offsetof(Tetrahedron, d) == (sizeof(Vector3) * 3u));
}
