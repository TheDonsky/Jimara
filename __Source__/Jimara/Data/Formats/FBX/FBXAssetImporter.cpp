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
				const Reference<const FileSystemDatabase::AssetImporter> m_importer;
				const size_t m_revision;
				const FBXUid m_fbxId;

				Reference<FBXDataCache> m_dataCache;
				FBXObject* m_targetObject = nullptr;

			public:
				inline FBXAsset(FileSystemDatabase::AssetImporter* importer, size_t revision, FBXUid fbxId)
					: m_importer(importer), m_revision(revision), m_fbxId(fbxId) {}

			protected:
				virtual Reference<ResourceType>* ResourceReference(FBXObject* object)const = 0;

				virtual Reference<Resource> LoadResource() final override {
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
					Reference<ResourceType>* resourceReference = ResourceReference(m_targetObject);
					if (resourceReference == nullptr) {
						m_importer->Log()->Error("FBXAsset::LoadResource - Asset type mismatch! <internal error>");
						return failed();
					}
					Reference<ResourceType> result;
					std::swap(*resourceReference, result);
					if (result == nullptr) return failed();
					else return result;
				}

				inline virtual void UnloadResource(Reference<Resource> resource) final override {
					if (resource == nullptr)
						m_importer->Log()->Error("FBXAsset::UnloadResource - Got null resource! <internal error>");
					else if (m_dataCache == nullptr || m_targetObject == nullptr) {
						m_importer->Log()->Error("FBXAsset::UnloadResource - Resource does not seem to be loaded! <internal error>");
						return;
					}
					else {
						Reference<ResourceType>* resourceReference = ResourceReference(m_targetObject);
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
				inline FBXMeshAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, size_t revision, FBXUid fbxId)
					: Asset(guid), FBXAsset<PolyMesh>(importer, revision, fbxId) {}
			protected:
				inline virtual Reference<PolyMesh>* ResourceReference(FBXObject* object)const final override {
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
				virtual Reference<Resource> LoadResource() final override {
					m_sourceMesh = m_meshAsset->LoadAs<PolyMesh>();
					const SkinnedPolyMesh* skinnedSourceMesh = dynamic_cast<const SkinnedPolyMesh*>(m_sourceMesh.operator->());
					if (skinnedSourceMesh != nullptr) return ToSkinnedTriMesh(skinnedSourceMesh);
					else if (m_sourceMesh != nullptr) return ToTriMesh(m_sourceMesh);
					else return nullptr;
				}

				inline virtual void UnloadResource(Reference<Resource> resource) final override {
					m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
				}
			};

			class FBXAnimationAsset : public virtual FBXAsset<AnimationClip> {
			public:
				inline FBXAnimationAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, size_t revision, FBXUid fbxId)
					: Asset(guid), FBXAsset<AnimationClip>(importer, revision, fbxId) {}
			protected:
				inline virtual Reference<AnimationClip>* ResourceReference(FBXObject* object)const final override {
					FBXAnimation* fbxAnimation = dynamic_cast<FBXAnimation*>(object);
					if (fbxAnimation == nullptr) return nullptr;
					else return &fbxAnimation->clip;
				}
			};





			class FBXImporterSerializer;


			class FBXImporter : public virtual FileSystemDatabase::AssetImporter {
			public:
				inline virtual bool Import(Callback<Asset*> reportAsset) final override {
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

					m_polyMeshGUIDs = std::move(polyMeshGUIDs);
					m_triMeshGUIDs = std::move(triMeshGUIDs);
					m_animationGUIDs = std::move(animationGUIDs);

					return true;
				}

			private:
				std::atomic<size_t> m_revision = 0;

				typedef std::map<FBXUid, GUID> FBXUidToGUID;
				FBXUidToGUID m_polyMeshGUIDs;
				FBXUidToGUID m_triMeshGUIDs;
				FBXUidToGUID m_animationGUIDs;

				friend class FBXImporterSerializer;

				typedef std::pair<FBXUid, GUID> GuidMapping;
				class GuidMappingSerializer : public virtual Serialization::SerializerList::From<GuidMapping> {
				private:
					inline GuidMappingSerializer() : Serialization::ItemSerializer("Mapping") {}

				public:
					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, GuidMapping* target)const final override {
						{
							static const Reference<const ItemSerializer::Of<FBXUid>> fbxIdSerializer = Serialization::ValueSerializer<FBXUid>::Create("FBXUid");
							recordElement(fbxIdSerializer->Serialize(target->first));
						}
						{
							static const Reference<const GUID::Serializer> guidSerializer = Object::Instantiate<GUID::Serializer>("GUID");
							recordElement(guidSerializer->Serialize(target->second));
						}
					}

					inline static const GuidMappingSerializer& Instance() {
						static const GuidMappingSerializer serializer;
						return serializer;
					}
				};

				class FBXUidToGUIDSerializer : public virtual Serialization::SerializerList::From<FBXUidToGUID> {
				public:
					inline FBXUidToGUIDSerializer(const std::string_view& name) : Serialization::ItemSerializer(name) {}

					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FBXUidToGUID* target)const final override {
						std::vector<GuidMapping> mappings(target->begin(), target->end());
						{
							static const Reference<const ItemSerializer::Of<std::vector<GuidMapping>>> countSerializer = Serialization::ValueSerializer<int64_t>::For<std::vector<GuidMapping>>(
								"Count", "Number of entries",
								[](std::vector<GuidMapping>* mappings) -> int64_t { return static_cast<int64_t>(mappings->size()); },
								[](const int64_t& size, std::vector<GuidMapping>* mappings) { mappings->resize(static_cast<size_t>(size)); });
							recordElement(countSerializer->Serialize(mappings));
						}
						bool dirty = (mappings.size() != target->size());
						for (size_t i = 0; i < mappings.size(); i++) {
							GuidMapping& mapping = mappings[i];
							GuidMapping oldMapping = mapping;
							recordElement(GuidMappingSerializer::Instance().Serialize(mapping));
							if (oldMapping.first != mapping.first || oldMapping.second != mapping.second)
								dirty = true;
						}
						if (dirty) {
							target->clear();
							for (size_t i = 0; i < mappings.size(); i++) {
								GuidMapping& mapping = mappings[i];
								target->operator[](mapping.first) = mapping.second;
							};
						}
					}
				};
			};





			class FBXImporterSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
			public:
				inline FBXImporterSerializer() : Serialization::ItemSerializer("FBXAssetImporterSerializer") {}

				inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
					return Object::Instantiate<FBXImporter>();
				}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
					if (target == nullptr) return;
					FBXImporter* importer = dynamic_cast<FBXImporter*>(target);
					if (importer == nullptr) {
						target->Log()->Error("FBXImporterSerializer::GetFields - Target not of the correct type!");
						return;
					}
					{
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> polyMeshGUIDSerializer = Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Polygonal meshes");
						recordElement(polyMeshGUIDSerializer->Serialize(importer->m_polyMeshGUIDs));
					}
					{
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> triMeshGUIDSerializer = Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Triangle meshes");
						recordElement(triMeshGUIDSerializer->Serialize(importer->m_triMeshGUIDs));
					}
					{
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> animationGUIDSerializer = Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Animations");
						recordElement(animationGUIDSerializer->Serialize(importer->m_animationGUIDs));
					}
				}

				inline static FBXImporterSerializer* Instance() {
					static const Reference<FBXImporterSerializer> instance = Object::Instantiate<FBXImporterSerializer>();
					return instance;
				}

				inline static const OS::Path& Extension() { 
					static const OS::Path extension = ".fbx";
					return extension;
				}
			};
		}
	}

	template<> void TypeIdDetails::OnRegisterType<FBXHelpers::FBXAssetImporter>() { 
		FBXHelpers::FBXImporterSerializer::Instance()->Register(FBXHelpers::FBXImporterSerializer::Extension());
	}
	template<> void TypeIdDetails::OnUnregisterType<FBXHelpers::FBXAssetImporter>() {
		FBXHelpers::FBXImporterSerializer::Instance()->Unregister(FBXHelpers::FBXImporterSerializer::Extension());
	}
}
