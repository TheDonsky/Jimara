#pragma once
#include "Mesh.h"
#include "../../Core/Synch/SpinLock.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	/// <summary>
	/// Cached bounding box calculator for mesh types
	/// </summary>
	/// <typeparam name="VertexType"> Mesh vertex type (HAS TO CONTAIN 'position' Vector3/Vector2 field) </typeparam>
	/// <typeparam name="FaceType"> Mesh face type </typeparam>
	template<typename VertexType, typename FaceType>
	class MeshBoundingBox : public virtual Object {
	public:
		/// <summary>
		/// Gets cached instance of MeshBoundingBox
		/// </summary>
		/// <param name="mesh"> Target mesh </param>
		/// <returns> Shared MeshBoundingBox instance </returns>
		inline static Reference<MeshBoundingBox> GetFor(const Mesh<VertexType, FaceType>* mesh);

		/// <summary> Virtual destructor </summary>
		inline virtual ~MeshBoundingBox() { m_mesh->OnDirty() -= Callback(&MeshBoundingBox::MeshChanged, this); }

		/// <summary> Gets up to date bounding box of the target mesh geometry </summary>
		inline AABB GetMeshBounds();


	private:
		// Target mesh
		const Reference<const Mesh<VertexType, FaceType>> m_mesh;

		// Lock for last calculated value
		SpinLock m_aabbLock;

		// True, when the calculation is not yet done, or is invalidated
		std::atomic_bool m_aabbDirty = true;

		// Result of the last boundary calculation
		AABB m_aabb = {};


		// Constructor is private
		inline MeshBoundingBox(const Mesh<VertexType, FaceType>* mesh) : m_mesh(mesh) { 
			assert(m_mesh != nullptr);
			m_mesh->OnDirty() += Callback(&MeshBoundingBox::MeshChanged, this);
		}

		// Invoked each time the Mesh gets altered, invalidating the calculation
		inline void MeshChanged(const Mesh<VertexType, FaceType>* mesh) {
			assert(mesh == m_mesh);
			std::unique_lock<SpinLock> lock(m_aabbLock);
			m_aabbDirty = true;
		}
	};


	/// <summary> MeshBoundingBox for TriMesh </summary>
	using TriMeshBoundingBox = MeshBoundingBox<MeshVertex, TriangleFace>;

	/// <summary> MeshBoundingBox for PolyMesh </summary>
	using PolyMeshBoundingBox = MeshBoundingBox<MeshVertex, PolygonFace>;





	/// <summary>
	/// Gets cached instance of MeshBoundingBox
	/// </summary>
	/// <param name="mesh"> Target mesh </param>
	/// <returns> Shared MeshBoundingBox instance </returns>
	template<typename VertexType, typename FaceType>
	inline Reference<MeshBoundingBox<VertexType, FaceType>> MeshBoundingBox<VertexType, FaceType>::GetFor(const Mesh<VertexType, FaceType>* mesh) {
		if (mesh == nullptr)
			return nullptr;
		struct CachedAABB : public virtual MeshBoundingBox, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			inline CachedAABB(const Mesh<VertexType, FaceType>* mesh) : MeshBoundingBox(mesh) {}
			inline virtual ~CachedAABB() {}
		};
		struct AABBCache : public virtual ObjectCache<Reference<const Object>> {
			inline Reference<MeshBoundingBox> GetFor(const Mesh<VertexType, FaceType>* mesh) {
				return GetCachedOrCreate(mesh, false, [&]() { return Object::Instantiate<CachedAABB>(mesh); });
			}
		};
		static AABBCache cache;
		return cache.GetFor(mesh);
	}

	/// <summary> Gets up to date bounding box of the target mesh geometry </summary>
	template<typename VertexType, typename FaceType>
	inline AABB MeshBoundingBox<VertexType, FaceType>::GetMeshBounds() {
		AABB aabb;
		auto tryGetBounds = [&]() {
			std::unique_lock<SpinLock> lock(m_aabbLock);
			aabb = m_aabb;
			return !m_aabbDirty.load();
		};
		if (tryGetBounds())
			return aabb;
		typename Mesh<VertexType, FaceType>::Reader reader(m_mesh);
		if (tryGetBounds())
			return aabb;
		if (reader.VertCount() <= 0u)
			aabb = AABB(Vector3(0.0f), Vector3(0.0f));
		else {
			auto pos = [&](uint32_t i) { return reader.Vert(i).position; };
			aabb.start = aabb.end = pos(0u);
			for (uint32_t i = 1u; i < reader.VertCount(); i++) {
				const Vector3 position = pos(i);
				aabb.start.x = Math::Min(aabb.start.x, position.x);
				aabb.start.y = Math::Min(aabb.start.y, position.y);
				aabb.start.z = Math::Min(aabb.start.z, position.z);
				aabb.end.x = Math::Max(aabb.end.x, position.x);
				aabb.end.y = Math::Max(aabb.end.y, position.y);
				aabb.end.z = Math::Max(aabb.end.z, position.z);
			}
		}
		{
			std::unique_lock<SpinLock> lock(m_aabbLock);
			m_aabb = aabb;
			m_aabbDirty = false;
		}
		return aabb;
	}
}
