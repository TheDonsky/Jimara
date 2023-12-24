#include "CollisionMesh.h"

namespace Jimara {
	namespace Physics {
		namespace {
			struct CollisionMeshIdentifier {
				GUID assetId = {};
				Reference<Asset> meshAsset;
				Reference<PhysicsInstance> physicsInstance;

				inline bool operator<(const CollisionMeshIdentifier& id)const {
					if (assetId < id.assetId) return true;
					else if (assetId > id.assetId) return false;
					else if (meshAsset < id.meshAsset) return true;
					else if (meshAsset > id.meshAsset) return false;
					else return physicsInstance < id.physicsInstance;
				}

				inline bool operator==(const CollisionMeshIdentifier& id)const {
					return (assetId == id.assetId) && (meshAsset == id.meshAsset) && (physicsInstance == id.physicsInstance);
				}
			};
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Physics::CollisionMeshIdentifier> {
		inline size_t operator()(const Jimara::Physics::CollisionMeshIdentifier& id)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::GUID>()(id.assetId),
				Jimara::MergeHashes(
					std::hash<Jimara::Asset*>()(id.meshAsset),
					std::hash<Jimara::Physics::PhysicsInstance*>()(id.physicsInstance)));
		}
	};
}

namespace Jimara {
	namespace Physics {
		namespace {
#pragma warning(disable: 4250)
			class CollisionMeshAsset final
				: public virtual Asset::Of<CollisionMesh>
				, public virtual ObjectCache<CollisionMeshIdentifier>::StoredObject {
			private:
				const Reference<Asset> m_meshAsset;
				const Reference<TriMesh> m_mesh;
				const Reference<PhysicsInstance> m_physicsInstance;

			public:
				inline CollisionMeshAsset(const CollisionMeshIdentifier& id, TriMesh* mesh)
					: Asset(id.assetId), m_meshAsset(id.meshAsset), m_mesh(mesh), m_physicsInstance(id.physicsInstance) {}

			protected:
				inline virtual Reference<CollisionMesh> LoadItem()final override {
					Reference<TriMesh> mesh = m_mesh;
					if (mesh == nullptr)
						mesh = m_meshAsset->LoadAs<TriMesh>();
					if (mesh == nullptr) {
						m_physicsInstance->Log()->Error("CollisionMeshAsset::LoadItem - Failed to retrieve Mesh asset!");
						return nullptr;
					}
					else return m_physicsInstance->CreateCollisionMesh(mesh);
				}
			};
#pragma warning(default: 4250)

			class CollisionMeshAssetCache : public virtual ObjectCache<CollisionMeshIdentifier> {
			public:
				inline static Reference<CollisionMeshAsset> GetFor(
					const CollisionMeshIdentifier& id, 
					const Function<Reference<CollisionMeshAsset>>& createNew) {
					static CollisionMeshAssetCache cache;
					return cache.GetCachedOrCreate(id, [&]() -> Reference<CollisionMeshAsset> {
						return createNew();
						});
				}
			};

			static GUID GetCollisionAssetGUID(const GUID& meshId) {
				static const GUID ID = []() {
					const GUID a = GUID::Generate();
					const GUID b = GUID::Generate();
					GUID id = {};
					// Just in case we change GUID so that some part of the id remains constant on given hardware, 
					// generated CollisionMeshAsset id will still conform to the standard this way
					for (size_t i = 0; i < GUID::NUM_BYTES; i++)
						id.bytes[i] = a.bytes[i] ^ b.bytes[i];
					return id;
				}();
				GUID rv;
				for (size_t i = 0; i < GUID::NUM_BYTES; i++)
					rv.bytes[i] = meshId.bytes[i] ^ ID.bytes[i];
				return rv;
			}
		}

		Reference<Asset::Of<CollisionMesh>> CollisionMesh::GetAsset(MeshAsset* meshAsset, PhysicsInstance* apiInstance) {
			if (apiInstance == nullptr) return nullptr;
			else if (meshAsset == nullptr) {
				if (apiInstance != nullptr)
					apiInstance->Log()->Error("CollisionMesh::GetAsset - Mesh Asset missing!");
				return nullptr;
			}
			else return GetCachedAsset(meshAsset->CollisionMeshId(), meshAsset, apiInstance);
		}

		Reference<Asset::Of<CollisionMesh>> CollisionMesh::GetAsset(TriMesh* mesh, PhysicsInstance* apiInstance) {
			if (apiInstance == nullptr) return nullptr;
			else if (mesh == nullptr) {
				if (apiInstance != nullptr)
					apiInstance->Log()->Error("CollisionMesh::GetAsset - Mesh missing!");
				return nullptr;
			}
			else {
				Reference<Asset> asset = mesh->GetAsset();
				const Reference<MeshAsset> meshAsset = asset;
				if (meshAsset != nullptr)
					return GetAsset(meshAsset, apiInstance);
				else if (asset == nullptr) {
					CollisionMeshIdentifier identifier;
					{
						identifier.assetId = {};
						reinterpret_cast<TriMesh**>(identifier.assetId.bytes)[0] = mesh;
						identifier.meshAsset = nullptr;
						identifier.physicsInstance = apiInstance;
					}
					const std::pair<CollisionMeshIdentifier, TriMesh*> createArgs(identifier, mesh);
					Reference<CollisionMeshAsset>(*createNew)(decltype(createArgs)*) = [](decltype(createArgs)* args) -> Reference<CollisionMeshAsset> {
						return Object::Instantiate<CollisionMeshAsset>(args->first, args->second);
					};
					Reference<CollisionMeshAsset> rv = CollisionMeshAssetCache::GetFor(identifier, Function<Reference<CollisionMeshAsset>>(createNew, &createArgs));
					return rv;
				}
				else return GetCachedAsset(GetCollisionAssetGUID(asset->Guid()), asset, apiInstance);
			}
		}

		Reference<Asset::Of<CollisionMesh>> CollisionMesh::GetCachedAsset(const GUID& guid, Asset* meshAsset, PhysicsInstance* apiInstance) {
			if (meshAsset == nullptr) {
				if (apiInstance != nullptr)
					apiInstance->Log()->Error("CollisionMesh::GetCachedAsset - Mesh Asset missing!");
				return nullptr;
			}
			else if (apiInstance == nullptr)
				return nullptr;
			const CollisionMeshIdentifier identifier{ guid, meshAsset, apiInstance };
			Reference<CollisionMeshAsset>(*createNew)(const CollisionMeshIdentifier*) = [](const CollisionMeshIdentifier* id) -> Reference<CollisionMeshAsset> {
				return Object::Instantiate<CollisionMeshAsset>(*id, nullptr);
			};
			return CollisionMeshAssetCache::GetFor(identifier, Function<Reference<CollisionMeshAsset>>(createNew, &identifier));
		}
	}
}
