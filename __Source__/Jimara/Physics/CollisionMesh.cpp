#include "CollisionMesh.h"

namespace Jimara {
	namespace Physics {
		namespace {
			class CollisionMeshAssetCache : public virtual ObjectCache<CollisionMeshIdentifier> {
			public:
				inline static Reference<CollisionMeshAsset> GetFor(
					const CollisionMeshIdentifier& id, 
					const Function<Reference<CollisionMeshAsset>>& createNew) {
					static CollisionMeshAssetCache cache;
					return cache.GetCachedOrCreate(id, false, [&]() -> Reference<CollisionMeshAsset> {
						return createNew();
						});
				}
			};
		}

		Reference<CollisionMeshAsset> CollisionMeshAsset::GetFor(const CollisionMeshIdentifier& identifier) {
			if (identifier.meshAsset == nullptr) {
				if (identifier.physicsInstance != nullptr)
					identifier.physicsInstance->Log()->Error("CollisionMeshAsset::GetFor - Mesh Asset missing!");
				return nullptr;
			}
			else if (identifier.physicsInstance == nullptr)
				return nullptr;
			Reference<CollisionMeshAsset>(*createNew)(const CollisionMeshIdentifier*) = [](const CollisionMeshIdentifier* id) -> Reference<CollisionMeshAsset> {
				Reference<CollisionMeshAsset> instance = new CollisionMeshAsset(*id, nullptr);
				instance->ReleaseRef();
				return instance;
			};
			return CollisionMeshAssetCache::GetFor(identifier, Function<Reference<CollisionMeshAsset>>(createNew, &identifier));
		}

		Reference<CollisionMeshAsset> CollisionMeshAsset::GetFor(const GUID& assetId, Asset::Of<TriMesh>* meshAsset, PhysicsInstance* physicsInstance) {
			return GetFor({ assetId, meshAsset, physicsInstance });
		}

		Reference<CollisionMeshAsset> CollisionMeshAsset::GetFor(TriMesh* mesh, PhysicsInstance* physicsInstance) {
			if (mesh == nullptr) {
				if (physicsInstance != nullptr)
					physicsInstance->Log()->Error("CollisionMeshAsset::GetFor - Mesh missing!");
				return nullptr;
			}
			else {
				Reference<Asset::Of<TriMesh>> asset = dynamic_cast<Asset::Of<TriMesh>*>(mesh->GetAsset());
				const Reference<MeshAsset> meshAsset = asset;
				if (meshAsset != nullptr)
					return meshAsset->GetCollisionMeshAsset();
				else if (asset == nullptr) {
					if (physicsInstance == nullptr)
						return nullptr;
					CollisionMeshIdentifier identifier;
					{
						identifier.assetId = {};
						reinterpret_cast<TriMesh**>(identifier.assetId.bytes)[0] = mesh;
						identifier.meshAsset = nullptr;
						identifier.physicsInstance = physicsInstance;
					}
					const std::pair<CollisionMeshIdentifier, TriMesh*> createArgs(identifier, mesh);
					Reference<CollisionMeshAsset>(*createNew)(decltype(createArgs)*) = [](decltype(createArgs)* args) -> Reference<CollisionMeshAsset> {
						Reference<CollisionMeshAsset> instance = new CollisionMeshAsset(args->first, args->second);
						instance->ReleaseRef();
						return instance;
					};
					Reference<CollisionMeshAsset> rv = CollisionMeshAssetCache::GetFor(identifier, Function<Reference<CollisionMeshAsset>>(createNew, &createArgs));
					return rv;
				}
				else return GetFor(asset->Guid(), asset, physicsInstance);
			}
		}

		Reference<CollisionMesh> CollisionMeshAsset::LoadItem() {
			Reference<TriMesh> mesh = m_mesh;
			if (mesh == nullptr)
				mesh = m_meshAsset->Load();
			return m_physicsInstance->CreateCollisionMesh(mesh);
		}

		CollisionMeshAsset::CollisionMeshAsset(const CollisionMeshIdentifier& id, TriMesh* mesh)
			: Asset(id.assetId), m_meshAsset(id.meshAsset), m_mesh(mesh), m_physicsInstance(id.physicsInstance) {}
	}
}
