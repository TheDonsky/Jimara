#pragma once
#include "../Data/AssetDatabase/AssetDatabase.h"
#include "../Core/Collections/ObjectCache.h"
#include "../Math/Helpers.h"
#include "../Data/Mesh.h"
namespace Jimara { namespace Physics { class CollisionMesh; } }
#include "PhysicsInstance.h"

namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Physics mesh
		/// </summary>
		class CollisionMesh : public virtual Resource {
		public:
			/// <summary> In-engine mesh, the CollisionMesh represents </summary>
			inline TriMesh* Mesh()const { return m_mesh; }

			/// <summary>
			/// Retrieves a cached CollisionMesh asset for a TriMesh asset
			/// </summary>
			/// <typeparam name="MeshType"> Type of the asset (should be derived from TriMesh) </typeparam>
			/// <param name="guid"> Inique identifier of the CollisionMesh asset (this way, same meshAsset can have multiple collision mesh assets, if anyone needs that) </param>
			/// <param name="meshAsset"> Source mesh asset </param>
			/// <param name="apiInstance"> Physics API instance to tie asset to </param>
			/// <returns> Cached Asset of CollisionMesh </returns>
			template<typename MeshType>
			inline static std::enable_if_t<std::is_base_of_v<TriMesh, MeshType>, Reference<Asset::Of<CollisionMesh>>>
				GetAsset(const GUID& guid, Asset::Of<MeshType>* meshAsset, PhysicsInstance* apiInstance) {
				return GetCachedAsset(guid, meshAsset, apiInstance);
			}

			/// <summary>
			/// Retrieves  a cached CollisionMesh asset for the given mesh
			/// </summary>
			/// <param name="mesh"> "Source" mesh </param>
			/// <param name="physicsInstance"> Physics API instance to tie asset to </param>
			/// <returns> Cached Asset of CollisionMesh </returns>
			static Reference<Asset::Of<CollisionMesh>> GetAsset(TriMesh* mesh, PhysicsInstance* apiInstance);

			/// <summary> Asset::Of<TriMesh/SkinnedTriMesh or derived> that also can retrieve a 'paired' Asset::Of<CollisionMesh> </summary>
			class MeshAsset;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> In-engine mesh </param>
			inline CollisionMesh(TriMesh* mesh) : m_mesh(mesh) {}

		private:
			// In-engine mesh, the CollisionMesh represents
			const Reference<TriMesh> m_mesh;

			// Finds a cahhed asset
			static Reference<Asset::Of<CollisionMesh>> GetCachedAsset(const GUID& guid, Asset* meshAsset, PhysicsInstance* apiInstance);
		};
	}

	// Parent types of CollisionMesh
	template<> inline void TypeIdDetails::GetParentTypesOf<Physics::CollisionMesh>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }

	namespace Physics {
		/// <summary> Asset::Of<TriMesh/SkinnedTriMesh or derived> that also can retrieve a 'paired' Asset::Of<CollisionMesh> </summary>
		class CollisionMesh::MeshAsset : public virtual Asset {
		private:
			// GUID of the corresponding CollisionMeshAsset
			const GUID m_collisionMeshAssetGUID;

			// Physics API instance
			const Reference<PhysicsInstance> m_physicsInstance;

		public:
			/// <summary>
			/// Gets paired CollisionMeshAsset
			/// </summary>
			/// <returns> Reference to the CollisionMeshAsset </returns>
			inline Reference<Asset::Of<CollisionMesh>> GetCollisionMeshAsset() {
				return CollisionMesh::GetCachedAsset(m_collisionMeshAssetGUID, this, m_physicsInstance);
			}

			/// <summary> GUID of the 'paired' CollisionMesh asset </summary>
			inline GUID CollisionMeshId()const { return m_collisionMeshAssetGUID; }

			/// <summary> "Tied" Physics API instance </summary>
			inline PhysicsInstance* PhysicsAPIInstance()const { return m_physicsInstance; }

			template<typename MeshType> class Of;

		private:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collisionMeshAssetGUID"> GUID of the paired CollisionMeshAsset </param>
			/// <param name="physicsInstance"> Physics API instance </param>
			inline MeshAsset(const GUID& collisionMeshAssetGUID, PhysicsInstance* physicsInstance)
				: m_collisionMeshAssetGUID(collisionMeshAssetGUID), m_physicsInstance(physicsInstance) {}
		};

#pragma warning(disable: 4250)
		/// <summary>
		/// CollisionMesh::MeshAsset for Asset::Of<MeshType>, where MeshType is derived from TriMesh
		/// </summary>
		/// <typeparam name="MeshType"> Mesh type (should be derived from TriMesh) </typeparam>
		template<typename MeshType>
		class CollisionMesh::MeshAsset::Of : public virtual Asset::Of<MeshType>, public CollisionMesh::MeshAsset {
			static_assert(std::is_base_of_v<TriMesh, MeshType>);
		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collisionMeshAssetGUID"> GUID of the paired CollisionMeshAsset </param>
			/// <param name="physicsInstance"> Physics API instance </param>
			inline Of(const GUID& collisionMeshAssetGUID, PhysicsInstance* physicsInstance)
				: MeshAsset(collisionMeshAssetGUID, physicsInstance) {}
		};
#pragma warning(default: 4250)
	}

	// Parent types of CollisionMesh::MeshAsset
	template<> inline void TypeIdDetails::GetParentTypesOf<Physics::CollisionMesh::MeshAsset>(const Callback<TypeId>& report) { report(TypeId::Of<Asset>()); }

	/// <summary>
	/// TypeIdDetails::TypeDetails specialization for <Physics::CollisionMesh::MeshAsset::Of<>
	/// </summary>
	/// <typeparam name="MeshType"> Mesh type (should be derived from TriMesh) </typeparam>
	template<typename MeshType>
	struct TypeIdDetails::TypeDetails<Physics::CollisionMesh::MeshAsset::Of<MeshType>> {
		/// <summary>
		/// Asset::Of<MeshType> and CollisionMesh::MeshAsset
		/// </summary>
		/// <param name="reportParentType"> Each parent of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Asset::Of<MeshType>>());
			reportParentType(TypeId::Of<Physics::CollisionMesh::MeshAsset>());
		}

		/// <summary> Nothing </summary>
		inline static void GetTypeAttributes(const Callback<const Object*>&) { }
	};
}
