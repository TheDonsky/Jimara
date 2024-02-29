#pragma once
#include "../../Core/JimaraApi.h"
#include "../Intersections.h"

namespace Jimara {
	/// <summary>
	/// Basic triangle, defined as a 3d-vector or vertices
	/// </summary>
	/// <typeparam name="Vertex"> Vertex type </typeparam>
	template<typename Vertex>
	using Triangle = glm::vec<3, Vertex, glm::defaultp>;

	/// <summary> 2d triangle </summary>
	using Triangle2 = Triangle<Vector2>;

	/// <summary> 3d triangle </summary>
	using Triangle3 = Triangle<Vector3>;


	namespace Math {
		/// <summary>
		/// Calculates a bounding box of a 3d triangle
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <returns> AABB </returns>
		template<>
		inline AABB BoundingBox<Triangle3>(const Triangle3& tri) {
			const auto maxAxis = [&](uint32_t axis) { return Max(tri[0u][axis], tri[1u][axis], tri[2u][axis]); };
			const auto minAxis = [&](uint32_t axis) { return Min(tri[0u][axis], tri[1u][axis], tri[2u][axis]); };
			return AABB(
				Vector3(minAxis(0u), minAxis(1u), minAxis(2u)),
				Vector3(maxAxis(0u), maxAxis(1u), maxAxis(2u)));
		}

		/// <summary>
		/// Checks overlap between a triangle and a point
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <param name="point"> Vertex </param>
		/// <returns> Overlap info </returns>
		template<>
		inline ShapeOverlapResult<Triangle3, Vector3> Overlap<Triangle3, Vector3>(const Triangle3& tri, const Vector3& point) {
			const Vector3& a = tri.x;
			const Vector3& b = tri.y;
			const Vector3& c = tri.z;
			if (a == b || b == c || c == a) 
				return {};
			if (point == a || point == b || point == c) 
				return {};
			const Vector3 ab = (b - a);
			const Vector3 bc = (c - b);
			const Vector3 ca = (a - c);
			const Vector3 ax = (point - a);
			const Vector3 bx = (point - b);
			const Vector3 cx = (point - c);
			const constexpr float epsilon = std::numeric_limits<float>::epsilon() * 32.0f;
			const bool overlaps =
				(Dot(ab, ax) / Magnitude(ax) + epsilon) >= (-Dot(ab, ca) / Magnitude(ca)) &&
				(Dot(bc, bx) / Magnitude(bx) + epsilon) >= (-Dot(bc, ab) / Magnitude(ab)) &&
				(Dot(ca, cx) / Magnitude(cx) + epsilon) >= (-Dot(ca, bc) / Magnitude(bc));
			if (!overlaps)
				return {};
			else return ShapeOverlapResult<Triangle3, Vector3>(1.0f, point);
		}

		/// <summary>
		/// Checks overlap between a triangle and a point
		/// </summary>
		/// <param name="point"> Vertex </param>
		/// <param name="tri"> Triangle </param>
		/// <returns> Overlap info </returns>
		template<>
		inline ShapeOverlapResult<Vector3, Triangle3> Overlap<Vector3, Triangle3>(const Vector3& point, const Triangle3& tri) {
			return Overlap<Triangle3, Vector3>(tri, point);
		}

		/// <summary>
		/// Checks intersection/overlap between a triangle and an axis-aligned bounding box
		/// </summary>
		/// <typeparam name="InspectOverlappingBitFn"> Any function, receiving a Triangle3 as and argument and returning a boolean </typeparam>
		/// <param name="tri"> Triangle </param>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="inspectOverlappingBit"> 
		/// During the check, the routine will report some pieces of the triangle using this callback inspectOverlappingBit(subTriangle);
		/// If the call returns false, next pieces (if available) will not be reported and check will terminate there and then.
		/// </param>
		/// <returns> True, if triangle and bbox overlap </returns>
		template<typename InspectOverlappingBitFn>
		inline bool CheckOverlap(const Triangle3& tri, const AABB& bbox, const InspectOverlappingBitFn& inspectOverlappingBit) {
			static const auto crossPoint = [](const Vector3& from, const Vector3& to, float fromV, float toV, float barrier) {
				return from + (to - from) * ((barrier - fromV) / (toV - fromV));
			};

			static const auto sortTriangle = [](Triangle3& t, float am, float bm, float cm) {
				Vector3& a = t.x;
				Vector3& b = t.y;
				Vector3& c = t.z;
				if (am > bm) {
					if (cm > am) std::swap(a, b); // b a c
					else if (bm > cm) std::swap(a, c); // c b a
					else { Vector3 tmp = a; a = b; b = c; c = tmp; } // b c a
				}
				else if (am > cm) { Vector3 tmp = a; a = c; c = b; b = tmp; } // c a b
				else if (bm > cm) std::swap(b, c); // a c b
			};

			static const auto intersectsTri = [](const Triangle3& t, float av, float bv, float cv, float s, float e, const auto& intersectsPart) -> bool {
				if (cv < s) 
					return false; // a b c | | (1)
				if (av > e) 
					return false; // | | a b c (10)
				const Vector3& ta = t.x;
				const Vector3& tb = t.y;
				const Vector3& tc = t.z;
				if (av <= s) {
					const Vector3 asc = crossPoint(ta, tc, av, cv, s);
					if (bv <= s) {
						const Vector3 bsc = crossPoint(tb, tc, bv, cv, s);
						if (cv <= e)
							return intersectsPart(Triangle3(asc, bsc, tc)); // a b | c | (2)
						else { // a b | | c (3)
							const Vector3 bec = crossPoint(tb, tc, bv, cv, e);
							if (intersectsPart(Triangle3(bsc, bec, asc))) 
								return true;
							const Vector3 aec = crossPoint(ta, tc, av, cv, e);
							return(intersectsPart(Triangle3(asc, bec, aec)));
						}
					}
					else if (bv <= e) {
						if (cv <= e) { // a | b c | (4)
							if (intersectsPart(Triangle3(asc, tb, tc))) 
								return true;
							const Vector3 asb = crossPoint(ta, tb, av, bv, s);
							return(intersectsPart(Triangle3(asc, asb, tb)));
						}
						else { // a | b | c (5)
							const Vector3 asb = crossPoint(ta, tb, av, bv, s);
							const Vector3 bec = crossPoint(tb, tc, bv, cv, e);
							if (intersectsPart(Triangle3(asb, tb, bec))) 
								return true;
							if (intersectsPart(Triangle3(asc, asb, bec))) 
								return true;
							const Vector3 aec = crossPoint(ta, tc, av, cv, e);
							return intersectsPart(Triangle3(asc, bec, aec));
						}
					}
					else { // a | | b c (6)
						const Vector3 asb = crossPoint(ta, tb, av, bv, s);
						const Vector3 aeb = crossPoint(ta, tb, av, bv, e);
						if (intersectsPart(Triangle3(asc, asb, aeb))) 
							return true;
						const Vector3 aec = crossPoint(ta, tc, av, cv, e);
						return intersectsPart(Triangle3(asc, aeb, aec));
					}
				}
				else {
					if (cv <= e)
						return intersectsPart(t); // | a b c | (7)
					else {
						const Vector3 aec = crossPoint(ta, tc, av, cv, e);
						if (bv <= e) { // | a b | c (8)
							const Vector3 bec = crossPoint(tb, tc, bv, cv, e);
							if (intersectsPart(Triangle3(ta, tb, bec))) 
								return true;
							return intersectsPart(Triangle3(ta, aec, bec));
						}
						else { // | a | b c (9)
							const Vector3 aeb = crossPoint(ta, tb, av, bv, e);
							return intersectsPart(Triangle3(ta, aeb, aec));
						}
					}
				}
			};

			const auto intersectsTriAxis = [&](Triangle3 t, uint8_t axis, const auto& nextAxis) -> bool {
				Vector3& a = t.x;
				Vector3& b = t.y;
				Vector3& c = t.z;
				sortTriangle(t, a[axis], b[axis], c[axis]);
				return intersectsTri(t, a[axis], b[axis], c[axis], bbox.start[axis], bbox.end[axis], nextAxis);
			};

			bool intersects = false;
			intersectsTriAxis(tri, 2u, [&](const Triangle3& t0) {
				return intersectsTriAxis(t0, 0u, [&](const Triangle3& t1) {
					return intersectsTriAxis(t1, 1u, [&](const Triangle3& t) {
						intersects = true;
						return inspectOverlappingBit(t);
						});
					});
				});
			return intersects;
		}

		/// <summary>
		/// Checks intersection/overlap between a triangle and an axis-aligned bounding box
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <param name="bbox"> Bounding box </param>
		/// <returns> True, if triangle and bbox overlap </returns>
		inline bool CheckOverlap(const Triangle3& tri, const AABB& bbox) {
			return CheckOverlap(tri, bbox, [&](const Triangle3&) { return true; });
		}

		/// <summary>
		/// Calculates triangle and bbox overlap information
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <param name="bbox"> Bounding box </param>
		/// <returns> Overlap information </returns>
		template<>
		inline ShapeOverlapResult<Triangle3, AABB> Overlap<Triangle3, AABB>(const Triangle3& tri, const AABB& bbox) {
			static const auto centerOf = [](const Triangle3& t) { return (t[0u] + t[1u] + t[2u]) * (1.0f / 3.0f); };
			ShapeOverlapResult<Triangle3, AABB> result;
			result.volume = 0.0f;
			result.center = (bbox.start + bbox.end) * 0.5f;
			const auto inspectTri = [&](const Triangle3& t) {
				const float volume = 0.5f * Magnitude(Cross(t[1u] - t[0u], t[2u] - t[0u]));
				if (volume > 0.0f) {
					const float totalVolume = result.volume + volume;
					result.center = Lerp(result.center, centerOf(t), volume / totalVolume);
					result.volume = totalVolume;
				}
				return false;
			};
			if (!CheckOverlap(tri, bbox, inspectTri))
				return {};
			return result;
		}

		/// <summary>
		/// Calculates triangle and bbox overlap information
		/// </summary>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="tri"> Triangle </param>
		/// <returns> Overlap information </returns>
		template<>
		inline ShapeOverlapResult<AABB, Triangle3> Overlap<AABB, Triangle3>(const AABB& bbox, const Triangle3& tri) {
			return Overlap<Triangle3, AABB>(tri, bbox);
		}

		/// <summary>
		/// Calculates triangle-ray cast operation
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <param name="rayOrigin"> Origin of the ray </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="clipBackface"> If true, backface-hit should be ignored </param>
		/// <returns> Raycast information </returns>
		inline RaycastResult<Triangle3> Raycast(const Triangle3& tri, const Vector3& rayOrigin, const Vector3& direction, bool clipBackface) {
			const Vector3& a = tri.x;
			const Vector3& b = tri.y;
			const Vector3& c = tri.z;
			Vector3 normal = Cross(b - a, c - a);
			const float deltaProjection = Dot(a - rayOrigin, normal);
			if (clipBackface && (deltaProjection < -std::numeric_limits<float>::epsilon())) 
				return {};
			const float dirProjection = Dot(direction, normal);
			if (deltaProjection * dirProjection <= 0.0f) 
				return {};
			const float distance = deltaProjection / dirProjection;
			const Vector3 hitVert = rayOrigin + direction * distance;
			const ShapeOverlapVolume volume = Overlap(tri, hitVert);
			if (volume)
				return RaycastResult<Triangle3>(distance, hitVert);
			else return {};
		}

		/// <summary>
		/// Calculates triangle-ray cast operation
		/// <para/> By default, backface-clip option is disabled
		/// </summary>
		/// <param name="tri"> Triangle </param>
		/// <param name="rayOrigin"> Origin of the ray </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Raycast information </returns>
		template<>
		inline RaycastResult<Triangle3> Raycast<Triangle3>(const Triangle3& tri, const Vector3& rayOrigin, const Vector3& direction) {
			Raycast(tri, rayOrigin, direction, false);
		}
	}
}
