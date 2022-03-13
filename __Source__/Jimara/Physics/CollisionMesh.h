#pragma once
#include "../Data/AssetDatabase/AssetDatabase.h"
#include "../Core/Collections/ObjectCache.h"
#include "../Math/Helpers.h"
#include "../Data/Mesh.h"
namespace Jimara {
	namespace Physics {
		class CollisionMesh;
		class CollisionMeshAsset;
		struct CollisionMeshIdentifier;
	}
}
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

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="mesh"> In-engine mesh </param>
			inline CollisionMesh(TriMesh* mesh) : m_mesh(mesh) {}

		private:
			// In-engine mesh, the CollisionMesh represents
			const Reference<TriMesh> m_mesh;
		};
	}

	// Parent types of CollisionMesh
	template<> inline void TypeIdDetails::GetParentTypesOf<Physics::CollisionMesh>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }

	namespace Physics {
		/// <summary>
		/// Unique identifier for a CollisionMeshAsset
		/// </summary>
		struct CollisionMeshIdentifier {
			/// <summary> 
			/// GUID of the CollisionMeshAsset
			/// Note: Yes, in theory several CollisionMeshAsset objects can have the same GUID, but that should not happen in general;
			///		(only reson this is here is to make it more straightforward for the cache to function)
			/// </summary>
			GUID assetId = {};

			/// <summary> Mesh asset for the in-engine mesh retrieval </summary>
			Reference<Asset::Of<TriMesh>> meshAsset;

			/// <summary> Physics API instance </summary>
			Reference<PhysicsInstance> physicsInstance;

			/// <summary>
			/// Checks if other CollisionMeshIdentifier is 'less' than this one
			/// </summary>
			/// <param name="id"> Other CollisionMeshIdentifier </param>
			/// <returns> True, if this is 'less than' id </returns>
			inline bool operator<(const CollisionMeshIdentifier& id)const {
				if (assetId < id.assetId) return true;
				else if (assetId > id.assetId) return false;
				else if (meshAsset < id.meshAsset) return true;
				else if (meshAsset > id.meshAsset) return false;
				else return physicsInstance < id.physicsInstance;
			}

			/// <summary>
			/// Checks if two CollisionMeshIdentifiers are equal
			/// </summary>
			/// <param name="id"> Other CollisionMeshIdentifier </param>
			/// <returns> True, if this is the same as id </returns>
			inline bool operator==(const CollisionMeshIdentifier& id)const {
				return (assetId == id.assetId) && (meshAsset == id.meshAsset) && (physicsInstance == id.physicsInstance);
			}
		};
	}
}

namespace std {
	/// <summary>
	/// std::hash specialization for Jimara::Physics::CollisionMeshIdentifier
	/// </summary>
	template<>
	struct hash<Jimara::Physics::CollisionMeshIdentifier> {
		/// <summary>
		/// Hash function for Jimara::Physics::CollisionMeshIdentifier
		/// </summary>
		/// <param name="id"> Jimara::Physics::CollisionMeshIdentifier </param>
		/// <returns> Hash of id </returns>
		inline size_t operator()(const Jimara::Physics::CollisionMeshIdentifier& id)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::GUID>()(id.assetId),
				Jimara::MergeHashes(
					std::hash<Jimara::Asset::Of<Jimara::TriMesh>*>()(id.meshAsset),
					std::hash<Jimara::Physics::PhysicsInstance*>()(id.physicsInstance)));
		}
	};
}

namespace Jimara {
	namespace Physics {
#pragma warning(disable: 4250)
		/// <summary>
		/// Asset for Collision meshes
		/// </summary>
		class CollisionMeshAsset final
			: public virtual Asset::Of<CollisionMesh>
			, public virtual ObjectCache<CollisionMeshIdentifier>::StoredObject {
		public:
			/// <summary>
			/// Retrieves an instance of a cached CollisionMeshAsset for the given identifier
			/// </summary>
			/// <param name="identifier"> Asset identifier </param>
			/// <returns> Cached instance of a CollisionMeshAsset </returns>
			static Reference<CollisionMeshAsset> GetFor(const CollisionMeshIdentifier& identifier);

			/// <summary>
			/// Retrieves an instance of a cached CollisionMeshAsset for the given identifier
			/// </summary>
			/// <param name="assetId"> CollisionMeshIdentifier.assetId </param>
			/// <param name="meshAsset"> CollisionMeshIdentifier.meshAsset </param>
			/// <param name="physicsInstance"> CollisionMeshIdentifier.physicsInstance </param>
			/// <returns> Cached instance of a CollisionMeshAsset </returns>
			static Reference<CollisionMeshAsset> GetFor(const GUID& assetId, Asset::Of<TriMesh>* meshAsset, PhysicsInstance* physicsInstance);

			/// <summary>
			/// Retrieves an instance of a cached CollisionMeshAsset for the given identifier
			/// </summary>
			/// <param name="mesh"> "Source" mesh </param>
			/// <param name="physicsInstance"> CollisionMeshIdentifier.physicsInstance </param>
			/// <returns> Cached instance of a CollisionMeshAsset </returns>
			static Reference<CollisionMeshAsset> GetFor(TriMesh* mesh, PhysicsInstance* physicsInstance);

			/// <summary> Asset::Of<TriMesh> that also can retrieve 'corresponding' CollisionMeshAsset </summary>
			class MeshAsset;

		protected:
			/// <summary>
			/// Loads mesh from the mesh asset and creates corresponding CollisionMesh
			/// </summary>
			/// <returns> new CollisionMesh instance </returns>
			virtual Reference<CollisionMesh> LoadItem()final override;

		private:
			// Mesh asset for the in-engine mesh retrieval
			const Reference<Asset::Of<TriMesh>> m_meshAsset;

			// 'Preloaded mesh'
			const Reference<TriMesh> m_mesh;

			// Physics API instance
			const Reference<PhysicsInstance> m_physicsInstance;

			// Constructor
			CollisionMeshAsset(const CollisionMeshIdentifier& id, TriMesh* mesh);
		};
#pragma warning(default: 4250)

		/// <summary> Asset::Of<TriMesh> that also can retrieve 'corresponding' CollisionMeshAsset </summary>
		class CollisionMeshAsset::MeshAsset : public virtual Asset::Of<TriMesh> {
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
			inline Reference<CollisionMeshAsset> GetCollisionMeshAsset() {
				return CollisionMeshAsset::GetFor(m_collisionMeshAssetGUID, this, m_physicsInstance);
			}

			/// <summary> "Tied" Physics API instance </summary>
			inline PhysicsInstance* PhysicsAPIInstance()const { return m_physicsInstance; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collisionMeshAssetGUID"> GUID of the paired CollisionMeshAsset </param>
			/// <param name="physicsInstance"> Physics API instance </param>
			inline MeshAsset(const GUID& collisionMeshAssetGUID, PhysicsInstance* physicsInstance)
				: m_collisionMeshAssetGUID(collisionMeshAssetGUID), m_physicsInstance(physicsInstance) {}
		};
	}

	// Parent types of CollisionMeshAsset
	template<> inline void TypeIdDetails::GetParentTypesOf<Physics::CollisionMeshAsset>(const Callback<TypeId>& report) { 
		report(TypeId::Of<Asset::Of<Physics::CollisionMesh>>());
		report(TypeId::Of<ObjectCache<Physics::CollisionMeshIdentifier>::StoredObject>());
	}

	// Parent types of CollisionMeshAsset::MeshAsset
	template<> inline void TypeIdDetails::GetParentTypesOf<Physics::CollisionMeshAsset::MeshAsset>(const Callback<TypeId>& report) {
		report(TypeId::Of<Asset::Of<TriMesh>>());
	}
}
