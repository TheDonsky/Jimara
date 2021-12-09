#include "FBXAssetImporter.h"
#include "FBXData.h"
#include "../../../Math/Helpers.h"

namespace {
	struct PathAndRevision {
		Jimara::OS::Path path;
		size_t revision = 0;

		inline bool operator<(const PathAndRevision& other)const { return (revision < other.revision) || ((revision == other.revision) && (path < other.path)); }
		inline bool operator!=(const PathAndRevision& other)const { return (revision != other.revision) || (path != other.path); }
		inline bool operator==(const PathAndRevision& other)const { return !((*this) != other); }
	};
}

namespace std {
	template<>
	struct hash<PathAndRevision> {
		inline size_t operator()(const PathAndRevision& pathAndRevision)const {
			return Jimara::MergeHashes(std::hash<Jimara::OS::Path>()(pathAndRevision.path), std::hash<size_t>()(pathAndRevision.revision));
		}
	};
}

namespace Jimara {
	namespace FBXHelpers {
		namespace {
			class FBXAssetImporterSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
			public:
				inline FBXAssetImporterSerializer() : Serialization::ItemSerializer("FBXAssetImporterSerializer") {}

				inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
					return Object::Instantiate<FBXAssetImporter>();
				}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
					// __TODO__: Actually, serialize this stuff...
				}

				inline static FBXAssetImporterSerializer* Instance() {
					static const Reference<FBXAssetImporterSerializer> instance = Object::Instantiate<FBXAssetImporterSerializer>();
					return instance;
				}

				inline static const OS::Path& Extension() { 
					static const OS::Path extension = ".fbx";
					return extension;
				}
			};

			struct FBXDataCache : public virtual ObjectCache<PathAndRevision>::StoredObject {
				std::vector<FBXMesh> meshes;
				std::vector<FBXSkinnedMesh> skinnedMeshes;
				std::vector<FBXAnimation> animations;
				std::unordered_map<FBXUid, FBXObject*> uidToObject;

				class Cache : public virtual ObjectCache<PathAndRevision> {
				public:
					inline static Reference<FBXDataCache> For(const PathAndRevision& pathAndRevision, OS::Logger* logger) {
						static Cache cache;
						return cache.GetCachedOrCreate(pathAndRevision, false, [&]() -> Reference<FBXDataCache> {
							Reference<FBXData> data = FBXData::Extract(pathAndRevision.path, logger);
							if (data == nullptr) return nullptr;
							Reference<FBXDataCache> instance = Object::Instantiate<FBXDataCache>();
							
							for (size_t i = 0; i < data->MeshCount(); i++) {
								const FBXMesh* mesh = data->GetMesh(i);
								const FBXSkinnedMesh* skinnedMesh = dynamic_cast<const FBXSkinnedMesh*>(data->GetMesh(i));
								if (skinnedMesh != nullptr)
									instance->skinnedMeshes.push_back(*skinnedMesh);
								else if (mesh != nullptr)
									instance->meshes.push_back(*mesh);
							}

							for (size_t i = 0; i < data->AnimationCount(); i++)
								instance->animations.push_back(*data->GetAnimation(i));

							auto recordUIDReferences = [&](auto list) {
								for (size_t i = 0; i < list.size(); i++) {
									FBXObject* object = &list[i];
									instance->uidToObject[object->uid] = object;
								}
							};
							recordUIDReferences(instance->meshes);
							recordUIDReferences(instance->skinnedMeshes);
							recordUIDReferences(instance->animations);

							// __TODO__: Add records for the FBX scene creation...
							return instance;
							});
					}
				};
			};

			template<typename ResourceType>
			class FBXAsset : public virtual Asset {
			private:
				const Reference<const FBXAssetImporter> m_importer;
				const size_t m_revision;
				const FBXUid m_fbxId;

				Reference<FBXDataCache> m_dataCache;
				FBXObject* m_targetObject = nullptr;

			public:
				inline FBXAsset(FBXAssetImporter* importer, size_t revision, FBXUid fbxId)
					: m_importer(importer), m_revision(revision), m_fbxId(fbxId) {}

			protected:
				virtual Reference<const ResourceType>* ResourceReference(FBXObject* object)const = 0;

				virtual Reference<const Resource> LoadResource() final override {
					auto failed = [&]() {
						m_targetObject = nullptr;
						m_dataCache = nullptr;
						return nullptr;
					};
					m_dataCache = FBXDataCache::Cache::For(PathAndRevision{ m_importer->AssetFilePath(), m_revision }, m_importer->Log());
					if (m_dataCache == nullptr) return failed();
					else {
						typedef typename decltype(m_dataCache->uidToObject)::const_iterator FBXIdIterator;
						FBXIdIterator it = m_dataCache->uidToObject.find(m_fbxId);
						if (it == m_dataCache->uidToObject.end()) {
							it = FBXIdIterator();
							return failed();
						}
						m_targetObject = it->second;
						if (m_targetObject == nullptr) {
							it = FBXIdIterator();
							return failed();
						}
					}
					Reference<const ResourceType>* resourceReference = ResourceReference(m_targetObject);
					if (resourceReference == nullptr) {
						m_importer->Log()->Error("FBXAsset::LoadResource - Asset type mismatch! <internal error>");
						return failed();
					}
					Reference<const ResourceType> result;
					std::swap(*resourceReference, result);
					if (result == nullptr) return failed();
					else return result;
				}
				
				inline virtual void UnloadResource(Reference<const Resource> resource) final override {
					if (resource == nullptr) 
						m_importer->Log()->Error("FBXAsset::UnloadResource - Got null resource! <internal error>");
					else if (m_dataCache == nullptr || m_targetObject == nullptr) {
						m_importer->Log()->Error("FBXAsset::UnloadResource - Resource does not seem to be loaded! <internal error>");
						return;
					}
					else {
						Reference<const ResourceType>* resourceReference = ResourceReference(m_targetObject);
						if (resourceReference == nullptr)
							m_importer->Log()->Error("FBXAsset::UnloadResource - Asset type mismatch! <internal error>");
						else if ((*resourceReference) != nullptr)
							m_importer->Log()->Error("FBXAsset::UnloadResource - Possible circular dependencies detected! <internal error>");
						else (*resourceReference) = resource;
					}
					m_targetObject = nullptr;
					m_dataCache = nullptr;
				}
			};

			class FBXMeshAsset : public virtual FBXAsset<PolyMesh> {
			public:
				inline FBXMeshAsset(const GUID& guid, FBXAssetImporter* importer, size_t revision, FBXUid fbxId)
					: Asset(guid), FBXAsset<PolyMesh>(importer, revision, fbxId) {}
			protected:
				inline virtual Reference<const PolyMesh>* ResourceReference(FBXObject* object)const final override {
					FBXMesh* fbxMesh = dynamic_cast<FBXMesh*>(object);
					if (fbxMesh == nullptr) return nullptr;
					else return &fbxMesh->mesh;
				}
			};

			class FBXTriMeshAsset : public virtual Asset {
			private:
				const Reference<FBXMeshAsset> m_meshAsset;

				Reference<const PolyMesh> m_sourceMesh;

			public:
				inline FBXTriMeshAsset(const GUID& guid, FBXMeshAsset* meshAsset)
					: Asset(guid), m_meshAsset(meshAsset) {
					assert(m_meshAsset != nullptr);
				}

			protected:
				virtual Reference<const Resource> LoadResource() final override {
					m_sourceMesh = m_meshAsset->LoadAs<const PolyMesh>();
					const SkinnedPolyMesh* skinnedSourceMesh = dynamic_cast<const SkinnedPolyMesh*>(m_sourceMesh.operator->());
					if (skinnedSourceMesh != nullptr) return ToSkinnedTriMesh(skinnedSourceMesh);
					else if (m_sourceMesh != nullptr) return ToTriMesh(m_sourceMesh);
					else return nullptr;
				}

				inline virtual void UnloadResource(Reference<const Resource> resource) final override {
					m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
				}
			};

			class FBXAnimationAsset : public virtual FBXAsset<AnimationClip> {
			public:
				inline FBXAnimationAsset(const GUID& guid, FBXAssetImporter* importer, size_t revision, FBXUid fbxId)
					: Asset(guid), FBXAsset<AnimationClip>(importer, revision, fbxId) {}
			protected:
				inline virtual Reference<const AnimationClip>* ResourceReference(FBXObject* object)const final override {
					FBXAnimation* fbxAnimation = dynamic_cast<FBXAnimation*>(object);
					if (fbxAnimation == nullptr) return nullptr;
					else return &fbxAnimation->clip;
				}
			};
		}


		JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(FBXAssetImporter, {
			FBXAssetImporterSerializer::Instance()->Register(FBXAssetImporterSerializer::Extension());
			}, { FBXAssetImporterSerializer::Instance()->Unregister(FBXAssetImporterSerializer::Extension()); });

		bool FBXAssetImporter::Import(Callback<Asset*> reportAsset) {
			// __TODO__: If last write time matches the one from the previous import, probably no need to rescan the FBX (will make startup times faster)...
			Reference<FBXData> data = FBXData::Extract(AssetFilePath(), Log());
			if (data == nullptr) return false;
			size_t revision = m_revision.fetch_add(1);

			FBXUidToGUID polyMeshGUIDs;
			FBXUidToGUID triMeshGUIDs;
			FBXUidToGUID animationGUIDs;

			auto getGuidOf = [](FBXUid uid, const FBXUidToGUID& cache, FBXUidToGUID& resultCache) -> GUID {
				FBXUidToGUID::const_iterator it = cache.find(uid);
				const GUID guid = [&]() -> GUID {
					if (it == cache.end()) return GUID::Generate();
					else return it->second;
				}();
				resultCache[uid] = guid;
				return guid;
			};

			for (size_t i = 0; i < data->MeshCount(); i++) {
				const FBXUid uid = data->GetMesh(i)->uid;
				const Reference<FBXMeshAsset> polyMeshAsset = Object::Instantiate<FBXMeshAsset>(getGuidOf(uid, m_polyMeshGUIDs, polyMeshGUIDs), this, revision, uid);
				const Reference<FBXTriMeshAsset> triMeshAsset = Object::Instantiate<FBXTriMeshAsset>(getGuidOf(uid, m_triMeshGUIDs, triMeshGUIDs), polyMeshAsset);
				reportAsset(polyMeshAsset);
				reportAsset(triMeshAsset);
			}

			for (size_t i = 0; i < data->AnimationCount(); i++) {
				const FBXUid uid = data->GetAnimation(i)->uid;
				const Reference<FBXAnimationAsset> animationAsset = Object::Instantiate<FBXAnimationAsset>(getGuidOf(uid, m_animationGUIDs, animationGUIDs), this, revision, uid);
				reportAsset(animationAsset);
			}

			// __TODO__: Add records for the FBX scene creation...

			m_polyMeshGUIDs = polyMeshGUIDs;
			m_triMeshGUIDs = triMeshGUIDs;
			m_animationGUIDs = animationGUIDs;

			return true;
		}
	}
}
