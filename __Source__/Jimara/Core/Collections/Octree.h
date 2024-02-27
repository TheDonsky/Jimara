#pragma once
#include "../../Math/Intersections.h"
#include <vector>


namespace Jimara {
	/// <summary>
	/// A generic Octree
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
			/// <summary> Hit information </summary>
			HitInfo hit = {};

			/// <summary> Piece of geometry that got hit </summary>
			const Type* target = nullptr;

			/// <summary> Type-cast to hit information </summary>
			inline operator const HitInfo& ()const { return result; }

			/// <summary> Type-cast to hit information </summary>
			inline operator HitInfo& () { return result; }

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
		/// <param name="inspectHit"> Callback, that provides user with an intersection information (should return CastHint) </param>
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
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information (should return CastHint) </typeparam>
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
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information (should return CastHint) </typeparam>
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
		};
		struct Data {
			std::vector<Type> elements;
			std::vector<TypeRef> nodeElements;
			std::vector<Node> nodes;
		};
		const std::shared_ptr<const Data> m_data;
		inline std::shared_ptr<const Data> GetData()const { return m_data; }

		// Actual constructor with data:
		inline Octree(const std::shared_ptr<const Data>& data) : m_data(data) {}


		// Standard casts:

		template<typename HitType, typename CastFn>
		inline bool CastClosest(const CastFn& castFn, CastResult<HitType>& result)const {
			float bestDistance = std::numeric_limits<float>::quiet_NaN();
			castFn(
				[&](const HitType& hit, const Type& target) {
					Math::SweepDistance sweepDist = hit;
					if (std::isnan(bestDistance) || (sweepDist.distance < bestDistance)) {
						result = CastResult<HitType>{ hit, &target };
						bestDistance = sweepDist.distance;
					}
					return CastHint::CONTINUE_CAST;
				}, 
				[]() { return CastHint::STOP_CAST; });
			return !std::isnan(bestDistance);
		}

		template<typename HitType, typename CastFn>
		inline CastResult<HitType> CastClosest(const CastFn& castFn)const {
			CastResult<HitType> rv = {};
			CastClosest<HitType, CastFn>(castFn, rv);
			return rv;
		}

		template<typename HitType, typename CastFn>
		inline size_t CastAll(const CastFn& castFn, std::vector<CastResult<HitType>>& result, bool sort)const {
			const size_t initialCount = result.size();
			size_t lastLeafStart = initialCount;
			castFn(
				[&](const HitType& hit, const Type& target) {
					result.push_back(CastResult<HitType> { hit, & target });
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

		template<typename HitType, typename CastFn>
		inline std::vector<CastResult<HitType>> CastAll(const CastFn& castFn, bool sort)const {
			std::vector<CastResult<HitType>> result;
			CastAll<HitType, CastDn>(castFn, result, sort);
			return result;
		}
	};




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
		};
		BuildContext context = {
			std::make_shared<Data>(),
			calculateShapeBBox,
			getBBoxOverlapInfo,
			size_t(8u),
			size_t(24u),
			0.0000001f,
			std::vector<AABB>()
		};

		// Collect elements and individual bounding boxes:
		const std::shared_ptr<Data>& data = context.data;
		std::vector<Type>& elemBuffer = data->elements;
		for (TypeIt ptr = start; ptr != end; ++ptr) {
			elemBuffer.push_back(*ptr);
			const Type& elem = elemBuffer.back();
			const AABB bnd = context.getBoundingBox(elem);
			if (std::isfinite(bnd.start.x) && std::isfinite(bnd.start.y) && std::isfinite(bnd.start.z) &&
				std::isfinite(bnd.end.x) && std::isfinite(bnd.end.y) && std::isfinite(bnd.end.z))
				context.elemBounds.push_back(AABB(
					Vector3(Math::Min(bnd.start.x, bnd.end.x), Math::Min(bnd.start.y, bnd.end.y), Math::Min(bnd.start.z, bnd.end.z)),
					Vector3(Math::Max(bnd.start.x, bnd.end.x), Math::Max(bnd.start.y, bnd.end.y), Math::Max(bnd.start.z, bnd.end.z))));
			else elemBuffer.pop_back();
		}
		if (elemBuffer.size() <= 0u)
			return {};
		const Type* const elems = elemBuffer.data();

		// Calculate total geometry boundary
		AABB totalBoundary = context.elemBounds[0u];
		for (size_t i = 1u; i < elemBuffer.size(); i++) {
			const AABB& bbox = context.elemBounds[i];
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
				static void InsertNodes(const BuildContext& buildContext, const AABB& nodeBounds, 
						std::vector<TypeRef>& elemBuffer, size_t elemBufferStart, size_t depth) {
					const size_t elemBufferEnd = elemBuffer.size();
					const size_t elemCount = (elemBufferEnd - elemBufferStart);

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
					if (depth >= buildContext.maxDepth || elemCount <= buildContext.nodeSplitThreshold) {
						Node& node = curNode();
						node.elements = ((TypeRef*)nullptr) + buildContext.data->nodeElements.size();
						for (size_t i = 0u; i < elemCount; i++)
							buildContext.data->nodeElements.push_back(elemBuffer[i + elemBufferStart]);
						node.elemCount = elemCount;
						return;
					}

					// Otherwise, we find mass center to split:
					Vector3 center((nodeBounds.start + nodeBounds.end) * 0.5f);
					{
						float totalWeight = 0.0f;
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++) {
							auto overlap = buildContext.getOverlapInfo(*elemBuffer[i], nodeBounds);
							const Math::ShapeOverlapVolume volume = overlap;
							if ((!std::isfinite(volume.volume)) || volume.volume <= 0.0f)
								continue;
							Math::ShapeOverlapCenter overlapCenter;
							overlapCenter.center = Vector3(
								Math::Min(Math::Max(nodeBounds.start.x, overlapCenter.center.x), nodeBounds.end.x),
								Math::Min(Math::Max(nodeBounds.start.y, overlapCenter.center.y), nodeBounds.end.y),
								Math::Min(Math::Max(nodeBounds.start.z, overlapCenter.center.z), nodeBounds.end.z));
							center = Math::Lerp(center, overlapCenter.center, volume.volume / (totalWeight + volume.volume));
							totalWeight += volume.volume;
						}
					}

					// Insert children:
					for (size_t childId = 0u; childId < 8u; childId++) {
						const AABB childBounds = AABB(
							Vector3(
								((childId & 1u) ? center : nodeBounds.start).x,
								((childId & 2u) ? center : nodeBounds.start).y,
								((childId & 4u) ? center : nodeBounds.start).z),
							Vector3(
								((childId & 1u) ? nodeBounds.end : center).x,
								((childId & 2u) ? nodeBounds.end : center).y,
								((childId & 4u) ? nodeBounds.end : center).z));

						// Add element references to the child:
						for (size_t i = elemBufferStart; i < elemBufferEnd; i++) {
							TypeRef element = elemBuffer[i];
							auto overlap = buildContext.getOverlapInfo(*element, childBounds);
							const Math::ShapeOverlapVolume volume = overlap;
							if (std::isfinite(volume.volume) && volume.volume >= 0.0f)
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
			std::vector<TypeRef> elems;
			for (size_t i = 0u; i < data->elements.size(); i++)
				elems.push_back(data->elements.data() + i);
			NodeBuilder::InsertNodes(context, totalBoundary, elems, 0u, 0u);

			// Correct node tree pointers:
			for (size_t i = 0u; i < data->nodes.size(); i++) {
				Node& node = data->nodes[i];
				for (size_t childId = 0u; childId < 8u; childId++)
					if (node.children[childId] != nullptr)
						node.children[childId] = data->nodes.data() + (node.children[childId] - ((NodeRef)nullptr));
				if (node.elemCount > 0u)
					node.elements = data->nodeElements.data() + (node.elements - ((const TypeRef*)nullptr));
			}
		}

		// 'Shrink-wrap' node boundaries for small performance boost:
		{
			struct ShrinkWrapper {
				inline static bool Shrink(const BuildContext& buildContext, Node* node) {
					const Vector3 center = (node->bounds.start + node->bounds.end) * 0.5f;
					AABB shrunkBBox = AABB(center, center);
					bool hasElements = false;
					auto expandBBox = [&](const AABB& bounds) {
						if (hasElements)
							shrunkBBox = AABB(
								Vector3(
									Math::Min(shrunkBBox.start.x, bounds.start.x),
									Math::Min(shrunkBBox.start.y, bounds.start.y),
									Math::Min(shrunkBBox.start.z, bounds.start.z)),
								Vector3(
									Math::Max(shrunkBBox.end.x, bounds.end.x),
									Math::Max(shrunkBBox.end.y, bounds.end.y),
									Math::Max(shrunkBBox.end.z, bounds.end.z)));
						else {
							shrunkBBox = bounds;
							hasElements = true;
						}
					};
					if (node->elemCount > 0u) {
						// We have a leaf element:
						for (size_t i = 0u; i < node->elemCount; i++)
							expandBBox(buildContext.elemBounds[node->elements[i] - buildContext.data->elements.data()]);

						// Make sure geometry-based boundaries do not go beyond the initial bbox:
						shrunkBBox = AABB(
							Vector3(
								Math::Max(shrunkBBox.start.x, node->bounds.start.x),
								Math::Max(shrunkBBox.start.y, node->bounds.start.y),
								Math::Max(shrunkBBox.start.z, node->bounds.start.z)),
							Vector3(
								Math::Min(shrunkBBox.end.x, node->bounds.end.x),
								Math::Min(shrunkBBox.end.y, node->bounds.end.y),
								Math::Min(shrunkBBox.end.z, node->bounds.end.z)));
					}
					else for (size_t i = 0u; i < 8u; i++) {
						// We need to shrink children:
						if (Shrink(buildContext, buildContext.data->nodes.data() + (node->children[i] - buildContext.data->nodes.data())))
							expandBBox(node->children[i]->bounds);
						else node->children[i] = nullptr; // Entire child hierarchy seems to be empty, we'll never have a valid hit here, so let us discard it..
					}

					// Update node bbox:
					node->bounds = shrunkBBox;
					return hasElements;
				}
			};
			ShrinkWrapper::Shrink(context, data->nodes.data());
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
		return Build(start, end, Math::BoundingBox<Type>, Math::Overlap<Type, AABB>);
	}

	template<typename Type>
	inline AABB Octree<Type>::BoundingBox()const {
		const std::shared_ptr<const Data> data = GetData();
		return data == nullptr ? AABB(Vector3(0.0f), Vector3(0.0f)) : data->nodes[0u].bounds;
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

		class CastProcess {
		public:
			// Ray-related stuff:
			const Vector3 offset;
			const Vector3 direction;
			const size_t childOrder;

			// Function pointers:
			decltype(inspectCast) inspectGeometryCast;
			decltype(inspectMoreLeaves) inspectMoreLeaves;
			decltype(sweepAgainstAABB) sweepBoundingBox;
			decltype(sweepAgainstGeometry) sweepSurface;

		private:
			// Leaf node cast:
			inline bool CastInLeaf(const Node* node, float distance)const {
				const Vector3 position = offset + direction * distance;
				TypeRef ptr = *node->elements;
				const TypeRef const end = ptr + node->elemCount;
				bool validCastsHappened = false;
				while (ptr < end) {
					const Type& surface = *ptr;
					ptr++;
					const auto result = sweepSurface(surface, position, direction);
					{
						const Math::SweepDistance distance = result;
						if ((!std::isfinite(distance.distance)) || distance.distance < 0.0f)
							continue;
					}
					{
						const Math::SweepHitPoint hitPoint = result;
						const auto overlap = Math::Overlap<Vector3, AABB>(hitPoint.position, node->bounds);
						const Math::ShapeOverlapVolume overlapVolume = overlap;
						if ((!std::isfinite(overlapVolume.volume)) || overlapVolume.volume < 0.0f)
							continue;
					}
					if (inspectGeometryCast(result, surface) == CastHint::STOP_CAST)
						return true;
					validCastsHappened = true;
				}
				if (validCastsHappened)
					return inspectMoreLeaves() == CastHint::STOP_CAST;
				else return false;
			}

		public:
			// Recursive cast in node tree:
			inline bool CastInNode(const Node* node, float distance)const {
				if (node == nullptr)
					return false;
				else {
					const auto sweepResult = sweepBoundingBox(node->bounds, offset, direction);
					const Math::SweepDistance sweepDistance = sweepResult;
					if (!std::isfinite(sweepDistance.distance))
						return false;
					if (sweepDistance.distance > distance)
						distance = sweepDistance.distance;
				}
				if (node->elemCount > 0u)
					return CastInLeaf(node, distance);
				else for (size_t i = 0u; i < 8u; i++)
					if (CastInNode(node->children[i ^ childOrder], distance))
						return true;
				return false;
			}
		};

		const CastProcess process = {
			// Ray:
			position,
			direction,

			// Child order:
			((direction.x < 0.0f) ? 1u : 0u) |
			((direction.y < 0.0f) ? 2u : 0u) |
			((direction.z < 0.0f) ? 4u : 0u),

			// Function pointers:
			inspectHit,
			onLeafHitsFinished,
			sweepAgainstAABB,
			sweepAgainstGeometry
		};

		process.CastInNode(data->nodes.data(), 0.0f);
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
		CastClosest([&](const auto& inspectHit, const auto& leafDone) { Raycast(position, direction, inspectHit, leafDone); }, result);
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
		CastClosest([&](const auto& inspectHit, const auto& leafDone) { Sweep(shape, position, direction, inspectHit, leafDone); }, result);
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
