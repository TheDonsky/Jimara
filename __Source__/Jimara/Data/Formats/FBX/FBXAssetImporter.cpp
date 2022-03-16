#include "FBXAssetImporter.h"
#include "FBXData.h"
#include "../../ComponentHeirarchySpowner.h"
#include "../../../Components/GraphicsObjects/MeshRenderer.h"
#include "../../../Components/GraphicsObjects/SkinnedMeshRenderer.h"
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

							auto recordUIDReferences = [&](auto& list) {
								for (size_t i = 0; i < list.size(); i++) {
									FBXObject* object = dynamic_cast<FBXObject*>(&list[i]);
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

			template<typename Type, typename ResourceReferenceType = Type>
			class FBXAsset : public virtual Asset::Of<Type> {
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
				virtual Reference<ResourceReferenceType>* ResourceReference(FBXObject* object)const = 0;

				virtual Reference<Type> LoadItem() final override {
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
					Reference<ResourceReferenceType>* resourceReference = ResourceReference(m_targetObject);
					if (resourceReference == nullptr) {
						m_importer->Log()->Error("FBXAsset::LoadResource - Asset type mismatch! <internal error>");
						return failed();
					}
					Reference<ResourceReferenceType> result;
					std::swap(*resourceReference, result);
					if (result == nullptr) return failed();
					else return result;
				}

				inline virtual void UnloadItem(Type* resource) final override {
					if (resource == nullptr)
						m_importer->Log()->Error("FBXAsset::UnloadResource - Got null resource! <internal error>");
					else if (m_dataCache == nullptr || m_targetObject == nullptr) {
						m_importer->Log()->Error("FBXAsset::UnloadResource - Resource does not seem to be loaded! <internal error>");
						return;
					}
					else {
						Reference<ResourceReferenceType>* resourceReference = ResourceReference(m_targetObject);
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

			class FBXSkinnedMeshAsset : public virtual FBXAsset<SkinnedPolyMesh, PolyMesh> {
			public:
				inline FBXSkinnedMeshAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, size_t revision, FBXUid fbxId)
					: Asset(guid), FBXAsset<SkinnedPolyMesh, PolyMesh>(importer, revision, fbxId) {}
			protected:
				inline virtual Reference<PolyMesh>* ResourceReference(FBXObject* object)const final override {
					FBXMesh* fbxMesh = dynamic_cast<FBXMesh*>(object);
					if (fbxMesh == nullptr) return nullptr;
					else return &fbxMesh->mesh;
				}
			};

#pragma warning(disable: 4250)
			class FBXTriMeshAsset : public virtual Physics::CollisionMesh::MeshAsset::Of<TriMesh> {
			private:
				const Reference<FBXMeshAsset> m_meshAsset;

				Reference<const PolyMesh> m_sourceMesh;

			public:
				inline FBXTriMeshAsset(const GUID& guid, const GUID& collisionMeshId, FBXMeshAsset* meshAsset)
					: Asset(guid), Physics::CollisionMesh::MeshAsset::Of<TriMesh>(collisionMeshId), m_meshAsset(meshAsset) {
					assert(m_meshAsset != nullptr);
				}

			protected:
				virtual Reference<TriMesh> LoadItem() final override {
					m_sourceMesh = m_meshAsset->Load();
					return ToTriMesh(m_sourceMesh);
				}

				inline virtual void UnloadItem(TriMesh* resource) final override {
					m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
				}
			};

			class FBXSkinnedTriMeshAsset : public Physics::CollisionMesh::MeshAsset::Of<SkinnedTriMesh> {
			private:
				const Reference<FBXSkinnedMeshAsset> m_meshAsset;

				Reference<const SkinnedPolyMesh> m_sourceMesh;

			public:
				inline FBXSkinnedTriMeshAsset(const GUID& guid, const GUID& collisionMeshId, FBXSkinnedMeshAsset* meshAsset)
					: Asset(guid), Physics::CollisionMesh::MeshAsset::Of<SkinnedTriMesh>(collisionMeshId), m_meshAsset(meshAsset) {
					assert(m_meshAsset != nullptr);
				}

			protected:
				virtual Reference<SkinnedTriMesh> LoadItem() final override {
					m_sourceMesh = m_meshAsset->Load();
					return ToSkinnedTriMesh(m_sourceMesh);
				}

				inline virtual void UnloadItem(SkinnedTriMesh* resource) final override {
					m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
				}
			};
#pragma warning(default: 4250)

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

			class FBXHeirarchyAsset : public virtual Asset::Of<ComponentHeirarchySpowner> {
			private:
				struct MeshInfo {
					Reference<Asset> mesh;
					size_t rootBoneId = 0;
					std::vector<size_t> boneNodes;
				};

				struct Node {
					std::string name;
					Vector3 position = Vector3(0.0f);
					Vector3 rotation = Vector3(0.0f);
					Vector3 scale = Vector3(1.0f);
					size_t parent = 0;
					Stacktor<MeshInfo, 1> meshes;
				};

				const Reference<const FileSystemDatabase::AssetImporter> m_importer;
				const std::vector<Node> m_nodes;

				class Spowner : public virtual ComponentHeirarchySpowner {
				private:
					const Reference<FBXHeirarchyAsset> m_asset;
					const std::vector<Reference<Resource>> m_resources;

				public:
					inline Spowner(FBXHeirarchyAsset* asset, std::vector<Reference<Resource>>&& resources) : m_asset(asset), m_resources(std::move(resources)) {}

					inline virtual Reference<Component> SpownHeirarchy(Component* parent) final override {
						if (parent == nullptr)
							return nullptr;

						std::unique_lock<std::recursive_mutex> lock(parent->Context()->UpdateLock());
						std::vector<Reference<Transform>> transforms;

						// Create all components:
						for (size_t nodeId = 0; nodeId < m_asset->m_nodes.size(); nodeId++) {
							const Node& node = m_asset->m_nodes[nodeId];
							transforms.push_back(Object::Instantiate<Transform>(
								node.parent >= nodeId ? parent : (Component*)transforms[node.parent].operator->(),
								node.name, node.position, node.rotation, node.scale));
							for (size_t meshId = 0; meshId < node.meshes.Size(); meshId++) {
								Reference<TriMesh> mesh = node.meshes[meshId].mesh->LoadResource();
								if (mesh == nullptr)
									parent->Context()->Log()->Error("FBXHeirarchyAsset::Spowner::SpownHeirarchy - Failed to load the mesh!");
								SkinnedTriMesh* skinnedMesh = dynamic_cast<SkinnedTriMesh*>(mesh.operator->());
								if (skinnedMesh != nullptr)
									Object::Instantiate<SkinnedMeshRenderer>(transforms.back(), TriMesh::Reader(mesh).Name(), mesh);
								else Object::Instantiate<MeshRenderer>(transforms.back(), TriMesh::Reader(mesh).Name(), mesh);
							}
						}
						if (transforms.size() > 0)
							transforms[0]->Name() = OS::Path(m_asset->m_importer->AssetFilePath().stem());

						// Set bones:
						for (size_t nodeId = 0; nodeId < m_asset->m_nodes.size(); nodeId++) {
							const Node& node = m_asset->m_nodes[nodeId];
							if (transforms.size() <= nodeId) {
								parent->Context()->Log()->Error("FBXHeirarchyAsset::Spowner::SpownHeirarchy - Internal error: Not enough transforms!");
								break;
							}
							Transform* transform = transforms[nodeId];
							for (size_t meshId = 0; meshId < node.meshes.Size(); meshId++) {
								if (meshId >= transform->ChildCount()) {
									parent->Context()->Log()->Error("FBXHeirarchyAsset::Spowner::SpownHeirarchy - Internal error: Not enough renderers!");
									break;
								}
								SkinnedMeshRenderer* renderer = dynamic_cast<SkinnedMeshRenderer*>(transform->GetChild(meshId));
								if (renderer != nullptr) {
									const MeshInfo& meshInfo = node.meshes[meshId];
									auto getTransform = [&](size_t index) -> Transform* {
										if (index >= transforms.size()) return nullptr;
										else return transforms[index];
									};
									renderer->SetSkeletonRoot(getTransform(meshInfo.rootBoneId));
									for (size_t boneIndex = 0; boneIndex < meshInfo.boneNodes.size(); boneIndex++)
										renderer->SetBone(boneIndex, getTransform(meshInfo.boneNodes[boneIndex]));
								}
							}
						}

						return (transforms.size() > 0) ? transforms[0] : nullptr;
					}
				};

				inline static void AppendNode(std::vector<Node>& nodes, std::vector<const FBXNode*>& sourceNodes, const FBXNode* fbxNode, size_t parentId) {
					Node node;
					node.name = fbxNode->name;
					node.position = fbxNode->position;
					node.rotation = fbxNode->rotation;
					node.scale = fbxNode->scale;
					node.parent = parentId;
					size_t index = nodes.size();
					nodes.push_back(node);
					sourceNodes.push_back(fbxNode);
					for (size_t i = 0; i < fbxNode->children.size(); i++)
						AppendNode(nodes, sourceNodes, fbxNode->children[i], index);
				}

				inline static void AddMeshes(
					std::vector<Node>& nodes, 
					const std::vector<const FBXNode*>& sourceNodes,
					const Function<Asset*, FBXUid>& findTriMeshByUID) {
					std::unordered_map<FBXUid, size_t> nodeIndex;
					for (size_t i = 0; i < sourceNodes.size(); i++)
						nodeIndex[sourceNodes[i]->uid] = i;
					auto findBoneIndex = [&](std::optional<FBXUid> uid) -> size_t {
						std::unordered_map<FBXUid, size_t>::const_iterator it;
						if (!uid.has_value()) it = nodeIndex.end();
						else it = nodeIndex.find(uid.value());
						if (it == nodeIndex.end())
							return ~(size_t(0));
						else return it->second;
					};

					for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
						Node& node = nodes[nodeIndex];
						const FBXNode* fbxNode = sourceNodes[nodeIndex];
						for (size_t meshIndex = 0; meshIndex < fbxNode->meshes.Size(); meshIndex++) {
							const FBXMesh* mesh = fbxNode->meshes[meshIndex];
							if (mesh == nullptr) continue;
							MeshInfo meshInfo;
							meshInfo.mesh = findTriMeshByUID(mesh->uid);
							if (meshInfo.mesh == nullptr) continue; // This should not happen...
							const FBXSkinnedMesh* skinnedMesh = dynamic_cast<const FBXSkinnedMesh*>(mesh);
							if (skinnedMesh != nullptr) {
								meshInfo.rootBoneId = findBoneIndex(skinnedMesh->rootBoneId);
								for (size_t boneIndex = 0; boneIndex < skinnedMesh->boneIds.size(); boneIndex++)
									meshInfo.boneNodes.push_back(findBoneIndex(skinnedMesh->boneIds[boneIndex]));
							}
							node.meshes.Push(meshInfo);
						}
					}
				}

			public:
				inline FBXHeirarchyAsset(const GUID& guid, const FileSystemDatabase::AssetImporter* importer, FBXData* data, const Function<Asset*, FBXUid>& findTriMeshByUID)
					: Asset(guid)
					, m_importer(importer)
					, m_nodes([&]()-> std::vector<Node> {
					std::vector<Node> result;
					std::vector<const FBXNode*> sourceNodes;
					AppendNode(result, sourceNodes, data->RootNode(), 0);
					AddMeshes(result, sourceNodes, findTriMeshByUID);
					return result;
						}()) {}

			protected:
				inline virtual Reference<ComponentHeirarchySpowner> LoadItem() final override {
					// Load individual mesh resources:
					std::vector<Reference<Resource>> resources;
					for (size_t i = 0; i < m_nodes.size(); i++) {
						const Node& node = m_nodes[i];
						for (size_t j = 0; j < node.meshes.Size(); j++)
							resources.push_back(node.meshes[j].mesh->LoadResource());
					}

					// Create spowner;
					Reference<Spowner> spowner = new Spowner(this, std::move(resources));
					spowner->ReleaseRef();
					return spowner;
				}
			};



			class FBXImporterSerializer;


			class FBXImporter : public virtual FileSystemDatabase::AssetImporter {
			public:
				inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
					// __TODO__: If last write time matches the one from the previous import, probably no need to rescan the FBX (will make startup times faster)...
					Reference<FBXData> data = FBXData::Extract(AssetFilePath(), Log());
					if (data == nullptr) return false;
					size_t revision = m_revision.fetch_add(1);

					FBXUidToGUID polyMeshGUIDs;
					FBXUidToGUID triMeshGUIDs;
					FBXUidToGUID collisionMeshGUIDs;
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

					std::unordered_map<FBXUid, Reference<Asset>> triMeshAssets;
					for (size_t i = 0; i < data->MeshCount(); i++) {
						const FBXMesh* mesh = data->GetMesh(i);
						const FBXUid uid = mesh->uid;
						const Reference<Asset> polyMeshAsset = [&]() -> Reference<Asset> {
							if (dynamic_cast<const SkinnedPolyMesh*>(mesh->mesh.operator->()) != nullptr)
								return Object::Instantiate<FBXSkinnedMeshAsset>(getGuidOf(uid, m_polyMeshGUIDs, polyMeshGUIDs), this, revision, uid);
							else return Object::Instantiate<FBXMeshAsset>(getGuidOf(uid, m_polyMeshGUIDs, polyMeshGUIDs), this, revision, uid);
						}();
						const Reference<Asset> triMeshAsset = [&]() -> Reference<Asset> {
							if (dynamic_cast<const SkinnedPolyMesh*>(mesh->mesh.operator->()) != nullptr)
								return Object::Instantiate<FBXSkinnedTriMeshAsset>(
									getGuidOf(uid, m_triMeshGUIDs, triMeshGUIDs),
									getGuidOf(uid, m_collisionMeshGUIDs, collisionMeshGUIDs),
									dynamic_cast<FBXSkinnedMeshAsset*>(polyMeshAsset.operator->()));
							else return Object::Instantiate<FBXTriMeshAsset>(
								getGuidOf(uid, m_triMeshGUIDs, triMeshGUIDs),
								getGuidOf(uid, m_collisionMeshGUIDs, collisionMeshGUIDs),
								dynamic_cast<FBXMeshAsset*>(polyMeshAsset.operator->()));
						}();
						AssetInfo info;
						info.resourceName = PolyMesh::Reader(mesh->mesh).Name();
						{
							info.asset = polyMeshAsset;
							reportAsset(info);
						}
						{
							info.asset = triMeshAsset;
							reportAsset(info);
							triMeshAssets[mesh->uid] = triMeshAsset;
						}
						{
							Reference<Physics::CollisionMesh::MeshAsset> asset = triMeshAsset;
							if (asset != nullptr) {
								info.asset = Physics::CollisionMesh::GetAsset(asset, PhysicsInstance());
								reportAsset(info);
							}
						}
					}

					for (size_t i = 0; i < data->AnimationCount(); i++) {
						const FBXAnimation* animation = data->GetAnimation(i);
						const FBXUid uid = animation->uid;
						const Reference<FBXAnimationAsset> animationAsset = Object::Instantiate<FBXAnimationAsset>(
							getGuidOf(uid, m_animationGUIDs, animationGUIDs), this, revision, uid);
						{
							AssetInfo info;
							info.asset = animationAsset;
							info.resourceName = animation->clip->Name();
							reportAsset(info);
						}
					}

					{
						Asset* (*findTriMeshAsset)(const std::unordered_map<FBXUid, Reference<Asset>>*, FBXUid) =
							[](const std::unordered_map<FBXUid, Reference<Asset>>* assets, FBXUid uid) -> Asset* {
							std::unordered_map<FBXUid, Reference<Asset>>::const_iterator it = assets->find(uid);
							if (it == assets->end()) return nullptr;
							else return it->second;
						};
						Reference<FBXHeirarchyAsset> heirarchy = Object::Instantiate<FBXHeirarchyAsset>(
							m_heirarchyId, this, data, Function<Asset*, FBXUid>(findTriMeshAsset, &triMeshAssets));
						AssetInfo info;
						info.asset = heirarchy;
						info.resourceName = OS::Path(AssetFilePath().stem());
						reportAsset(info);
					}

					m_polyMeshGUIDs = std::move(polyMeshGUIDs);
					m_triMeshGUIDs = std::move(triMeshGUIDs);
					m_collisionMeshGUIDs = std::move(collisionMeshGUIDs);
					m_animationGUIDs = std::move(animationGUIDs);

					return true;
				}

			private:
				std::atomic<size_t> m_revision = 0;

				typedef std::map<FBXUid, GUID> FBXUidToGUID;
				GUID m_heirarchyId = GUID::Generate();
				FBXUidToGUID m_polyMeshGUIDs;
				FBXUidToGUID m_triMeshGUIDs;
				FBXUidToGUID m_collisionMeshGUIDs;
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
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("Heirarchy", "FBX Scene");
						recordElement(serializer->Serialize(importer->m_heirarchyId));
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
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> triMeshGUIDSerializer = Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Collision meshes");
						recordElement(triMeshGUIDSerializer->Serialize(importer->m_collisionMeshGUIDs));
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
