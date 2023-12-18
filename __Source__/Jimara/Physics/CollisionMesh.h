#pragma once
#include "../Data/AssetDatabase/AssetDatabase.h"
#include "../Core/Collections/ObjectCache.h"
#include "../Data/Geometry/Mesh.h"
#include "../Math/Helpers.h"
namespace Jimara { namespace Physics { class CollisionMesh; } }
#include "PhysicsInstance.h"

namespace Jimara {
	namespace Physics {
		/// <summary>
		/// Physics mesh
		/// </summary>
		class JIMARA_API CollisionMesh : public virtual Resource {
		public:
			/// <summary> In-engine mesh, the CollisionMesh represents </summary>
			inline TriMesh* Mesh()const { return m_mesh; }

			/// <summary> Asset::Of<TriMesh/SkinnedTriMesh or derived> that also can retrieve a 'paired' Asset::Of<CollisionMesh> </summary>
			class MeshAsset;

			/// <summary>
			/// Retrieves  a cached CollisionMesh asset for the given mesh
			/// </summary>
			/// <param name="meshAsset"> "Source" mesh asset </param>
			/// <param name="physicsInstance"> Physics API instance to tie asset to </param>
			/// <returns> Cached Asset of CollisionMesh </returns>
			static Reference<Asset::Of<CollisionMesh>> GetAsset(MeshAsset* meshAsset, PhysicsInstance* apiInstance);

			/// <summary>
			/// Retrieves  a cached CollisionMesh asset for the given mesh
			/// </summary>
			/// <param name="mesh"> "Source" mesh </param>
			/// <param name="physicsInstance"> Physics API instance to tie asset to </param>
			/// <returns> Cached Asset of CollisionMesh </returns>
			static Reference<Asset::Of<CollisionMesh>> GetAsset(TriMesh* mesh, PhysicsInstance* apiInstance);

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
		class JIMARA_API CollisionMesh::MeshAsset : public virtual Asset {
		private:
			// GUID of the corresponding CollisionMeshAsset
			const GUID m_collisionMeshAssetGUID;

		public:
			/// <summary> GUID of the 'paired' CollisionMesh asset </summary>
			inline GUID CollisionMeshId()const { return m_collisionMeshAssetGUID; }

			template<typename MeshType> class Of;

		private:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collisionMeshAssetGUID"> GUID of the paired CollisionMeshAsset </param>
			inline MeshAsset(const GUID& collisionMeshAssetGUID) : m_collisionMeshAssetGUID(collisionMeshAssetGUID) {}
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
			inline Of(const GUID& collisionMeshAssetGUID) : MeshAsset(collisionMeshAssetGUID) {}
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
