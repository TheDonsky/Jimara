#pragma once
#include "Octree.h"
#include "../Property.h"
#include <vector>


namespace Jimara {
	/// <summary>
	/// A generic Voxel Grid
	/// <para/> VoxelGrid is built more for dynamic insertions/removals/reinsertions than for pure performance;
	/// <para/> Copying grid will result in the duplication of the whole state snapshot and therefore, is expensive
	/// </summary>
	/// <typeparam name="Type"> Stored geometric element type </typeparam>
	template<typename Type>
	class VoxelGrid {
	public:
		/// <summary>
		/// Hint, expected to be returned from inspectHit and onLeafHitsFinished 
		/// callbacks passed to generic cast/sweep functions
		/// </summary>
		using CastHint = typename Octree<Type>::CastHint;

		/// <summary> Result of raycast calls </summary>
		using RaycastResult = typename Octree<Type>::RaycastResult;

		/// <summary>
		/// Result of Sweep calls
		/// </summary>
		/// <typeparam name="Shape"> Sweep shape type </typeparam>
		template<typename Shape>
		using SweepResult = typename Octree<Type>::template SweepResult<Shape>;

		/// <summary> Constructor </summary>
		inline VoxelGrid();

		/// <summary> Total bounding box of the VoxelGrid </summary>
		inline AABB BoundingBox()const;

		/// <summary> Total bounding box of the VoxelGrid </summary>
		inline Property<AABB> BoundingBox();

		/// <summary> Number of buckets within the VoxelGrid </summary>
		inline Size3 GridSize()const;

		/// <summary> Total bounding box of the VoxelGrid </summary>
		inline Property<Size3> GridSize();

		/// <summary> Number of geometric elements stored in the VoxelGrid </summary>
		inline size_t Size()const;

		/// <summary>
		/// Adds a geometric element to the VoxelGrid
		/// </summary>
		/// <param name="element"> Element </param>
		inline void Push(const Type& element);

		/// <summary> Removes last geometric element from the VoxelGrid </summary>
		inline void Pop();

		/// <summary>
		/// Stored geometric element by index
		/// </summary>
		/// <param name="index"> Element index </param>
		/// <returns> Element reference </returns>
		inline const Type& operator[](size_t index)const;

		/// <summary>
		/// Accessor for modifying stored geometric elements
		/// </summary>
		class ElementAccessor {
		private:
			VoxelGrid* const grid;
			const size_t index;
			friend class VoxelGrid;
			inline ElementAccessor(VoxelGrid* g, size_t i) : grid(g), index(i) {}

		public:
			/// <summary> Element </summary>
			inline operator const Type& ()const;

			/// <summary>
			/// Updates element
			/// </summary>
			/// <param name="value"> New value </param>
			/// <returns> Self </returns>
			inline ElementAccessor& operator=(const Type& value);
		};

		/// <summary>
		/// Stored geometric element accessor by index
		/// </summary>
		/// <param name="index"> Element index </param>
		/// <returns> Element accessor </returns>
		inline ElementAccessor operator[](size_t index);

		/// <summary>
		/// Returns index of given element
		/// <para/> Element, passed as the argument has to be returned internally by 
		/// any of the operator[]/Cast/Raycast/Sweep operations via public API. Otherwise, the behaviour is undefined.
		/// <para/> Do not assume the VoxelGrid knows how to compare actual primitives during this, 
		/// this is just for letting the user know the index from generation time, when the query result returns an internal pointer.
		/// </summary>
		/// <param name="element"> Element pointer </param>
		/// <returns> Index of the element </returns>
		inline size_t IndexOf(const Type* element)const;

		/// <summary>
		/// Generic cast function inside the VoxelGrid
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
		/// <param name="sweepBBox"> Bounding box of the swept geometry (basically, on each step, all the buckets overlapping moved sweepBBox will be inspected) </param>
		/// <param name="maxDistance"> Max hit distance </param>
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
		template<typename InspectHit, typename OnBucketHitsFinished,
			typename SweepAgainstAABB, typename SweepAgainstGeometry>
		inline void Cast(
			const Vector3& position, Vector3 direction, AABB sweepBBox, float maxDistance,
			const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished,
			const SweepAgainstAABB& sweepAgainstAABB, const SweepAgainstGeometry& sweepAgainstGeometry)const;

		/// <summary>
		/// Generic raycast function inside the VoxelGrid
		/// <para/> Hits are reported through inspectHit callback; distances are not sorted by default;
		/// <para/> Reports are "interrupted" with onBucketHitsFinished calls, which gets invoked after several successful inspectHit
		/// calls, before leaving the bucket. It's more or less guaranteed that normal Raycasts will go through the buckets in sorted order,
		/// even if the contacts from inside the buckets may be reported out of order.
		/// </summary>
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information
		/// (should return CastHint; receives result from sweepAgainstGeometry, total sweep distance and const reference to the geometry that got hit)
		/// </typeparam>
		/// <typeparam name="OnBucketHitsFinished"> Callable, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </typeparam>
		/// <param name="position"> Ray origin </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <param name="inspectHit"> Callback, that provides user with an intersection information (should return CastHint) </param>
		/// <param name="onBucketHitsFinished"> Callback type, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </param>
		template<typename InspectHit, typename OnBucketHitsFinished>
		inline void Raycast(
			const Vector3& position, const Vector3& direction, float maxDistance,
			const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished)const;

		/// <summary>
		/// Generic sweep function inside the VoxelGrid
		/// <para/> Hits are reported through inspectHit callback; distances are not sorted by default;
		/// <para/> Reports are "interrupted" with onBucketHitsFinished calls, which gets invoked after several successful inspectHit
		/// calls, before leaving the bucket. It's more or less guaranteed that normal Raycasts will go through the buckets in sorted order,
		/// even if the contacts from inside the buckets may be reported out of order.
		/// </summary>
		/// <typeparam name="Shape"> 'Swept' geometry type </typeparam>
		/// <typeparam name="InspectHit"> Callable, that provides user with an intersection information 
		/// (should return CastHint; receives result from sweepAgainstGeometry, total sweep distance and const reference to the geometry that got hit) 
		/// </typeparam>
		/// <typeparam name="OnBucketHitsFinished"> Callable, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </typeparam>
		/// <param name="shape"> 'Swept' shape </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <param name="inspectHit"> Callback, that provides user with an intersection information (should return CastHint) </param>
		/// <param name="onBucketHitsFinished"> Callback type, that lets the user know that the Cast function has left the last bucket it reported hits from (should return CastHint) </param>
		template<typename Shape, typename InspectHit, typename OnBucketHitsFinished>
		inline void Sweep(
			const Shape& shape, const Vector3& position, const Vector3& direction, float maxDistance,
			const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished)const;

		/// <summary>
		/// Raycast, reporting the closest hit
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="result"> Result will be stored here </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> True, if the ray hits anything </returns>
		inline bool Raycast(const Vector3& position, const Vector3& direction,
			RaycastResult& result, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::CastClosest([&](const auto& inspectHit, const auto& leafDone) { 
				Raycast(position, direction, maxDistance, inspectHit, leafDone); }, result);
		}

		/// <summary>
		/// Raycast, reporting the closest hit
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> Closest hit if found, null-target otherwise </returns>
		inline RaycastResult Raycast(const Vector3& position, const Vector3& direction,
			float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::template CastClosest<Math::RaycastResult<Type>>([&](const auto& inspectHit, const auto& leafDone) { 
				Raycast(position, direction, maxDistance, inspectHit, leafDone); });
		}

		/// <summary>
		/// Raycast, reporting all hits
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="result"> Hits will be appended to this list </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> Number of reported hits </returns>
		inline size_t RaycastAll(const Vector3& position, const Vector3& direction, std::vector<RaycastResult>& result,
			bool sort = false, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::CastAll([&](const auto& inspectHit, const auto& leafDone) { 
				Raycast(position, direction, maxDistance, inspectHit, leafDone); }, result, sort);
		}

		/// <summary>
		/// Raycast, reporting all hits
		/// </summary>
		/// <param name="position"> Ray position </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> All hits </returns>
		inline std::vector<RaycastResult> RaycastAll(const Vector3& position, const Vector3& direction,
			bool sort = false, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::template CastAll<Math::RaycastResult<Type>>([&](const auto& inspectHit, const auto& leafDone) {
				Raycast(position, direction, maxDistance, inspectHit, leafDone); }, sort);
		}

		/// <summary>
		/// Sweep, reporting the closest hit
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="result"> Hit information will be stored here </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> True, if the shape hits anything </returns>
		template<typename Shape>
		inline bool Sweep(const Shape& shape, const Vector3& position, const Vector3& direction,
			SweepResult<Shape>& result, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::CastClosest([&](const auto& inspectHit, const auto& leafDone) {
				Sweep(shape, position, direction, maxDistance, inspectHit, leafDone); }, result);
		}

		/// <summary>
		/// Sweep, reporting the closest hit
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> Closest hit if found, null-target otherwise </returns>
		template<typename Shape>
		inline SweepResult<Shape> Sweep(const Shape& shape, const Vector3& position, const Vector3& direction,
			float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::template CastClosest<Math::SweepResult<Shape, Type>>([&](const auto& inspectHit, const auto& leafDone) {
				Sweep(shape, position, direction, maxDistance, inspectHit, leafDone); });
		}

		/// <summary>
		/// Sweep, reporting all contacts
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="result"> Hits will be appended to this list </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> Number of reported hits </returns>
		template<typename Shape>
		inline size_t SweepAll(const Shape& shape, const Vector3& position, const Vector3& direction, std::vector<SweepResult<Shape>>& result,
			bool sort = false, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::CastAll([&](const auto& inspectHit, const auto& leafDone) {
				Sweep(shape, position, direction, maxDistance, inspectHit, leafDone); }, result, sort);
		}

		/// <summary>
		/// Sweep, reporting all contacts
		/// </summary>
		/// <typeparam name="Shape"> Shape that's being swept </typeparam>
		/// <param name="shape"> Shape that's being swept </param>
		/// <param name="position"> Initial shape location </param>
		/// <param name="direction"> Sweep direction </param>
		/// <param name="sort"> If true, reported hits will be sorted based on the distance </param>
		/// <param name="maxDistance"> Max hit distance </param>
		/// <returns> All hits </returns>
		template<typename Shape>
		inline std::vector<SweepResult<Shape>> SweepAll(const Shape& shape, const Vector3& position, const Vector3& direction,
			bool sort = false, float maxDistance = std::numeric_limits<float>::infinity())const {
			return Octree<Type>::template CastAll<Math::SweepResult<Shape, Type>>([&](const auto& inspectHit, const auto& leafDone) {
				Sweep(shape, direction, maxDistance, inspectHit, leafDone); }, sort);
		}


	private:
		AABB m_boundingBox = AABB(Vector3(-100.0f), Vector3(100.0f));
		Size3 m_gridSize = Size3(100u);

		static const constexpr float AABB_EPSILON = Math::INTERSECTION_EPSILON * 8.0f;
		static const constexpr size_t NoId = ~size_t(0u);
		
		struct ElemData {
			Type shape = {};
			size_t firstNodeId = NoId;
		};
		std::vector<ElemData> m_elements;

		struct BucketElemNode {
			size_t elementId = NoId;
			size_t bucketId = NoId;
			
			size_t nextNodeId = NoId;
			size_t prevNodeId = NoId;

			size_t nextElemNodeId = NoId;
		};
		std::vector<size_t> m_bucketRootNodes;
		std::vector<BucketElemNode> m_bucketNodes;
		std::vector<size_t> m_freeBucketNodes;

		inline void FillMissingRootNodes() {
			const size_t nodeCount = (m_elements.size() > 0u) ? (size_t(m_gridSize.x) * m_gridSize.y * m_gridSize.z) : 0u;
			while (m_bucketRootNodes.size() < nodeCount)
				m_bucketRootNodes.push_back(NoId);
		}

		inline void RemoveElementInfo(size_t elementIndex) {
			size_t elemNodeId = m_elements[elementIndex].firstNodeId;
			m_elements[elementIndex].firstNodeId = NoId;
			while (elemNodeId != NoId) {
				m_freeBucketNodes.push_back(elemNodeId);
				const BucketElemNode& node = m_bucketNodes[elemNodeId];
				if (node.prevNodeId == NoId)
					m_bucketRootNodes[node.bucketId] = node.nextNodeId;
				else m_bucketNodes[node.prevNodeId].nextNodeId = node.nextNodeId;
				if (node.nextNodeId != NoId)
					m_bucketNodes[node.nextNodeId].prevNodeId = node.prevNodeId;
				elemNodeId = node.nextElemNodeId;
			}
		}

		inline void InsertElementInfo(size_t elementIndex) {
			RemoveElementInfo(elementIndex);

			const Vector3 bucketSize = (m_boundingBox.end - m_boundingBox.start) / Vector3(m_gridSize);
			if (Math::SqrMagnitude(bucketSize) < std::numeric_limits<float>::epsilon())
				return;
			
			const Type& element = m_elements[elementIndex].shape;
			const AABB elemBBox = Math::BoundingBox(element);
			
			const AABB insertionBounds = AABB(
				Vector3(
					Math::Max(Math::Min(elemBBox.start.x, elemBBox.end.x), m_boundingBox.start.x + bucketSize.x * 0.5f),
					Math::Max(Math::Min(elemBBox.start.y, elemBBox.end.y), m_boundingBox.start.y + bucketSize.y * 0.5f),
					Math::Max(Math::Min(elemBBox.start.z, elemBBox.end.z), m_boundingBox.start.z + bucketSize.z * 0.5f)) - m_boundingBox.start,
				Vector3(
					Math::Min(Math::Max(elemBBox.start.x, elemBBox.end.x), m_boundingBox.end.x - bucketSize.x * 0.5f),
					Math::Min(Math::Max(elemBBox.start.y, elemBBox.end.y), m_boundingBox.end.y - bucketSize.y * 0.5f),
					Math::Min(Math::Max(elemBBox.start.z, elemBBox.end.z), m_boundingBox.end.z - bucketSize.z * 0.5f)) - m_boundingBox.start);
			const Size3 firstBucket = Size3(insertionBounds.start / bucketSize);
			const Size3 lastBucket = Size3(insertionBounds.end / bucketSize);

			auto allocateBucketNode = [&]() -> size_t {
				if (m_freeBucketNodes.empty()) {
					m_bucketNodes.emplace_back();
					return m_bucketNodes.size() - 1u;
				}
				else {
					size_t index = m_freeBucketNodes.back();
					m_freeBucketNodes.pop_back();
					return index;
				}
			};

			for (size_t x = firstBucket.x; x <= lastBucket.x; x++)
				for (size_t y = firstBucket.y; y <= lastBucket.y; y++)
					for (size_t z = firstBucket.z; z <= lastBucket.z; z++) {
						const AABB bucketBBox = AABB(
							Vector3(float(x), float(y), float(z)) * bucketSize + m_boundingBox.start - AABB_EPSILON,
							Vector3(float(x + 1u), float(y + 1u), float(z + 1u)) * bucketSize + m_boundingBox.start + AABB_EPSILON);
						const auto result = Math::Overlap(element, bucketBBox);
						if (!static_cast<Math::ShapeOverlapVolume>(result))
							continue;
						FillMissingRootNodes();

						const size_t bucketIndex = (m_gridSize.x * (size_t(m_gridSize.y) * z + y) + x);
						const size_t nodeIndex = allocateBucketNode();
						BucketElemNode& node = m_bucketNodes[nodeIndex];
						{
							node.elementId = elementIndex;
							node.bucketId = bucketIndex;
						}
						{
							node.prevNodeId = NoId;
							node.nextNodeId = m_bucketRootNodes[bucketIndex];
							if (node.nextNodeId != NoId)
								m_bucketNodes[node.nextNodeId].prevNodeId = nodeIndex;
							m_bucketRootNodes[bucketIndex] = nodeIndex;
						}
						{
							node.nextElemNodeId = m_elements[elementIndex].firstNodeId;
							m_elements[elementIndex].firstNodeId = nodeIndex;
						}
					}
		}

		inline void RebuildGrid() {
			for (size_t i = 0u; i < m_elements.size(); i++)
				m_elements[i].firstNodeId = NoId;
			m_bucketRootNodes.clear();
			m_bucketNodes.clear();
			m_freeBucketNodes.clear();
			for (size_t i = 0u; i < m_elements.size(); i++)
				InsertElementInfo(i);
		}
	};


	template<typename Type>
	inline VoxelGrid<Type>::VoxelGrid() {}

	template<typename Type>
	inline AABB VoxelGrid<Type>::BoundingBox()const {
		return m_boundingBox;
	}

	template<typename Type>
	inline Property<AABB> VoxelGrid<Type>::BoundingBox() {
		typedef AABB(*GetFn)(VoxelGrid*);
		typedef void(*SetFn)(VoxelGrid*, const AABB&);
		static const GetFn get = [](VoxelGrid* self) -> AABB { return self->m_boundingBox; };
		static const SetFn set = [](VoxelGrid* self, const AABB& bbox) {
			const AABB fixedBBox = AABB(
				Vector3(Math::Min(bbox.start.x, bbox.end.x), Math::Min(bbox.start.y, bbox.end.y), Math::Min(bbox.start.z, bbox.end.z)),
				Vector3(Math::Max(bbox.start.x, bbox.end.x), Math::Max(bbox.start.y, bbox.end.y), Math::Max(bbox.start.z, bbox.end.z)));
			if (self->m_boundingBox.start == fixedBBox.start && self->m_boundingBox.end == fixedBBox.end)
				return;
			self->m_boundingBox = fixedBBox;
			self->RebuildGrid();
		};
		return Property<AABB>(get, set, this);
	}

	template<typename Type>
	inline Size3 VoxelGrid<Type>::GridSize()const {
		return m_gridSize;
	}

	template<typename Type>
	inline Property<Size3> VoxelGrid<Type>::GridSize() {
		typedef Size3(*GetFn)(VoxelGrid*);
		typedef void(*SetFn)(VoxelGrid*, const Size3&);
		static const GetFn get = [](VoxelGrid* self) -> Size3 { return self->m_gridSize; };
		static const SetFn set = [](VoxelGrid* self, const Size3& size) {
			const Size3 gridSize = Size3(Math::Max(size.x, 1u), Math::Max(size.y, 1u), Math::Max(size.z, 1u));
			if (self->m_gridSize == gridSize)
				return;
			self->m_gridSize = gridSize;
			self->RebuildGrid();
		};
		return Property<Size3>(get, set, this);
	}

	template<typename Type>
	inline size_t VoxelGrid<Type>::Size()const {
		return m_elements.size();
	}

	template<typename Type>
	inline void VoxelGrid<Type>::Push(const Type& element) {
		m_elements.push_back({ element, NoId });
		InsertElementInfo(m_elements.size() - 1u);
	}

	template<typename Type>
	inline void VoxelGrid<Type>::Pop() {
		RemoveElementInfo(m_elements.size() - 1u);
		m_elements.pop_back();
	}

	template<typename Type>
	inline const Type& VoxelGrid<Type>::operator[](size_t index)const {
		return m_elements[index].shape;
	}

	template<typename Type>
	inline typename VoxelGrid<Type>::ElementAccessor VoxelGrid<Type>::operator[](size_t index) {
		return ElementAccessor(this, index);
	}

	template<typename Type>
	inline VoxelGrid<Type>::ElementAccessor::operator const Type& ()const {
		return grid->m_elements[index].shape;
	}

	template<typename Type>
	inline typename VoxelGrid<Type>::ElementAccessor& VoxelGrid<Type>::ElementAccessor::operator=(const Type& value) {
		grid->RemoveElementInfo(index);
		grid->m_elements[index].shape = value;
		grid->InsertElementInfo(index);
		return *this;
	}

	template<typename Type>
	inline size_t VoxelGrid<Type>::IndexOf(const Type* element)const {
		return reinterpret_cast<const ElemData*>(reinterpret_cast<const char*>(element) - offsetof(ElemData, shape)) - m_elements.data();
	}

	template<typename Type>
	template<typename InspectHit, typename OnBucketHitsFinished,
		typename SweepAgainstAABB, typename SweepAgainstGeometry>
	inline void VoxelGrid<Type>::Cast(
		const Vector3& position, Vector3 direction, AABB sweepBBox, float maxDistance,
		const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished,
		const SweepAgainstAABB& sweepAgainstAABB, const SweepAgainstGeometry& sweepAgainstGeometry)const {

		if (m_bucketRootNodes.empty())
			return;

		const auto expandedBBox = [](const Vector3& start, const Vector3& end) {
			return AABB(start - AABB_EPSILON, end + AABB_EPSILON);
		};

		direction = Math::Normalize(direction);
		if (Math::Magnitude(direction) < 0.0001f)
			return;
		const AABB gridBoundingBox = m_boundingBox;
		const Int3 gridSize = m_gridSize;
		const Vector3 bucketSize = (gridBoundingBox.end - gridBoundingBox.start) / Vector3(gridSize);
		const Vector3 inverseBucketSize = 1.0f / bucketSize;

		auto pointBucket = [&](const Vector3& pos) -> Int3 {
			static const constexpr auto roundDown = [](float f) { return static_cast<int>(f) - static_cast<int>(f < 0.0f); };
			static_assert(roundDown(0.0f) == 0);
			static_assert(roundDown(-0.0f) == 0);
			static_assert(roundDown(0.5f) == 0);
			static_assert(roundDown(1.5f) == 1);
			static_assert(roundDown(2.5f) == 2);
			static_assert(roundDown(-0.5f) == -1);
			static_assert(roundDown(-1.5f) == -2);
			const Vector3 fIndex = (pos - gridBoundingBox.start) * inverseBucketSize;
			return Int3(roundDown(fIndex.x), roundDown(fIndex.y), roundDown(fIndex.z));
		};

		auto isValidBucket = [&](const Int3& index) {
			return
				index.x >= 0 && index.x < gridSize.x &&
				index.y >= 0 && index.x < gridSize.y &&
				index.z >= 0 && index.z < gridSize.z;
		};

		const float distanceStep = Math::Min(
			bucketSize.x / std::abs(direction.x),
			bucketSize.y / std::abs(direction.y),
			bucketSize.z / std::abs(direction.z));

		for (uint32_t i = 0u; i < 3u; i++)
			if (sweepBBox.start[i] > sweepBBox.end[i])
				std::swap(sweepBBox.start[i], sweepBBox.end[i]);

		AABB lastIterationBBox = AABB(gridBoundingBox.start, gridBoundingBox.start);
		auto inspectBucket = [&](const Int3& bucketId) -> bool {
			size_t bucketIt = m_bucketRootNodes[
				gridSize.x * (size_t(gridSize.y) * bucketId.z + bucketId.y) + bucketId.x];
			if (bucketIt == NoId)
				return false;

			AABB bbox = AABB(
				gridBoundingBox.start + bucketSize * Vector3(bucketId) - AABB_EPSILON,
				gridBoundingBox.start + bucketSize * Vector3(bucketId + 1) + AABB_EPSILON);

			{
				const Math::ShapeOverlapVolume lastAreaOverlap = Math::Overlap<Vector3, AABB>((bbox.start + bbox.end) * 0.5f, lastIterationBBox);
				if (lastAreaOverlap)
					return false;
			}

			const float bucketSweepDistance = static_cast<Math::SweepDistance>(sweepAgainstAABB(bbox, position, direction));
			if (!std::isfinite(bucketSweepDistance))
				return false;
			const Vector3 offsetPos = direction * Math::Max(bucketSweepDistance, 0.0f) + position;

			bool hitsInspected = false;
			while (bucketIt != NoId) {
				const BucketElemNode& bucketNode = m_bucketNodes[bucketIt];
				bucketIt = bucketNode.nextNodeId;

				const Type& elem = m_elements[bucketNode.elementId].shape;
				const auto result = sweepAgainstGeometry(elem, offsetPos, direction);

				const Math::SweepDistance sweepDistance = result;
				if ((!std::isfinite(sweepDistance.distance)) || sweepDistance.distance < 0.0f)
					continue;

				const float totalSweepDistance = bucketSweepDistance + sweepDistance.distance;
				if (totalSweepDistance > maxDistance)
					continue;
				else {
					const Math::SweepHitPoint hitPoint = result;
					const Math::ShapeOverlapVolume overlapVolume = Math::Overlap<Vector3, AABB>(hitPoint.position, bbox);
					if (!static_cast<bool>(overlapVolume))
						continue;
				}

				if (inspectHit(result, totalSweepDistance, elem) == CastHint::STOP_CAST)
					return true;
				else hitsInspected = true;
			}

			if (hitsInspected)
				return (onBucketHitsFinished() == CastHint::STOP_CAST);
			else return false;
		};

		float distanceSoFar;
		{
			const auto sweepResult = sweepAgainstAABB(gridBoundingBox, position, direction);
			const Math::SweepDistance sweepDist = sweepResult;
			distanceSoFar = sweepDist;
		}
		if (!std::isfinite(distanceSoFar))
			return;
		if (distanceSoFar < 0.0f)
			distanceSoFar = 0.0f;

		const Size3 axisOrder =
			(std::abs(direction.x) >= std::abs(direction.y)) ? (
				(std::abs(direction.y) >= std::abs(direction.z)) ? Size3(0u, 1u, 2u) : // (X > Y), (Y > Z) => X > Y > Z
				(std::abs(direction.x) >= std::abs(direction.z)) ? Size3(0u, 2u, 1u) : // (X > Y), (Z > Y), (X > Z) => X > Z > Y
				Size3(2u, 0u, 1u)) : // (X > Y), (Z > Y), (Z > X) => Z > X > Y;
			(std::abs(direction.y) >= std::abs(direction.z)) ? (
				(std::abs(direction.x) >= std::abs(direction.z)) ? Size3(1u, 0u, 2u) : // (Y > X), (Y > Z), (X > Z) => Y > X > Z
				Size3(1u, 2u, 0u)) : // (Y > X), (Y > Z), (Z > X) => Y > Z > X
			Size3(2u, 1u, 0u); // (Y > X), (Z > Y) => Z > Y > X

		const Int3 indexDelta(
			(direction.x >= 0.0f) ? 1 : -1,
			(direction.y >= 0.0f) ? 1 : -1,
			(direction.z >= 0.0f) ? 1 : -1);

		while (true) {
			if (distanceSoFar > maxDistance)
				break;

			const Vector3 startPoint = (position + sweepBBox.start + distanceSoFar * direction);
			const Vector3 endPoint = (position + sweepBBox.end + distanceSoFar * direction);

			const Int3 startBucket = pointBucket(startPoint);
			const Int3 endBucket = pointBucket(endPoint);

			const Int3 firstBucket(
				indexDelta.x > 0 ? startBucket.x : endBucket.x,
				indexDelta.y > 0 ? startBucket.y : endBucket.y,
				indexDelta.z > 0 ? startBucket.z : endBucket.z);
			const Int3 lastBucket(
				indexDelta.x < 0 ? (startBucket.x - 1) : (endBucket.x + 1),
				indexDelta.y < 0 ? (startBucket.y - 1) : (endBucket.y + 1),
				indexDelta.z < 0 ? (startBucket.z - 1) : (endBucket.z + 1));
			const Int3 sentinelBucket = lastBucket + indexDelta;

			bool hasValidBuckets = false;
			for (int x = firstBucket[axisOrder.x]; x != sentinelBucket[axisOrder.x]; x += indexDelta[axisOrder.x])
				for (int y = firstBucket[axisOrder.y]; y != sentinelBucket[axisOrder.y]; y += indexDelta[axisOrder.y])
					for (int z = firstBucket[axisOrder.z]; z != sentinelBucket[axisOrder.z]; z += indexDelta[axisOrder.z]) {
						Int3 index = {};
						index[axisOrder.x] = x;
						index[axisOrder.y] = y;
						index[axisOrder.z] = z;
						if (!isValidBucket(index))
							continue;
						hasValidBuckets = true;
						if (inspectBucket(index))
							return;
					}
			if (!hasValidBuckets)
				break;

			lastIterationBBox = AABB(
				gridBoundingBox.start + bucketSize * Vector3(
					float(Math::Min(firstBucket.x, lastBucket.x)), 
					float(Math::Min(firstBucket.y, lastBucket.y)), 
					float(Math::Min(firstBucket.z, lastBucket.z))),
				gridBoundingBox.start + bucketSize * Vector3(
					float(Math::Max(firstBucket.x, lastBucket.x)),
					float(Math::Max(firstBucket.y, lastBucket.y)),
					float(Math::Max(firstBucket.z, lastBucket.z))));

			distanceSoFar += distanceStep;
		}
	}

	template<typename Type>
	template<typename InspectHit, typename OnBucketHitsFinished>
	inline void VoxelGrid<Type>::Raycast(
		const Vector3& position, const Vector3& direction, float maxDistance,
		const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished)const {
		const Vector3 inverseDirection = 1.0f / direction;
		const auto sweepBBox = [&](const AABB& shape, const Vector3& rayOrigin, const Vector3&) {
			Math::RaycastResult<AABB> rv;
			rv.distance = Math::CastPreInversed(shape, rayOrigin, inverseDirection);
			rv.hitPoint = rayOrigin + rv.distance * direction;
			return rv;
		};
		Cast(position, direction, 
			AABB(Vector3(0.0f), Vector3(0.0f)), maxDistance, 
			inspectHit, onBucketHitsFinished, 
			sweepBBox, Math::Raycast<Type>);
	}

	template<typename Type>
	template<typename Shape, typename InspectHit, typename OnBucketHitsFinished>
	inline void VoxelGrid<Type>::Sweep(
		const Shape& shape, const Vector3& position, const Vector3& direction, float maxDistance,
		const InspectHit& inspectHit, const OnBucketHitsFinished& onBucketHitsFinished)const {
		Cast(position, direction,
			Math::BoundingBox(shape), maxDistance,
			inspectHit, onBucketHitsFinished,
			[&](const AABB& bbox, const Vector3& pos, const Vector3& dir) { return Math::Sweep<Shape, AABB>(shape, bbox, pos, dir); },
			[&](const Type& target, const Vector3& pos, const Vector3& dir) { return Math::Sweep<Shape, Type>(shape, target, pos, dir); });
	}
}
