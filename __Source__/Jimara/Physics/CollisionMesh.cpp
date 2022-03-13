#include "CollisionMesh.h"

namespace Jimara {
	namespace Physics {
		namespace {
			class CollisionMeshAssetCache : public virtual ObjectCache<CollisionMeshIdentifier> {
			public:
				inline static Reference<CollisionMeshAsset> GetFor(
					const CollisionMeshIdentifier& id, 
					Reference<CollisionMeshAsset>(*createNew)(const CollisionMeshIdentifier&)) {
					static CollisionMeshAssetCache cache;
					return cache.GetCachedOrCreate(id, false, [&]() -> Reference<CollisionMeshAsset> {
						return createNew(id);
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
			Reference<CollisionMeshAsset>(*createNew)(const CollisionMeshIdentifier&) = [](const CollisionMeshIdentifier& id) -> Reference<CollisionMeshAsset> {
				Reference<CollisionMeshAsset> instance = new CollisionMeshAsset(id);
				instance->ReleaseRef();
				return instance;
			};
			return CollisionMeshAssetCache::GetFor(identifier, createNew);
		}

		Reference<CollisionMeshAsset> CollisionMeshAsset::GetFor(const GUID& assetId, Asset::Of<TriMesh>* meshAsset, PhysicsInstance* physicsInstance) {
			return GetFor({ assetId, meshAsset, physicsInstance });
		}

		namespace {
#pragma warning(disable: 4250)
			class TriMeshPtrAsset 
				: public virtual Asset::Of<TriMesh>
				, public virtual ObjectCache<Reference<const TriMesh>>::StoredObject {
			private:
				const Reference<const TriMesh> m_originalMesh;
				Reference<TriMesh> m_mesh;
				TriMesh* m_dataPtr;

				inline void OnMeshDirty(const TriMesh* mesh) {
					(*m_dataPtr) = (*mesh);
				}

			protected:
				inline virtual Reference<TriMesh> LoadItem() final override {
					Reference<TriMesh> resource = m_mesh;
					m_mesh = nullptr;
					return resource;
				}

				inline virtual void UnloadItem(TriMesh* resource) final override {
					m_mesh = resource;
				}

			public:
				inline TriMeshPtrAsset(const TriMesh* mesh) 
					: Asset(GUID::Generate()), m_originalMesh(mesh) {
					TriMesh::Reader reader(mesh);
					m_mesh = Object::Instantiate<TriMesh>(*mesh);
					m_dataPtr = m_mesh;
					m_originalMesh->OnDirty() += Callback(&TriMeshPtrAsset::OnMeshDirty, this);
				}

				inline virtual ~TriMeshPtrAsset() {
					m_originalMesh->OnDirty() -= Callback(&TriMeshPtrAsset::OnMeshDirty, this);
				}

				class Cache : public virtual ObjectCache<Reference<const TriMesh>> {
				public:
					inline static Reference<TriMeshPtrAsset> GetFor(const TriMesh* mesh) {
						static Cache cache;
						return cache.GetCachedOrCreate(mesh, false, [&]()->Reference<TriMeshPtrAsset> {
							return Object::Instantiate<TriMeshPtrAsset>(mesh);
							});
					}
				};
			};
#pragma warning(default: 4250)
		}

		Reference<CollisionMeshAsset> CollisionMeshAsset::GetFor(const TriMesh* mesh, PhysicsInstance* physicsInstance) {
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
				else if (asset == nullptr)
					asset = TriMeshPtrAsset::Cache::GetFor(mesh);
				return GetFor(asset->Guid(), asset, physicsInstance);
			}
		}

		Reference<CollisionMesh> CollisionMeshAsset::LoadItem() {
			Reference<TriMesh> mesh = m_meshAsset->Load();
			return m_physicsInstance->CreateCollisionMesh(mesh);
		}

		CollisionMeshAsset::CollisionMeshAsset(const CollisionMeshIdentifier& id)
			: Asset(id.assetId), m_meshAsset(id.meshAsset), m_physicsInstance(id.physicsInstance) {}
	}
}
