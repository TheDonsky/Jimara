#pragma once
#include "Math.h"


namespace Jimara {
	namespace Math {
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
			float volume = 0.0f;
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
			float volume = -1.0f;

			/// <summary> Mass center of the overlap, as defined in ShapeOverlapCenter </summary>
			Vector3 center = Vector3(0.0f);

			/// <summary> Type-cast to ShapeOverlapVolume </summary>
			inline operator ShapeOverlapVolume()const { return ShapeOverlapVolume{ volume }; }

			/// <summary> Type-cast to ShapeOverlapCenter </summary>
			inline operator ShapeOverlapCenter()const { return ShapeOverlapCenter{ center }; }
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
	}
}
