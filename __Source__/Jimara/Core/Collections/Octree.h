#pragma once
#include "../../Math/Primitives/PosedAABB.h"
#include <vector>
#include <memory>


namespace Jimara {
	/// <summary>
	/// A generic Octree
	/// <para/> This wraps around a smart pointer to the immutable underlying data, so copying Octrees is a trivial operation
	/// </summary>
	/// <typeparam name="Type"> Stored geometric element type </typeparam>
	template<typename Type>
	class Octree {
	public:
		/// <summary>
		/// Hint, expected to be returned from inspectHit and onLeafHitsFinished 
		/// callbacks passed to generic cast/sweep functions
		/// </summary>
		enum class CastHint : uint8_t {
			/// <summary> If inspectHit or onLeafHitsFinished callbacks return this value, no further hit will be reported </summary>
			STOP_CAST = 0u,

			/// <summary> If inspectHit or onLeafHitsFinished callbacks return this value, cast/sweep will continue to the next hits </summary>
			CONTINUE_CAST = 1u
		};

		/// <summary>
		/// A generic Cast call result
		/// </summary>
		/// <typeparam name="HitInfo"> Hit information (depends on sweep geometry, or for Raycasts, can just be standard RaycastResult) </typeparam>
		template<typename HitInfo>
		struct CastResult {
			/// <summary> Hit information (distance from here may not be the 'real' distance, since the cast/sweep query origin will move around) </summary>
			HitInfo hit = {};

			/// <summary> Piece of geometry that got hit </summary>
			const Type* target = nullptr;

			/// <summary> Total hit distance from query origin </summary>
			float totalDistance = std::numeric_limits<float>::quiet_NaN();

			/// <summary> Type-cast to total hit distance </summary>
			inline operator Math::SweepDistance()const { return totalDistance; }

			/// <summary> Type-cast to hit point </summary>
			inline operator Math::SweepHitPoint()const { return hit; }

			/// <summary> Type-cast to geometry that got hit (unsafe, if the geometry is null) </summary>
			inline operator const Type& ()const { return *target; }

			/// <summary> True, if CastResult is valid (ei if target is not null; Queries will never return invalid HitInfo-s with valid targets) </summary>
			inline operator bool()const { return target != nullptr; }
		};

		/// <summary> Result of raycast calls </summary>
		using RaycastResult = CastResult<Math::RaycastResult<Type>>;

		/// <summary>
		/// Result of Sweep calls
		/// </summary>
		/// <typeparam name="Shape"> Sweep shape type </typeparam>
		template<typename Shape>
		using SweepResult = CastResult<Math::SweepResult<Shape, Type>>;

		/// <summary> Constructor (creates an empty Octree) </summary>
		inline Octree();

		/// <summary> Destructor </summary>
		inline ~Octree();

		/// <summary>
		/// Builds an Octree
		/// </summary>
		/// <typeparam name="TypeIt"> Arbitary type that can be used as an iterator for Type objects </typeparam>
		/// <typeparam name="CalculateShapeBBox"> Callable, that returns bounding box of a Type shape </typeparam>
		/// <typeparam name="GetBBoxOverlapInfo"> Callable, that returns an overlap information of Type and AABB </typeparam>
		/// <param name="start"> Iterator to the first element </param>
		/// <param name="end"> Iterator to the sentinel element </param>
		/// <param name="calculateShapeBBox"> Callable, that returns bounding box of a Type shape (AABB box = calculateShapeBBox(const Type&#38;)) </param>
		/// <param name="getBBoxOverlapInfo"> Callable, that returns an overlap information of Type and AABB 
		/// (getBBoxOverlapInfo(const Type&#38;, const AABB&#38;) should return something that satisfies requirenments of Math&#58;&#58;Overlap&#60;Type, AABB&#62;) 
		/// </param>
		/// <returns> New Octree </returns>
		template<typename TypeIt, typename CalculateShapeBBox, typename GetBBoxOverlapInfo>
		inline static Octree Build(
			const TypeIt& start, const TypeIt& end,
			const CalculateShapeBBox& calculateShapeBBox,
			const GetBBoxOverlapInfo& getBBoxOverlapInfo);

		/// <summary>
		/// Builds an Octree
		/// </summary>
		/// <typeparam name="TypeIt"> Arbitary type that can be used as an iterator for Type objects </typeparam>
		/// <param name="start"> Iterator to the first element </param>
		/// <param name="end"> Iterator to the sentinel element </param>
		/// <returns> New Octree </returns>
		template<typename TypeIt>
		inline static Octree Build(const TypeIt& start, const TypeIt& end);

		/// <summary> Octree's combined bounding box </summary>
		inline AABB BoundingBox()const;

		/// <summary> Stored element count </summary>
		inline size_t Size()const;

		/// <summary> 
		/// Stored element by index (order is the same as during the initial Build command)
		/// </summary>
		/// <param name="index"> Element index </param>
		/// <returns> Element </returns>
		inline const Type& operator[](size_t index)const;

		/// <summary>
		/// Returns index of given element
		/// <para/> Element, passed as the argument has to be returned internally by 
		/// any of the operator[]/Cast/Raycast/Sweep operations via public API. Otherwise, the behaviour is undefined.
		/// <para/> Do not assume the Octree knows how to compare actual primitives during this, 
		/// this is just for letting the user know the index from generation time, when the query result returns an internal pointer.
		/// </summary>
		/// <param name="element"> Element pointer </param>
		/// <returns> Index of the element based on it's index during the initial Build </returns>
		inline size_t IndexOf(const Type* element)const;

		/// <summary>
		/// Generic cast function inside the Octree
		/// <para/> Hits are reported through inspectHit callback; distances are not sorted by default;
		/// <para/> Reports are "interrupted" with onLeafHitsFinished calls, which gets invoked after several successful inspectHit
		/// calls, before leaving the bucket. It's more or less guaranteed that normal Raycasts will go through the buckets in sorted order,
		/// even if the contacts from inside the buckets may be reported out of order.
		/// </summary>
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information (should return CastHint) </typeparam>
		/// <typeparam name="OnLeafHitsFinished"> Callable, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </typeparam>
		/// <typeparam name="SweepAgainstAABB"> Callable, that calculates Sweep/Raycast information between the cast shape/ray and an AABB </typeparam>
		/// <typeparam name="SweepAgainstGeometry"> Callable, that calculates Sweep/Raycast information between the cast shape/ray and target Type geometry </typeparam>
		/// <param name="position"> Raycast/Sweep/Whatever origin/offset </param>
		/// <param name="direction"> Cast direction </param>
		/// <param name="inspectHit"> Callback, that provides user with an intersection information 
		/// (should return CastHint; receives result from sweepAgainstGeometry, total sweep distance and const reference to the geometry that got hit)
		/// </param>
		/// <param name="onLeafHitsFinished"> Callback type, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </param>
		/// <param name="sweepAgainstAABB"> Callable, that calculates Sweep/Raycast information between the cast shape/ray and an AABB 
		/// (Should satisfy the same requirenments as Math&#58;&#58;Raycast&#60;AABB&#62; or Math&#58;&#58;Sweep&#60;Shape, AABB&#62;)
		/// </param>
		/// <param name="sweepAgainstGeometry"> Callable, that calculates Sweep/Raycast information between the cast shape/ray and target Type geometry
		/// (Should satisfy the same requirenments as Math&#58;&#58;Raycast&#60;Type&#62; or Math&#58;&#58;Sweep&#60;Shape, Type&#62;)
		/// </param>
		template<typename InspectHit, typename OnLeafHitsFinished, 
			typename SweepAgainstAABB, typename SweepAgainstGeometry>
		inline void Cast(
			const Vector3& position, const Vector3& direction,
			const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished,
			const SweepAgainstAABB& sweepAgainstAABB, const SweepAgainstGeometry& sweepAgainstGeometry)const;

		/// <summary>
		/// Generic raycast function inside the Octree
		/// <para/> Hits are reported through inspectHit callback; distances are not sorted by default;
		/// <para/> Reports are "interrupted" with onLeafHitsFinished calls, which gets invoked after several successful inspectHit
		/// calls, before leaving the bucket. It's more or less guaranteed that normal Raycasts will go through the buckets in sorted order,
		/// even if the contacts from inside the buckets may be reported out of order.
		/// </summary>
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information
		/// (should return CastHint; receives result from sweepAgainstGeometry, total sweep distance and const reference to the geometry that got hit)
		/// </typeparam>
		/// <typeparam name="OnLeafHitsFinished"> Callable, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </typeparam>
		/// <param name="position"> Ray origin </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="inspectHit"> Callback, that provides user with an intersection information (should return CastHint) </param>
		/// <param name="onLeafHitsFinished"> Callback type, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </param>
		template<typename InspectHit, typename OnLeafHitsFinished>
		inline void Raycast(
			const Vector3& position, const Vector3& direction, 
			const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished)const;

		/// <summary>
		/// Generic sweep function inside the Octree
		/// <para/> Hits are reported through inspectHit callback; distances are not sorted by default;
		/// <para/> Reports are "interrupted" with onLeafHitsFinished calls, which gets invoked after several successful inspectHit
		/// calls, before leaving the bucket. It's more or less guaranteed that normal Raycasts will go through the buckets in sorted order,
		/// even if the contacts from inside the buckets may be reported out of order.
		/// </summary>
		/// <typeparam name="Shape"> 'Swept' geometry type </typeparam>
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information 
		/// (should return CastHint; receives result from sweepAgainstGeometry, total sweep distance and const reference to the geometry that got hit) 
		/// </typeparam>
		/// <typeparam name="OnLeafHitsFinished"> Callable, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </typeparam>
		/// <param name="shape"> 'Swept' shape </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="inspectHit"> Callback, that provides user with an intersection information (should return CastHint) </param>
		/// <param name="onLeafHitsFinished"> Callback type, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </param>
		template<typename Shape, typename InspectHit, typename OnLeafHitsFinished>
		inline void Sweep(
			const Shape& shape, const Vector3& position, const Vector3& direction,
			const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished)const;

		/// <summary>
		/// Raycast, reporting the closest hit
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="result"> Result will be stored here </param>
		/// <returns> True, if the ray hits anything </returns>
		inline bool Raycast(const Vector3& position, const Vector3& direction, RaycastResult& result)const;

		/// <summary>
		/// Raycast, reporting the closest hit
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Closest hit if found, null-target otherwise </returns>
		inline RaycastResult Raycast(const Vector3& position, const Vector3& direction)const;

		/// <summary>
		/// Raycast, reporting all hits
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="result"> Hits will be appended to this list </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <returns> Number of reported hits </returns>
		inline size_t RaycastAll(const Vector3& position, const Vector3& direction, std::vector<RaycastResult>& result, bool sort = false)const;

		/// <summary>
		/// Raycast, reporting all hits
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <returns> All hits </returns>
		inline std::vector<RaycastResult> RaycastAll(const Vector3& position, const Vector3& direction, bool sort = false)const;

		/// <summary>
		/// Sweep, reporting the closest hit
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="result"> Hit information will be stored here </param>
		/// <returns> True, if the shape hits anything </returns>
		template<typename Shape>
		inline bool Sweep(const Shape& shape, const Vector3& position, const Vector3& direction, SweepResult<Shape>& result)const;

		/// <summary>
		/// Sweep, reporting the closest hit
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <returns> Closest hit if found, null-target otherwise </returns>
		template<typename Shape>
		inline SweepResult<Shape> Sweep(const Shape& shape, const Vector3& position, const Vector3& direction)const;

		/// <summary>
		/// Sweep, reporting all contacts
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="result"> Hits will be appended to this list </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <returns> Number of reported hits </returns>
		template<typename Shape>
		inline size_t SweepAll(const Shape& shape, const Vector3& position, const Vector3& direction, std::vector<SweepResult<Shape>>& result, bool sort = false)const;

		/// <summary>
		/// Sweep, reporting all contacts
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <returns> All hits </returns>
		template<typename Shape>
		inline std::vector<SweepResult<Shape>> SweepAll(const Shape& shape, const Vector3& position, const Vector3& direction, bool sort = false)const;


	private:
		// Internals:
		struct Node;
		using NodeRef = const Node*;
		using TypeRef = const Type*;
		struct Node {
			AABB bounds;
			NodeRef children[8u] = { nullptr };
			const TypeRef* elements = nullptr;
			size_t elemCount = 0u;
			NodeRef parentNode = nullptr;
			uint8_t indexInParent = static_cast<uint8_t>(0u);
		};
		struct Data {
			std::vector<Type> elements;
			std::vector<TypeRef> nodeElements;
			std::vector<Node> nodes;
		};
		std::shared_ptr<const Data> m_data;
		inline std::shared_ptr<const Data> GetData()const { return m_data; }

		// Actual constructor with data:
		inline Octree(const std::shared_ptr<const Data>& data) : m_data(data) {}

	public:
		/// <summary>
		/// Standard casting strategy, picking the closest hit result
		/// <para/> This is mainly used for internal purposes, but is made public to simplify writing similar logic
		/// </summary>
		/// <typeparam name="HitType"> Hit type, reported by sweepAgainstGeometry call </typeparam>
		/// <typeparam name="CastFn"> Callable, that performs the underlying cast call </typeparam>
		/// <param name="castFn"> Underlying Cast call, that will simply receive inspectHit and onLeafHitsFinished functions </param>
		/// <param name="result"> Result will be stored here </param>
		/// <returns> True, if any hit gets reported </returns>
		template<typename HitType, typename CastFn>
		inline static bool CastClosest(const CastFn& castFn, CastResult<HitType>& result) {
			float bestDistance = std::numeric_limits<float>::quiet_NaN();
			castFn(
				[&](const HitType& hit, float totalDistance, const Type& target) {
					if (std::isnan(bestDistance) || (totalDistance < bestDistance)) {
						result = CastResult<HitType>{ hit, &target, totalDistance };
						bestDistance = totalDistance;
					}
					return CastHint::CONTINUE_CAST;
				}, 
				[&]() { 
					assert(!std::isnan(bestDistance));
					return CastHint::STOP_CAST; 
				});
			return !std::isnan(bestDistance);
		}

		/// <summary>
		/// Standard casting strategy, picking the closest hit result
		/// <para/> This is mainly used for internal purposes, but is made public to simplify writing similar logic
		/// </summary>
		/// <typeparam name="HitType"> Hit type, reported by sweepAgainstGeometry call </typeparam>
		/// <typeparam name="CastFn"> Callable, that performs the underlying cast call </typeparam>
		/// <param name="castFn"> Underlying Cast call, that will simply receive inspectHit and onLeafHitsFinished functions </param>
		/// <param name="result"> Result will be stored here </param>
		/// <returns> Closest hit result </returns>
		template<typename HitType, typename CastFn>
		inline static CastResult<HitType> CastClosest(const CastFn& castFn) {
			CastResult<HitType> rv = {};
			CastClosest<HitType, CastFn>(castFn, rv);
			return rv;
		}

		/// <summary>
		/// Standard casting strategy, collecting all hits along the cast path
		/// <para/> This is mainly used for internal purposes, but is made public to simplify writing similar logic
		/// </summary>
		/// <typeparam name="HitType"> Hit type, reported by sweepAgainstGeometry call </typeparam>
		/// <typeparam name="CastFn"> Callable, that performs the underlying cast call </typeparam>
		/// <param name="castFn"> Underlying Cast call, that will simply receive inspectHit and onLeafHitsFinished functions </param>
		/// <param name="result"> Result will be appended to this list </param>
		/// <param name="sort"> If true, the results will be sorted </param>
		/// <returns> Number of reported hits </returns>
		template<typename HitType, typename CastFn>
		inline static size_t CastAll(const CastFn& castFn, std::vector<CastResult<HitType>>& result, bool sort) {
			const size_t initialCount = result.size();
			size_t lastLeafStart = initialCount;
			castFn(
				[&](const HitType& hit, float totalDistance, const Type& target) {
					result.push_back(CastResult<HitType> { hit, &target, totalDistance });
					return CastHint::CONTINUE_CAST;
				},
				[&]() {
					if (sort && (result.size() - lastLeafStart) > 1u)
						std::sort(result.data() + lastLeafStart, result.data() + result.size(),
							[&](const CastResult<HitType>& a, const CastResult<HitType>& b) {
								Math::SweepDistance distA = a;
								Math::SweepDistance distB = b;
								return distA.distance < distB.distance;
							});
					lastLeafStart = result.size();
					return CastHint::CONTINUE_CAST;
				});
			return result.size() - initialCount;
		}

		/// <summary>
		/// Standard casting strategy, collecting all hits along the cast path
		/// <para/> This is mainly used for internal purposes, but is made public to simplify writing similar logic
		/// </summary>
		/// <typeparam name="HitType"> Hit type, reported by sweepAgainstGeometry call </typeparam>
		/// <typeparam name="CastFn"> Callable, that performs the underlying cast call </typeparam>
		/// <param name="castFn"> Underlying Cast call, that will simply receive inspectHit and onLeafHitsFinished functions </param>
		/// <param name="result"> Result will be appended to this list </param>
		/// <param name="sort"> If true, the results will be sorted </param>
		/// <returns> List of reported hits </returns>
		template<typename HitType, typename CastFn>
		inline static std::vector<CastResult<HitType>> CastAll(const CastFn& castFn, bool sort) {
			std::vector<CastResult<HitType>> result;
			CastAll<HitType, CastFn>(castFn, result, sort);
			return result;
		}
	};



	template<typename Type> struct PosedOctree;

	namespace Math {
		/// <summary>
		/// Overlap between an octree and a bounding box
		/// <para/> For performance reasons, invalid bounding boxes with starts greater than ends will always fail the check!
		/// <para/> Also, for performance reasons, this only compares the boundaries; this can not tell the user if there are any actual element overlaps.
		/// </summary>
		/// <typeparam name="Type"> Octree type </typeparam>
		/// <param name="octree"> Octree </param>
		/// <param name="bbox"> Bounding box </param>
		/// <returns> Overlap info </returns>
		template<typename Type>
		inline ShapeOverlapResult<Octree<Type>, AABB> Overlap(const Octree<Type>& octree, const AABB& bbox) {
			const AABB octreeBBox = octree.BoundingBox();
			if (octreeBBox.start == octreeBBox.end)
				return {};
			return Overlap(octreeBBox, bbox);
		}

		/// <summary>
		/// Overlap between an octree and a bounding box
		/// <para/> For performance reasons, invalid bounding boxes with starts greater than ends will always fail the check!
		/// <para/> Also, for performance reasons, this only compares the boundaries; this can not tell the user if there are any actual element overlaps.
		/// </summary>
		/// <typeparam name="Type"> Octree type </typeparam>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="octree"> Octree </param>
		/// <returns> Overlap info </returns>
		template<typename Type>
		inline ShapeOverlapResult<AABB, Octree<Type>> Overlap(const AABB& bbox, const Octree<Type>& octree) {
			return Overlap(octree, bbox);
		}

		/// <summary>
		/// Octree Raycast result
		/// </summary>
		/// <typeparam name="Type"> Octree type </typeparam>
		template<typename Type>
		struct RaycastResult<Octree<Type>> : public Octree<Type>::RaycastResult {
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="src"> Result to copy </param>
			inline RaycastResult(const typename Octree<Type>::RaycastResult& src = {}) : Octree<Type>::RaycastResult(src) {}
		};

		/// <summary>
		/// Octree Raycast result
		/// </summary>
		/// <typeparam name="Type"> Octree type </typeparam>
		template<typename SweptType, typename OctreeType>
		struct SweepResult<SweptType, Octree<OctreeType>> : public Octree<OctreeType>::template SweepResult<SweptType> {
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="src"> Result to copy </param>
			inline SweepResult(const typename Octree<OctreeType>::template SweepResult<SweptType>& src = {}) 
				: Octree<OctreeType>::template SweepResult<SweptType>(src) {}
		};


		/// <summary>
		/// PosedOctree Raycast result
		/// </summary>
		/// <typeparam name="Type"> Stored geometry type </typeparam>
		template<typename Type>
		struct RaycastResult<PosedOctree<Type>> {
			/// <summary> Pose-space hit information </summary>
			Math::RaycastResult<Type> localHit = {};

			/// <summary> Piece of geometry that got hit </summary>
			const Type* target = nullptr;

			/// <summary> World-space hit distance from query origin </summary>
			float distance = std::numeric_limits<float>::quiet_NaN();

			/// <summary> World-space hit point </summary>
			Vector3 hitPoint = {};

			/// <summary> Type-cast to total hit distance </summary>
			inline operator Math::SweepDistance()const { return distance; }

			/// <summary> Type-cast to hit point </summary>
			inline operator Math::SweepHitPoint()const { return hitPoint; }

			/// <summary> Type-cast to geometry that got hit (unsafe, if the geometry is null) </summary>
			inline operator const Type& ()const { return *target; }

			/// <summary> True, if CastResult is valid (ei if target is not null; Queries will never return invalid HitInfo-s with valid targets) </summary>
			inline operator bool()const { return target != nullptr; }
		};
	}



	/// <summary>
	/// Posed/Transformed octree
	/// </summary>
	/// <typeparam name="Type"> Octree element type </typeparam>
	template<typename Type>
	struct PosedOctree {
		/// <summary> Geometry </summary>
		Octree<Type> octree;

		/// <summary> Octree transform </summary>
		Matrix4 pose = Math::Identity();

		/// <summary> Constructor </summary>
		inline PosedOctree() {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="o"> Octree </param>
		/// <param name="p"> Pose </param>
		inline PosedOctree(const Octree<Type>& o, const Matrix4& p) : octree(o), pose(p) {}

		/// <summary> Bounding box of the transformed Octree </summary>
		inline AABB BoundingBox()const { return pose * octree.BoundingBox(); }

		/// <summary>
		/// Calculates overlap info between a PosedOctree and a bounding box
		/// </summary>
		/// <param name="bbox"> Axis aligned bounding box </param>
		/// <returns> Overlap information </returns>
		inline Math::ShapeOverlapResult<PosedOctree, AABB> Overlap(const AABB& bbox)const {
			PosedAABB posedBBox = { octree.BoundingBox(), pose };
			return posedBBox.Overlap(bbox);
		}

		/// <summary>
		/// Performs a Raycast against a posed octree
		/// </summary>
		/// <param name="rayOrigin"> Ray's origin point </param>
		/// <param name="direction"> Ray direction </param>
		/// <returns> Raycast info </returns>
		inline Math::RaycastResult<PosedOctree> Raycast(const Vector3& rayOrigin, const Vector3& direction)const {
			const Matrix4 inverseTransform = Math::Inverse(pose);
			typename Octree<Type>::RaycastResult localResult = octree.Raycast(
				Vector3(inverseTransform * Vector4(rayOrigin, 1.0f)),
				Math::Normalize(Vector3(inverseTransform * Vector4(direction, 0.0f))));
			if (!localResult)
				return {};
			Math::RaycastResult<PosedOctree> rv = {};
			rv.localHit = std::move(localResult.hit);
			rv.hitPoint = pose * Vector4(static_cast<Vector3>(static_cast<Math::SweepHitPoint>(rv.localHit)), 1.0f);
			rv.distance = Math::Magnitude(rv.hitPoint - rayOrigin);
			rv.target = localResult.target;
			return rv;
		}
		// __TODO__: Define Raycast and sweep in world-space somehow..
	};

	namespace Math {
		/// <summary>
		/// Overlap between a posed octree and a bounding box
		/// <para/> For performance reasons, invalid bounding boxes with starts greater than ends will always fail the check!
		/// <para/> Also, for performance reasons, this only compares the boundaries; this can not tell the user if there are any actual element overlaps.
		/// </summary>
		/// <typeparam name="Type"> Octree type </typeparam>
		/// <param name="bbox"> Bounding box </param>
		/// <param name="octree"> Octree </param>
		/// <returns> Overlap info </returns>
		template<typename Type>
		inline ShapeOverlapResult<AABB, PosedOctree<Type>> Overlap(const AABB& bbox, const PosedOctree<Type>& octree) {
			return Overlap(octree, bbox);
		}
	}





	template<typename Type>
	inline Octree<Type>::Octree() {}

	template<typename Type>
	inline Octree<Type>::~Octree() {}

	template<typename Type>
	template<typename TypeIt, typename CalculateShapeBBox, typename GetBBoxOverlapInfo>
	inline Octree<Type> Octree<Type>::Build(
		const TypeIt& start, const TypeIt& end,
		const CalculateShapeBBox& calculateShapeBBox,
		const GetBBoxOverlapInfo& getBBoxOverlapInfo) {

		// Basic functions, needed for construction:
		struct BuildContext {
			const std::shared_ptr<Data> data;
			const decltype(calculateShapeBBox) getBoundingBox;
			const decltype(getBBoxOverlapInfo) getOverlapInfo;
			const size_t nodeSplitThreshold;
			const size_t maxDepth;
			const float aabbEpsilon;
			std::vector<AABB> elemBounds;
			bool splitInIntersectionCenter;
			bool splitInIntersectionCenterWeightedByVolume;
			bool splitInCenterIfIntersectionCenterNotValid;
			bool shrinkNodes;
		};
		BuildContext context = {
			std::make_shared<Data>(),
			calculateShapeBBox,
			getBBoxOverlapInfo,
			size_t(2u),
			size_t(24u),
			Math::INTERSECTION_EPSILON * 8.0f,
			std::vector<AABB>(),
			false,	// splitInIntersectionCenter
			false,	// splitInIntersectionCenterWeightedByVolume
			true,	// splitInCenterIfIntersectionCenterNotValid
			true	// shrinkNodes
		};

		// Collect elements and individual bounding boxes:
		const std::shared_ptr<Data>& data = context.data;
		for (TypeIt ptr = start; ptr != end; ++ptr) {
			data->elements.push_back(*ptr);
			const Type& elem = data->elements.back();
			const AABB bnd = context.getBoundingBox(elem);
			context.elemBounds.push_back(AABB(
				Vector3(Math::Min(bnd.start.x, bnd.end.x), Math::Min(bnd.start.y, bnd.end.y), Math::Min(bnd.start.z, bnd.end.z)),
				Vector3(Math::Max(bnd.start.x, bnd.end.x), Math::Max(bnd.start.y, bnd.end.y), Math::Max(bnd.start.z, bnd.end.z))));
		}

		// Collect root-node elements (ei almost all, with the exception of a few that did not have valid boundaries):
		std::vector<TypeRef> rootElems;
		for (size_t i = 0u; i < data->elements.size(); i++) {
			const AABB& bnd = context.elemBounds[i];
			if (std::isfinite(bnd.start.x) && std::isfinite(bnd.start.y) && std::isfinite(bnd.start.z) &&
				std::isfinite(bnd.end.x) && std::isfinite(bnd.end.y) && std::isfinite(bnd.end.z))
				rootElems.push_back(data->elements.data() + i);
		}
		if (rootElems.size() <= 0u)
			return {};

		// Calculate total geometry boundary
		AABB totalBoundary = context.elemBounds[rootElems[0] - data->elements.data()];
		for (size_t i = 1u; i < rootElems.size(); i++) {
			const AABB& bbox = context.elemBounds[rootElems[i] - data->elements.data()];
			totalBoundary.start.x = Math::Min(totalBoundary.start.x, bbox.start.x);
			totalBoundary.start.y = Math::Min(totalBoundary.start.y, bbox.start.y);
			totalBoundary.start.z = Math::Min(totalBoundary.start.z, bbox.start.z);
			totalBoundary.end.x = Math::Max(totalBoundary.end.x, bbox.end.x);
			totalBoundary.end.y = Math::Max(totalBoundary.end.y, bbox.end.y);
			totalBoundary.end.z = Math::Max(totalBoundary.end.z, bbox.end.z);
		}

		// Create node structure:
		{
			struct NodeBuilder {
				static void InsertNodes(const BuildContext& buildContext, AABB nodeBounds, 
						std::vector<TypeRef>& elemBuffer, size_t elemBufferStart, size_t depth) {
					const size_t elemBufferEnd = elemBuffer.size();
					const size_t elemCount = (elemBufferEnd - elemBufferStart);

					// Pre-Shrink node bounds:
					if (elemCount > 0u && buildContext.shrinkNodes) {
						AABB contentBoundary = buildContext.elemBounds[elemBuffer[elemBufferStart] - buildContext.data->elements.data()];
						for (size_t i = elemBufferStart + 1u; i < elemBufferEnd; i++) {
							size_t elementIndex = elemBuffer[i] - buildContext.data->elements.data();
							const AABB& boundary = buildContext.elemBounds[elementIndex];
							contentBoundary = AABB(
								Vector3(
									Math::Min(contentBoundary.start.x, boundary.start.x),
									Math::Min(contentBoundary.start.x, boundary.start.x),
									Math::Min(contentBoundary.start.x, boundary.start.x)),
								Vector3(
									Math::Max(contentBoundary.end.x, boundary.end.x),
									Math::Max(contentBoundary.end.x, boundary.end.x),
									Math::Max(contentBoundary.end.x, boundary.end.x)));
						}
						auto fit = [](float v, float mn, float mx) { return Math::Min(Math::Max(mn, v), mx); };
						auto fit3 = [&](const Vector3& v) {
							return Vector3(
								fit(v.x, nodeBounds.start.x, nodeBounds.end.x),
								fit(v.y, nodeBounds.start.y, nodeBounds.end.y),
								fit(v.z, nodeBounds.start.z, nodeBounds.end.z));
						};
						contentBoundary = AABB(fit3(contentBoundary.start), fit3(contentBoundary.end));
					}

					// In any case, we do need to insert the node:
					const size_t nodeIndex = buildContext.data->nodes.size();
					{
						Node& node = buildContext.data->nodes.emplace_back();
						node.bounds = nodeBounds;
						for (size_t i = 0u; i < 8u; i++)
							node.children[i] = nullptr;
						node.elements = nullptr;
						node.elemCount = 0u;
					}
					auto curNode = [&]() -> Node& { return buildContext.data->nodes[nodeIndex]; };

					// If the nodes fit, we just place them in a new node:
					auto populateLeaf = [&]() {
						Node& node = curNode();
						node.elements = ((TypeRef*)nullptr) + buildContext.data->nodeElements.size();
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++)
							buildContext.data->nodeElements.push_back(elemBuffer[i]);
						node.elemCount = elemCount;
					};
					if (depth >= buildContext.maxDepth || elemCount <= buildContext.nodeSplitThreshold) {
						populateLeaf();
						return;
					}

					// Otherwise, we find mass center to split:
					Vector3 center((nodeBounds.start + nodeBounds.end) * 0.5f);
					if (buildContext.splitInIntersectionCenter) {
						float totalWeight = 0.0f;
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++) {
							const AABB overlapBounds = AABB(nodeBounds.start - buildContext.aabbEpsilon, nodeBounds.end + buildContext.aabbEpsilon);
							auto overlap = buildContext.getOverlapInfo(*elemBuffer[i], overlapBounds);
							Math::ShapeOverlapVolume volume = overlap;
							if ((!buildContext.splitInIntersectionCenterWeightedByVolume) && std::isfinite(volume.volume))
								volume.volume = 1.0f;
							if ((!std::isfinite(volume.volume)) || volume.volume <= 0.0f)
								continue;
							Math::ShapeOverlapCenter overlapCenter = overlap;
							overlapCenter.center = Vector3(
								Math::Min(Math::Max(nodeBounds.start.x, overlapCenter.center.x), nodeBounds.end.x),
								Math::Min(Math::Max(nodeBounds.start.y, overlapCenter.center.y), nodeBounds.end.y),
								Math::Min(Math::Max(nodeBounds.start.z, overlapCenter.center.z), nodeBounds.end.z));
							center = Math::Lerp(center, overlapCenter.center, volume.volume / (totalWeight + volume.volume));
							totalWeight += volume.volume;
						}
					}

					// If preffered split point is one of the corners, no need to continue splitting...
					if (Math::Min(
						std::abs(nodeBounds.start.x - center.x), std::abs(nodeBounds.end.x - center.x),
						std::abs(nodeBounds.start.y - center.y), std::abs(nodeBounds.end.y - center.y),
						std::abs(nodeBounds.start.z - center.z), std::abs(nodeBounds.end.z - center.z)) < buildContext.aabbEpsilon) {
						if (buildContext.splitInCenterIfIntersectionCenterNotValid)
							center = (nodeBounds.start + nodeBounds.end) * 0.5f;
						else {
							populateLeaf();
							return;
						}
					}

					// Child bounds
					auto childNodeBounds = [&](size_t childId) {
						return AABB(
							Vector3(
								((childId & 1u) ? center : nodeBounds.start).x,
								((childId & 2u) ? center : nodeBounds.start).y,
								((childId & 4u) ? center : nodeBounds.start).z),
							Vector3(
								((childId & 1u) ? nodeBounds.end : center).x,
								((childId & 2u) ? nodeBounds.end : center).y,
								((childId & 4u) ? nodeBounds.end : center).z));
					};

					// If at least one child contains all the geometry, it makes no sense to split any further..
					for (size_t childId = 0u; childId < 8u; childId++) {
						const AABB childBounds = childNodeBounds(childId);
						const AABB overlapBounds = AABB(childBounds.start - buildContext.aabbEpsilon, childBounds.end + buildContext.aabbEpsilon);
						bool containsAll = true;
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++) {
							TypeRef element = elemBuffer[i];
							auto overlap = buildContext.getOverlapInfo(*element, overlapBounds);
							const Math::ShapeOverlapVolume volume = overlap;
							if ((!std::isfinite(volume.volume)) || volume.volume < 0.0f) {
								containsAll = false;
								break;
							}
						}
						if (containsAll) {
							populateLeaf();
							return;
						}
					}

					// Insert children:
					for (size_t childId = 0u; childId < 8u; childId++) {
						const AABB childBounds = childNodeBounds(childId);
						const AABB overlapBounds = AABB(childBounds.start - buildContext.aabbEpsilon, childBounds.end + buildContext.aabbEpsilon);

						// Add element references to the child:
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++) {
							TypeRef element = elemBuffer[i];
							auto overlap = buildContext.getOverlapInfo(*element, overlapBounds);
							const Math::ShapeOverlapVolume volume = overlap;
							if ((!std::isfinite(volume.volume)) || volume.volume < 0.0f)
								continue;
							elemBuffer.push_back(element);
						}
						if (elemBuffer.size() == elemBufferEnd)
							continue;

						// Recursively populate child node:
						curNode().children[childId] = ((NodeRef)nullptr) + buildContext.data->nodes.size();
						InsertNodes(buildContext, childBounds, elemBuffer, elemBufferEnd, depth + 1u);
						elemBuffer.resize(elemBufferEnd);
					}
				}
			};

			// Build the node tree:
			NodeBuilder::InsertNodes(context, totalBoundary, rootElems, 0u, 0u);

			// Correct node tree pointers:
			for (size_t i = 0u; i < data->nodes.size(); i++) {
				Node& node = data->nodes[i];
				for (size_t childId = 0u; childId < 8u; childId++)
					if (node.children[childId] != nullptr) {
						const size_t childNodeIndex = (node.children[childId] - ((NodeRef)nullptr));
						node.children[childId] = data->nodes.data() + childNodeIndex;
						data->nodes[childNodeIndex].parentNode = &node;
						data->nodes[childNodeIndex].indexInParent = static_cast<uint8_t>(childId);
					}
				if (node.elemCount > 0u) {
					const size_t startIndex = node.elements - ((const TypeRef*)nullptr);
					node.elements = data->nodeElements.data() + startIndex;
				}
			}
		}

		// Slightly expand boundaries to escape floating point inaccuracies:
		for (size_t i = 0u; i < data->nodes.size(); i++) {
			Node& node = data->nodes[i];
			node.bounds.start -= context.aabbEpsilon;
			node.bounds.end += context.aabbEpsilon;
		}

		// We're done :D
		return Octree(data);
	}

	template<typename Type>
	template<typename TypeIt>
	inline Octree<Type> Octree<Type>::Build(const TypeIt& start, const TypeIt& end) {
		return Build(start, end,
			[](const Type& elem) { return Math::BoundingBox(elem); },
			[](const Type& elem, const AABB& bbox) { return Math::Overlap(elem, bbox); });
	}

	template<typename Type>
	inline size_t Octree<Type>::Size()const {
		const std::shared_ptr<const Data> data = GetData();
		return data == nullptr ? size_t(0u) : data->nodes.size();
	}

	template<typename Type>
	inline AABB Octree<Type>::BoundingBox()const {
		const std::shared_ptr<const Data> data = GetData();
		return data == nullptr ? AABB(Vector3(0.0f), Vector3(0.0f)) : data->nodes[0u].bounds;
	}

	template<typename Type>
	inline const Type& Octree<Type>::operator[](size_t index)const {
		return GetData()->elements[index];
	}

	template<typename Type>
	inline size_t Octree<Type>::IndexOf(const Type* element)const {
		return element - GetData()->elements.data();
	}

	template<typename Type>
	template<typename InspectHit, typename OnLeafHitsFinished,
		typename SweepAgainstAABB, typename SweepAgainstGeometry>
	inline void Octree<Type>::Cast(
		const Vector3& position, const Vector3& direction,
		const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished,
		const SweepAgainstAABB& sweepAgainstAABB, const SweepAgainstGeometry& sweepAgainstGeometry)const {

		// Check if there is data:
		const std::shared_ptr<const Data> data = m_data;
		if (data == nullptr)
			return;

		// Leaf node cast:
		const auto castInLeaf = [&](const Node* node, float distance) -> bool {
			const Vector3 offsetPos = position + direction * distance;
			const Type* const* ptr = node->elements;
			const Type* const* const end = ptr + node->elemCount;
			bool validCastsHappened = false;
			while (ptr < end) {
				const Type& surface = **ptr;
				ptr++;
				const auto result = sweepAgainstGeometry(surface, offsetPos, direction);
				const Math::SweepDistance sweepDistance = result;
				if ((!std::isfinite(sweepDistance.distance)) || sweepDistance.distance < 0.0f)
					continue;
				{
					const Math::SweepHitPoint hitPoint = result;
					const auto overlap = Math::Overlap<Vector3, AABB>(hitPoint.position, node->bounds);
					const Math::ShapeOverlapVolume overlapVolume = overlap;
					if ((!std::isfinite(overlapVolume.volume)) || overlapVolume.volume < 0.0f)
						continue;
				}
				if (inspectHit(result, distance + sweepDistance.distance, surface) == CastHint::STOP_CAST)
					return true;
				validCastsHappened = true;
			}
			if (validCastsHappened)
				return onLeafHitsFinished() == CastHint::STOP_CAST;
			else return false;
		};

		const uint8_t childOrder = static_cast<uint8_t>(
			((direction.x < 0.0f) ? 1u : 0u) |
			((direction.y < 0.0f) ? 2u : 0u) |
			((direction.z < 0.0f) ? 4u : 0u));

		const Node* nodePtr = data->nodes.data();
		uint8_t childIndex = static_cast<uint8_t>(0u);

		while (nodePtr != nullptr) {
			// 'Stack-pop' operation:
			auto moveToParent = [&]() {
				childIndex = (nodePtr->indexInParent ^ childOrder) + static_cast<uint8_t>(1u);
				nodePtr = nodePtr->parentNode;
			};

			if (childIndex <= static_cast<uint8_t>(0u)) {
				// On first entry, we need to sweep agains bounding box to make sure it's getting hit:
				const auto sweepResult = sweepAgainstAABB(nodePtr->bounds, position, direction);
				const Math::SweepDistance sweepDistance = sweepResult;

				// If sweep is invalid, we just move up to parent and continue iteration:
				if (!std::isfinite(sweepDistance.distance)) {
					moveToParent();
					continue;
				}

				// If the node is a leaf, we just do the geometry sweeps and return to parent:
				if (nodePtr->elemCount > 0u) {
					if (castInLeaf(nodePtr, Math::Max(sweepDistance.distance, 0.0f)))
						return;
					moveToParent();
					continue;
				}
			}
			
			// Try to move into the next child:
			while (childIndex < static_cast<uint8_t>(8u)) {
				const Node* childPtr = nodePtr->children[childIndex ^ childOrder];
				if (childPtr != nullptr) {
					childIndex = static_cast<uint8_t>(0u);
					nodePtr = childPtr;
					break;
				}
				else childIndex++;
			}

			// If no child was found, we just move back to the parent:
			if (childIndex >= static_cast<uint8_t>(8u))
				moveToParent();
		}
	}

	template<typename Type>
	template<typename InspectHit, typename OnLeafHitsFinished>
	inline void Octree<Type>::Raycast(
		const Vector3& position, const Vector3& direction,
		const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished)const {
		const Vector3 inverseDirection = 1.0f / direction;
		const auto sweepBBox = [&](const AABB& shape, const Vector3& rayOrigin, const Vector3&) {
			Math::RaycastResult<AABB> rv;
			rv.distance = Math::CastPreInversed(shape, rayOrigin, inverseDirection);
			rv.hitPoint = rayOrigin + rv.distance * direction;
			return rv;
		};
		Cast(position, direction, inspectHit, onLeafHitsFinished, sweepBBox, Math::Raycast<Type>);
	}

	template<typename Type>
	template<typename Shape, typename InspectHit, typename OnLeafHitsFinished>
	inline void Octree<Type>::Sweep(
		const Shape& shape, const Vector3& position, const Vector3& direction,
		const InspectHit& inspectHit, const OnLeafHitsFinished& onLeafHitsFinished)const {
		Cast(position, direction, inspectHit, onLeafHitsFinished,
			[&](const AABB& bbox, const Vector3& pos, const Vector3& dir) { return Math::Sweep<Shape, AABB>(shape, bbox, pos, dir); },
			[&](const Type& target, const Vector3& pos, const Vector3& dir) { return Math::Sweep<Shape, Type>(shape, target, pos, dir); });
	}

	template<typename Type>
	inline bool Octree<Type>::Raycast(
		const Vector3& position, const Vector3& direction, RaycastResult& result)const {
		return CastClosest([&](const auto& inspectHit, const auto& leafDone) { Raycast(position, direction, inspectHit, leafDone); }, result);
	}

	template<typename Type>
	inline typename Octree<Type>::RaycastResult Octree<Type>::Raycast(
		const Vector3& position, const Vector3& direction)const {
		return CastClosest<Math::RaycastResult<Type>>(
			[&](const auto& inspectHit, const auto& leafDone) { Raycast(position, direction, inspectHit, leafDone); });
	}

	template<typename Type>
	inline size_t Octree<Type>::RaycastAll(
		const Vector3& position, const Vector3& direction, std::vector<Octree::RaycastResult>& result, bool sort)const {
		return CastAll([&](const auto& inspectHit, const auto& leafDone) { Raycast(position, direction, inspectHit, leafDone); }, result, sort);
	}

	template<typename Type>
	inline std::vector<typename Octree<Type>::RaycastResult> Octree<Type>::RaycastAll(
		const Vector3& position, const Vector3& direction, bool sort)const {
		return CastAll<Math::RaycastResult<Type>>(
			[&](const auto& inspectHit, const auto& leafDone) { Raycast(position, direction, inspectHit, leafDone); }, sort);
	}

	template<typename Type>
	template<typename Shape>
	inline bool Octree<Type>::Sweep(
		const Shape& shape, const Vector3& position, const Vector3& direction, SweepResult<Shape>& result)const {
		return CastClosest([&](const auto& inspectHit, const auto& leafDone) { Sweep(shape, position, direction, inspectHit, leafDone); }, result);
	}

	template<typename Type>
	template<typename Shape>
	inline typename Octree<Type>::SweepResult<Shape> Octree<Type>::Sweep(
		const Shape& shape, const Vector3& position, const Vector3& direction)const {
		return CastClosest<Math::SweepResult<Shape, Type>>(
			[&](const auto& inspectHit, const auto& leafDone) { Sweep(shape, position, direction, inspectHit, leafDone); });
	}

	template<typename Type>
	template<typename Shape>
	inline size_t Octree<Type>::SweepAll(
		const Shape& shape, const Vector3& position, const Vector3& direction, std::vector<SweepResult<Shape>>& result, bool sort)const {
		return CastAll([&](const auto& inspectHit, const auto& leafDone) { Sweep(shape, position, direction, inspectHit, leafDone); }, result, sort);
	}

	template<typename Type>
	template<typename Shape>
	inline std::vector<typename Octree<Type>::SweepResult<Shape>> Octree<Type>::SweepAll(
		const Shape& shape, const Vector3& position, const Vector3& direction, bool sort)const {
		return CastAll<Math::SweepResult<Shape, Type>>(
			[&](const auto& inspectHit, const auto& leafDone) { Sweep(shape, direction, inspectHit, leafDone); }, sort);
	}
}
