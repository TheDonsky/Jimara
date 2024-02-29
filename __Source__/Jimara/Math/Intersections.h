#pragma once
#include "Math.h"


namespace Jimara {
	namespace Math {
		/// <summary> Error tolerance for some built-in intersections </summary>
		const constexpr float INTERSECTION_EPSILON = 0.000005f;

		/// <summary>
		/// A generic interface for calculating arbitrary 3d geometric shape bounding box
		/// <para/> For scpecific shapes, one should override this function, or define shape.BoundingBox() 
		/// for the relevant engine generics to be able to use them
		/// </summary>
		/// <typeparam name="Shape"> Shape type </typeparam>
		/// <param name="shape"> Geometry </param>
		/// <returns> Minimal bounding box that contains the shape </returns>
		template<typename Shape>
		inline AABB BoundingBox(const Shape& shape) { return shape.BoundingBox(); }

		/// <summary>
		/// Simple wrapper for checking validity and volume of Math::Overlap results used by engine generics & internals
		/// </summary>
		struct ShapeOverlapVolume final {
			/// <summary>
			/// Overlap volume size
			/// <para/> By most in-engine generics, negative, NaN and infinite values will be interpreted as no overlap;
			/// <para/> Zero means a touch for most generics;
			/// <para/> Based on the collection and/or shape pairs, this may represent an area or some weight; physical volume does not always matter.
			/// </summary>
			float volume;

			/// <summary>
			/// Constructor 
			/// </summary>
			/// <param name="v"> Volume </param>
			inline constexpr ShapeOverlapVolume(float v = std::numeric_limits<float>::quiet_NaN()) : volume(v) {}

			/// <summary> Type-cast to volume reference </summary>
			inline constexpr operator const float& ()const { return volume; }

			/// <summary> Type-cast to volume reference </summary>
			inline constexpr operator float& () { return volume; }

			/// <summary> Default check for overlap validity </summary>
			inline operator bool()const { return std::isfinite(volume) && volume >= 0.0f; }
		};

		/// <summary>
		/// Simple wrapper for obtaining mass-center of sorts of the overlapping volume from Math::Overlap results used by engine generics & internals
		/// </summary>
		struct ShapeOverlapCenter final {
			/// <summary> 
			/// 'Mass center' of the overlapping volume
			/// <para/> does not always have to be accurate; general requirenment is for it to be contained inside the overlapping volume, whatever it is
			/// </summary>
			Vector3 center;

			/// <summary>
			/// Constructor 
			/// </summary>
			/// <param name="c"> Center </param>
			inline constexpr ShapeOverlapCenter(const Vector3& c = Vector3(0.0f)) : center(c) {}

			/// <summary> Type-cast to center reference </summary>
			inline constexpr operator const Vector3& ()const { return center; }

			/// <summary> Type-cast to center reference </summary>
			inline constexpr operator Vector3& () { return center; }
		};

		/// <summary>
		/// Result of a generic Math::Overlap call
		/// <para/> For standard Octree/BVH and other engine-specific generic compatibility,
		/// ShapeOverlapResult should have a public type-cast to a valid ShapeOverlapVolume and ShapeOverlapCenter values.
		/// </summary>
		/// <typeparam name="ShapeA"> First shape type </typeparam>
		/// <typeparam name="ShapeB"> Second shape type </typeparam>
		template<typename ShapeA, typename ShapeB>
		struct ShapeOverlapResult {
			/// <summary> Overlap volume for given shapes as defined in ShapeOverlapVolume </summary>
			float volume = std::numeric_limits<float>::quiet_NaN();

			/// <summary> Mass center of the overlap, as defined in ShapeOverlapCenter </summary>
			Vector3 center = Vector3(0.0f);

			/// <summary> Type-cast to ShapeOverlapVolume </summary>
			inline constexpr operator ShapeOverlapVolume()const { return ShapeOverlapVolume{ volume }; }

			/// <summary> Type-cast to ShapeOverlapCenter </summary>
			inline constexpr operator ShapeOverlapCenter()const { return ShapeOverlapCenter{ center }; }

			/// <summary> Default constructor </summary>
			inline constexpr ShapeOverlapResult() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="v"> Volume </param>
			/// <param name="c"> Center </param>
			inline constexpr ShapeOverlapResult(float v, Vector3 c) : volume(v), center(c) {}

			/// <summary>
			/// Copy-contstructor
			/// </summary>
			/// <typeparam name="OverlapType"> Any type that can be statically cast to ShapeOverlapVolume and ShapeOverlapCenter </typeparam>
			/// <param name="other"> Overlap to copy </param>
			template<typename OverlapType>
			inline ShapeOverlapResult(const OverlapType& other) { (*this) = other; }

			/// <summary>
			/// Copy operation
			/// </summary>
			/// <typeparam name="OverlapType"> Any type that can be statically cast to ShapeOverlapVolume and ShapeOverlapCenter </typeparam>
			/// <param name="other"> Overlap to copy </param>
			/// <returns> Self </returns>
			template<typename OverlapType>
			inline ShapeOverlapResult& operator=(const OverlapType& other) {
				volume = static_cast<ShapeOverlapVolume>(other);
				center = static_cast<ShapeOverlapCenter>(other);
				return *this;
			}
		};

		/// <summary>
		/// A generic interface for checking shape intersection/overlap information
		/// <para/> For scpecific shapes, one should override this function, or define a.Overlap(b) 
		/// for the relevant engine generics to be able to use them
		/// </summary>
		/// <typeparam name="ShapeA"> First shape type </typeparam>
		/// <typeparam name="ShapeB"> Second shape type </typeparam>
		/// <param name="a"> First shape</param>
		/// <param name="b"> Second shape</param>
		/// <returns> True, if the shapes share at least a single common point </returns>
		template<typename ShapeA, typename ShapeB>
		inline ShapeOverlapResult<ShapeA, ShapeB> Overlap(const ShapeA& a, const ShapeB& b) { return a.Overlap(b); }

		/// <summary>
		/// A simple wrapper on a floating point value, specifically telling the in-engine utilities 
		/// that the value is the distance of a sweep/raycast operation till the shapes connect.
		/// </summary>
		struct SweepDistance final {
			/// <summary>
			/// Underlying distance
			/// <para/> NaN and/or infinities will be interpreted by the engine as sweep-misses.
			/// <para/> Negative values will generally be interpreted differently by different systems, 
			/// but the logic wold correspond to a "backwards-movement" along the sweep direction, wherever supported.
			/// </summary>
			float distance = std::numeric_limits<float>::quiet_NaN();

			/// <summary>
			/// Constructor 
			/// </summary>
			/// <param name="d"> Distance </param>
			inline constexpr SweepDistance(float d = 0.0f) : distance(d) {}

			/// <summary> Type-cast to distance reference </summary>
			inline constexpr operator const float& ()const { return distance; }

			/// <summary> Type-cast to distance reference </summary>
			inline constexpr operator float& () { return distance; }
		};

		/// <summary>
		/// A simple wrapper on a Vector3d value, specifically telling the in-engine utilities 
		/// what point was hit during the swep/raycast operaton.
		/// <para/> For Raycasts, the only correct behaviour is to return ray.pos + ray.dir * SweepDistance, 
		/// but the in-engine templates may use the sweeps and/or raycasts interchangeably and it still has to be provided most of the time
		/// </summary>
		struct SweepHitPoint final {
			/// <summary> Sweep/Raycast hit position </summary>
			Vector3 position = Vector3(0.0f);

			/// <summary>
			/// Constructor 
			/// </summary>
			/// <param name="p"> Hit Point </param>
			inline constexpr SweepHitPoint(const Vector3& p = Vector3(0.0f)) : position(p) {}

			/// <summary> Type-cast to position reference </summary>
			inline constexpr operator const Vector3& ()const { return position; }

			/// <summary> Type-cast to position reference </summary>
			inline constexpr operator Vector3& () { return position; }
		};

		/// <summary>
		/// Result of a generic Math::Raycast operation
		/// <para/> For standard Octree/BVH and other engine-specific generic compatibility,
		/// RaycastResult should have a public type-cast to valid SweepDistance and SweepHitPoint.
		/// </summary>
		/// <typeparam name="TargetShape"> The shape, upon which the ray is cast upon </typeparam>
		template<typename TargetShape>
		struct RaycastResult {
			/// <summary> Sweep distance, as defined in SweepDistance </summary>
			float distance = std::numeric_limits<float>::quiet_NaN();

			/// <summary> Raycast hit position for SweepHitPoint </summary>
			Vector3 hitPoint = Vector3(0.0f);

			/// <summary> Type-cast to SweepDistance </summary>
			inline operator SweepDistance()const { return SweepDistance{ distance }; }

			/// <summary> Type-cast to SweepHitPoint </summary>
			inline operator SweepHitPoint()const { return SweepHitPoint{ hitPoint }; }

			/// <summary> Default constructor </summary>
			inline RaycastResult() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="v"> Distance </param>
			/// <param name="p"> Hit Point </param>
			inline RaycastResult(float d, Vector3 p) : distance(d), hitPoint(p) {}

			/// <summary>
			/// Copy-contstructor
			/// </summary>
			/// <typeparam name="CastType"> Any type that can be statically cast to SweepDistance and SweepHitPoint </typeparam>
			/// <param name="other"> CastType to copy </param>
			template<typename CastType>
			inline RaycastResult(const CastType& other) { (*this) = other; }

			/// <summary>
			/// Copy operation
			/// </summary>
			/// <typeparam name="CastType"> Any type that can be statically cast to SweepDistance and SweepHitPoint </typeparam>
			/// <param name="other"> Overlap to copy </param>
			/// <returns> Self </returns>
			template<typename CastType>
			inline RaycastResult& operator=(const CastType& other) {
				distance = static_cast<SweepDistance>(other);
				hitPoint = static_cast<SweepHitPoint>(other);
				return *this;
			}
		};

		/// <summary>
		/// A generic raycast operation
		/// <para/> Throws a ray from a given position in given direction and tells if and how it hits the target shape.
		/// <para/> For scpecific shapes, one should override this function, or define shape.Raycast(origin, direction) 
		/// for the relevant engine generics to be able to use them
		/// </summary>
		/// <typeparam name="Shape"> Target shape type </typeparam>
		/// <param name="shape"> Shape the ray is cast upon </param>
		/// <param name="rayOrigin"> Origin of the ray </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Raycast result </returns>
		template<typename Shape>
		inline RaycastResult<Shape> Raycast(const Shape& shape, const Vector3& rayOrigin, const Vector3& direction) {
			return shape.Raycast(rayOrigin, direction);
		}

		/// <summary>
		/// Result of an arbitrary Math::Sweep operation between two shapes
		/// <para/> For standard Octree/BVH and other engine-specific generic compatibility,
		/// SweepResult should have a public type-casts to valid SweepDistance and SweepHitPoint.
		/// </summary>
		/// <typeparam name="ShapeA"> First shape type </typeparam>
		/// <typeparam name="ShapeB"> Second shape type </typeparam>
		template<typename ShapeA, typename ShapeB>
		struct SweepResult {
			/// <summary> Sweep distance, as defined in SweepDistance </summary>
			float distance = std::numeric_limits<float>::quiet_NaN();

			/// <summary> Raycast hit position for SweepHitPoint </summary>
			Vector3 hitPoint = Vector3(0.0f);

			/// <summary> Type-cast to SweepDistance </summary>
			inline operator SweepDistance()const { return SweepDistance{ distance }; }

			/// <summary> Type-cast to SweepHitPoint </summary>
			inline operator SweepHitPoint()const { return SweepHitPoint{ hitPoint }; }

			/// <summary> Default constructor </summary>
			inline SweepResult() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="v"> Distance </param>
			/// <param name="p"> Hit Point </param>
			inline SweepResult(float d, Vector3 p) : distance(d), hitPoint(p) {}

			/// <summary>
			/// Copy-contstructor
			/// </summary>
			/// <typeparam name="CastType"> Any type that can be statically cast to SweepDistance and SweepHitPoint </typeparam>
			/// <param name="other"> CastType to copy </param>
			template<typename CastType>
			inline SweepResult(const CastType& other) { (*this) = other; }

			/// <summary>
			/// Copy operation
			/// </summary>
			/// <typeparam name="CastType"> Any type that can be statically cast to SweepDistance and SweepHitPoint </typeparam>
			/// <param name="other"> Overlap to copy </param>
			/// <returns> Self </returns>
			template<typename CastType>
			inline SweepResult& operator=(const CastType& other) {
				distance = static_cast<SweepDistance>(other);
				hitPoint = static_cast<SweepHitPoint>(other);
				return *this;
			}
		};

		/// <summary>
		/// A generic sweep operation
		/// <para/> "Throws" object A from given position in given direction and tells if and how it hits the object B.
		/// <para/> For scpecific shapes, one should override this function, or define b.Sweep(a, origin, direction) 
		/// for the relevant engine generics to be able to use them
		/// </summary>
		/// <typeparam name="ShapeA"> First shape type </typeparam>
		/// <typeparam name="ShapeB"> Second shape type </typeparam>
		/// <param name="a"> First shape </param>
		/// <param name="b"> Second shape </param>
		/// <param name="position"> Initial position offset of the first shape </param>
		/// <param name="direction"> Sweep direction </param>
		/// <returns> Sweep result </returns>
		template<typename ShapeA, typename ShapeB>
		inline SweepResult<ShapeA, ShapeB> Sweep(const ShapeA& a, const ShapeB& b, const Vector3& position, const Vector3& direction) {
			return b.Sweep(a, direction);
		}


		/// <summary>
		/// Raycast distance to an axis aligned bounding box
		/// </summary>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="rayOrigin"> Ray origin point </param>
		/// <param name="inverseDirection"> 1.0f / ray.direction </param>
		/// <returns> 
		/// NaN, if hit can not occure; 
		/// Hit distance if the ray starts outside the BBox and hits it;
		/// Zero or negative if the ray starts-off inside the bounding box geometry
		/// </returns>
		inline float CastPreInversed(const AABB& bbox, const Vector3& rayOrigin, const Vector3& inverseDirection) {
			static_assert(std::numeric_limits<float>::is_iec559);
			static_assert((0.0f * std::numeric_limits<float>::infinity()) != std::numeric_limits<float>::quiet_NaN());
			static_assert((-0.0f * std::numeric_limits<float>::infinity()) != std::numeric_limits<float>::quiet_NaN());
			static_assert((0.0f * -std::numeric_limits<float>::infinity()) != std::numeric_limits<float>::quiet_NaN());
			static_assert((-0.0f * -std::numeric_limits<float>::infinity()) != std::numeric_limits<float>::quiet_NaN());
			float ds = (bbox.start.x - rayOrigin.x) * inverseDirection.x;
			float de = (bbox.end.x - rayOrigin.x) * inverseDirection.x;
			float mn = Math::Min(ds, de);
			float mx = Math::Max(ds, de);
			ds = (bbox.start.y - rayOrigin.y) * inverseDirection.y;
			de = (bbox.end.y - rayOrigin.y) * inverseDirection.y;
			mn = Math::Max(mn, Math::Min(ds, de));
			mx = Math::Min(mx, Math::Max(ds, de));
			ds = (bbox.start.z - rayOrigin.z) * inverseDirection.z;
			de = (bbox.end.z - rayOrigin.z) * inverseDirection.z;
			mn = Math::Max(mn, Math::Min(ds, de));
			mx = Math::Min(mx, Math::Max(ds, de));
			if (mn > mx + INTERSECTION_EPSILON)
				return std::numeric_limits<float>::quiet_NaN();
			else return mn;
		}

		/// <summary>
		/// Raycast implementation for an AABB
		/// </summary>
		/// <param name="shape"> Bounding box </param>
		/// <param name="rayOrigin"> Ray origin point </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Intersection information </returns>
		template<>
		inline RaycastResult<AABB> Raycast<AABB>(const AABB& shape, const Vector3& rayOrigin, const Vector3& direction) {
			RaycastResult<AABB> rv;
			rv.distance = CastPreInversed(shape, rayOrigin, 1.0f / direction);
			rv.hitPoint = rayOrigin + rv.distance * direction;
			return rv;
		}

		/// <summary>
		/// Checks Vector3 and AABB overlap
		/// <para/> Keep in mind, that for performance reasons, invalid bounding boxes with starts less than ends will always fail the check!
		/// </summary>
		/// <param name="point"> Point </param>
		/// <param name="bbox"> Bounding box </param>
		/// <returns> Overlap information </returns>
		template<>
		inline ShapeOverlapResult<Vector3, AABB> Overlap<Vector3, AABB>(const Vector3& point, const AABB& bbox) {
			ShapeOverlapResult<Vector3, AABB> rv;
			rv.volume = (
				(point.x >= bbox.start.x && point.x <= bbox.end.x) &&
				(point.y >= bbox.start.y && point.y <= bbox.end.y) &&
				(point.z >= bbox.start.z && point.z <= bbox.end.z))
				? 1.0f : std::numeric_limits<float>::quiet_NaN();
			rv.center = point;
			return rv;
		}

		/// <summary>
		/// Checks AABB and Vector3 overlap
		/// <para/> Keep in mind, that for performance reasons, invalid bounding boxes with starts less than ends will always fail the check!
		/// </summary>
		/// <param name="point"> Point </param>
		/// <param name="bbox"> Bounding box </param>
		/// <returns> Overlap information </returns>
		template<>
		inline ShapeOverlapResult<AABB, Vector3> Overlap<AABB, Vector3>(const AABB& bbox, const Vector3& point) {
			return Overlap(point, bbox);
		}
	}
}
