#include "FBXAssetImporter.h"
#include "FBXData.h"
#include "../../Serialization/Helpers/SerializerMacros.h"
#include "../../ComponentHierarchySpowner.h"
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
					inline static Reference<FBXDataCache> For(
						const PathAndRevision& pathAndRevision, OS::Logger* logger, const Callback<FBXData*>& onLoaded) {
						static Cache cache;
						return cache.GetCachedOrCreate(pathAndRevision, [&]() -> Reference<FBXDataCache> {
							Reference<FBXData> data = FBXData::Extract(pathAndRevision.path, logger);
							onLoaded(data);
							if (data == nullptr) 
								return nullptr;
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
					m_dataCache = FBXDataCache::Cache::For(
						PathAndRevision{ m_importer->AssetFilePath(), m_revision }, m_importer->Log(), Callback<FBXData*>(Unused<FBXData*>));
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

			class FBXHierarchyAsset : public virtual Asset::Of<ComponentHierarchySpowner> {
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
				const std::unordered_map<FBXUid, Reference<Asset>> m_triMeshAssets;
				const size_t m_revision;
				volatile bool m_nodesInitialized = false;
				std::vector<Node> m_nodes;

				class Spowner : public virtual ComponentHierarchySpowner {
				private:
					const Reference<FBXHierarchyAsset> m_asset;
					const std::vector<Reference<Resource>> m_resources;

				public:
					inline Spowner(FBXHierarchyAsset* asset, std::vector<Reference<Resource>>&& resources) : m_asset(asset), m_resources(std::move(resources)) {}

					inline virtual Reference<Component> SpownHierarchy(Component* parent) final override {
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
									parent->Context()->Log()->Error("FBXHierarchyAsset::Spowner::SpownHierarchy - Failed to load the mesh!");
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
								parent->Context()->Log()->Error("FBXHierarchyAsset::Spowner::SpownHierarchy - Internal error: Not enough transforms!");
								break;
							}
							Transform* transform = transforms[nodeId];
							for (size_t meshId = 0; meshId < node.meshes.Size(); meshId++) {
								if (meshId >= transform->ChildCount()) {
									parent->Context()->Log()->Error("FBXHierarchyAsset::Spowner::SpownHierarchy - Internal error: Not enough renderers!");
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

				inline void InitializeNodes(FBXData* data) {
					if (data == nullptr)
						return;
					assert(m_nodes.size() <= 0u);
					std::vector<const FBXNode*> sourceNodes;
					AppendNode(m_nodes, sourceNodes, data->RootNode(), 0);
					typedef Asset* (*FindTriMeshAssetFn)(const std::unordered_map<FBXUid, Reference<Asset>>*, FBXUid);
					static const FindTriMeshAssetFn findTriMeshAsset = [](const std::unordered_map<FBXUid, Reference<Asset>>* assets, FBXUid uid) -> Asset* {
						std::unordered_map<FBXUid, Reference<Asset>>::const_iterator it = assets->find(uid);
						if (it == assets->end()) return nullptr;
						else return it->second;
						};
					AddMeshes(m_nodes, sourceNodes, Function<Asset*, FBXUid>(findTriMeshAsset, &m_triMeshAssets));
					m_nodesInitialized = true;
				}

			public:
				inline FBXHierarchyAsset(
					const GUID& guid, const FileSystemDatabase::AssetImporter* importer, size_t revision,
					FBXData* data, const std::unordered_map<FBXUid, Reference<Asset>>& triMeshAssets)
					: Asset(guid)
					, m_importer(importer)
					, m_triMeshAssets(triMeshAssets)
					, m_revision(revision) {
					InitializeNodes(data);
				}

			protected:
				inline virtual Reference<ComponentHierarchySpowner> LoadItem() final override {
					// Load fbx if needed:
					Reference<FBXDataCache> dataCache;
					if (!m_nodesInitialized) {
						Reference<FBXData> data;
						bool dataLoadAttempted = false;
						auto onLoaded = [&](FBXData* d) {
							data = d;
							dataLoadAttempted = true;
						};
						dataCache = FBXDataCache::Cache::For(
							PathAndRevision{ m_importer->AssetFilePath(), m_revision }, m_importer->Log(), Callback<FBXData*>::FromCall(&onLoaded));
						if (!dataLoadAttempted) 
							data = FBXData::Extract(m_importer->AssetFilePath(), m_importer->Log());
						if (data == nullptr) {
							m_importer->Log()->Error("FBXHierarchyAsset::LoadItem - Failed to load FBX file '(", m_importer->AssetFilePath(), ")'!");
							return nullptr;
						}
						InitializeNodes(data);
					}

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
					static const std::string alreadyLoadedState = "Imported";
					const size_t revision = m_revision.fetch_add(1);
					if (PreviousImportData() != alreadyLoadedState) {
						Reference<FBXData> data = FBXData::Extract(AssetFilePath(), Log());
						if (data == nullptr)
							return false;
						else PreviousImportData() = alreadyLoadedState;

						FBXUidToPolyMeshInfo polyMeshGUIDs;
						FBXUidToGUID triMeshGUIDs;
						FBXUidToGUID collisionMeshGUIDs;
						FBXUidToAnimationInfo animationGUIDs;

						auto getGuidOf = [](FBXUid uid, const auto& cache, auto& resultCache, auto value) {
							auto it = cache.find(uid);
							value = [&]() -> GUID {
								if (it != cache.end())
									return it->second;
								else return GUID::Generate();
							}();
							resultCache[uid] = value;
							return value;
						};

						for (size_t i = 0; i < data->MeshCount(); i++) {
							const FBXMesh* mesh = data->GetMesh(i);
							const FBXUid uid = mesh->uid;
							const PolyMesh::Reader reader(mesh->mesh);
							getGuidOf(uid, m_polyMeshGUIDs, polyMeshGUIDs, PolyMeshInfo(
								GUID::Generate(), reader.Name(),
								(dynamic_cast<const SkinnedPolyMesh*>(mesh->mesh.operator->()) != nullptr)));
							getGuidOf(uid, m_triMeshGUIDs, triMeshGUIDs, GUID::Generate());
							getGuidOf(uid, m_collisionMeshGUIDs, collisionMeshGUIDs, GUID::Generate());
						}

						for (size_t i = 0; i < data->AnimationCount(); i++) {
							const FBXAnimation* animation = data->GetAnimation(i);
							const FBXUid uid = animation->uid;
							getGuidOf(uid, m_animationGUIDs, animationGUIDs, AnimationInfo(GUID::Generate(), animation->clip->Name()));
						}

						m_polyMeshGUIDs = std::move(polyMeshGUIDs);
						m_triMeshGUIDs = std::move(triMeshGUIDs);
						m_collisionMeshGUIDs = std::move(collisionMeshGUIDs);
						m_animationGUIDs = std::move(animationGUIDs);
					}

					// Validate mesh/collision mesh assets:
					{
						bool invalidated = false;
						for (auto polyIt = m_polyMeshGUIDs.begin(); polyIt != m_polyMeshGUIDs.end(); ++polyIt) {
							auto triIt = m_triMeshGUIDs.find(polyIt->first);
							auto collisionIt = m_collisionMeshGUIDs.find(polyIt->first);
							if (triIt == m_triMeshGUIDs.end() || collisionIt == m_collisionMeshGUIDs.end()) {
								invalidated = true;
								break;
							}
						}
						if (invalidated) {
							PreviousImportData() = "";
							return Import(reportAsset);
						}
					}

					// Report mesh/collision mesh assets:
					std::unordered_map<FBXUid, Reference<Asset>> triMeshAssets;
					for (auto polyIt = m_polyMeshGUIDs.begin(); polyIt != m_polyMeshGUIDs.end(); ++polyIt) {
						auto triIt = m_triMeshGUIDs.find(polyIt->first);
						auto collisionIt = m_collisionMeshGUIDs.find(polyIt->first);
						const Reference<Asset> polyMeshAsset = [&]() -> Reference<Asset> {
							if (polyIt->second.isSkinnedMesh)
								return Object::Instantiate<FBXSkinnedMeshAsset>(polyIt->second.guid, this, revision, polyIt->first);
							else return Object::Instantiate<FBXMeshAsset>(polyIt->second.guid, this, revision, polyIt->first);
						}();
						const Reference<Asset> triMeshAsset = [&]() -> Reference<Asset> {
							if (polyIt->second.isSkinnedMesh)
								return Object::Instantiate<FBXSkinnedTriMeshAsset>(
									triIt->second, collisionIt->second, dynamic_cast<FBXSkinnedMeshAsset*>(polyMeshAsset.operator->()));
							else return Object::Instantiate<FBXTriMeshAsset>(
								triIt->second, collisionIt->second, dynamic_cast<FBXMeshAsset*>(polyMeshAsset.operator->()));
						}();
						AssetInfo info;
						info.resourceName = polyIt->second.name;
						{
							info.asset = polyMeshAsset;
							reportAsset(info);
						}
						{
							info.asset = triMeshAsset;
							reportAsset(info);
							triMeshAssets[polyIt->first] = triMeshAsset;
						}
						{
							Reference<Physics::CollisionMesh::MeshAsset> asset = triMeshAsset;
							if (asset != nullptr) {
								info.asset = Physics::CollisionMesh::GetAsset(asset, PhysicsInstance());
								reportAsset(info);
							}
						}
					}

					// Report Animation assets:
					for (auto it = m_animationGUIDs.begin(); it != m_animationGUIDs.end(); ++it) {
						const Reference<FBXAnimationAsset> animationAsset = 
							Object::Instantiate<FBXAnimationAsset>(it->second.guid, this, revision, it->first);
						{
							AssetInfo info;
							info.asset = animationAsset;
							info.resourceName = it->second.name;
							reportAsset(info);
						}
					}


					// Report 
					{
						Reference<FBXHierarchyAsset> Hierarchy = 
							Object::Instantiate<FBXHierarchyAsset>(m_HierarchyId, this, revision, nullptr, triMeshAssets);
						AssetInfo info;
						info.asset = Hierarchy;
						info.resourceName = OS::Path(AssetFilePath().stem());
						reportAsset(info);
					}

					return true;
				}

			private:
				std::atomic<size_t> m_revision = 0;

				struct PolyMeshInfo {
					GUID guid = {};
					std::string name;
					bool isSkinnedMesh = false;
					inline PolyMeshInfo(const GUID& g = {}, const std::string_view& n = "", bool s = false) 
						: guid(g), name(n), isSkinnedMesh(s) {}
					inline PolyMeshInfo& operator=(const GUID& g) { guid = g; return *this; }
					inline operator const GUID& ()const { return guid; }
					inline bool operator!=(const PolyMeshInfo& other)const { return guid != other.guid || name != other.name || isSkinnedMesh != other.isSkinnedMesh; }
				};
				struct AnimationInfo {
					GUID guid = {};
					std::string name;
					inline AnimationInfo(const GUID& g = {}, const std::string_view& n = "") : guid(g), name(n) {}
					inline AnimationInfo& operator=(const GUID& g) { guid = g; return *this; }
					inline operator const GUID& ()const { return guid; }
					inline bool operator!=(const AnimationInfo& other)const { return guid != other.guid || name != other.name; }
				};
				using FBXUidToPolyMeshInfo = std::map<FBXUid, PolyMeshInfo>;
				using FBXUidToGUID = std::map<FBXUid, GUID>;
				using FBXUidToAnimationInfo = std::map<FBXUid, AnimationInfo>;
				GUID m_HierarchyId = GUID::Generate();
				FBXUidToPolyMeshInfo m_polyMeshGUIDs;
				FBXUidToGUID m_triMeshGUIDs;
				FBXUidToGUID m_collisionMeshGUIDs;
				FBXUidToAnimationInfo m_animationGUIDs;

				friend class FBXImporterSerializer;

				using GuidMapping = std::pair<FBXUid, GUID>;
				class GuidMappingSerializer : public virtual Serialization::SerializerList::From<GuidMapping> {
				private:
					inline GuidMappingSerializer() : Serialization::ItemSerializer("Mapping") {}

				public:
					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, GuidMapping* target)const final override {
						JIMARA_SERIALIZE_FIELDS(target, recordElement) {
							JIMARA_SERIALIZE_FIELD(target->first, "FBXUid", "FBX Id");
							JIMARA_SERIALIZE_FIELD(target->second, "GUID", "Asset Id");
						};
					}

					inline static const GuidMappingSerializer& Instance() {
						static const GuidMappingSerializer serializer;
						return serializer;
					}
				};

				using PolyMeshInfoMapping = std::pair<FBXUid, PolyMeshInfo>;
				class PolyMeshInfoMappingSerializer : public virtual Serialization::SerializerList::From<PolyMeshInfoMapping> {
				private:
					inline PolyMeshInfoMappingSerializer() : Serialization::ItemSerializer("Mapping") {}

				public:
					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, PolyMeshInfoMapping* target)const final override {
						JIMARA_SERIALIZE_FIELDS(target, recordElement) {
							JIMARA_SERIALIZE_FIELD(target->first, "FBXUid", "FBX Id");
							JIMARA_SERIALIZE_FIELD(target->second.guid, "GUID", "Asset Id");
							JIMARA_SERIALIZE_FIELD(target->second.name, "Name", "Asset Name");
							JIMARA_SERIALIZE_FIELD(target->second.isSkinnedMesh, "IsSkinned", "True, if the mesh is skinned");
						};
					}

					inline static const PolyMeshInfoMappingSerializer& Instance() {
						static const PolyMeshInfoMappingSerializer serializer;
						return serializer;
					}
				};

				using AnimationInfoMapping = std::pair<FBXUid, AnimationInfo>;
				class AnimationInfoMappingSerializer : public virtual Serialization::SerializerList::From<AnimationInfoMapping> {
				private:
					inline AnimationInfoMappingSerializer() : Serialization::ItemSerializer("Mapping") {}

				public:
					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AnimationInfoMapping* target)const final override {
						JIMARA_SERIALIZE_FIELDS(target, recordElement) {
							JIMARA_SERIALIZE_FIELD(target->first, "FBXUid", "FBX Id");
							JIMARA_SERIALIZE_FIELD(target->second.guid, "GUID", "Asset Id");
							JIMARA_SERIALIZE_FIELD(target->second.name, "Name", "Asset Name");
						};
					}

					inline static const AnimationInfoMappingSerializer& Instance() {
						static const AnimationInfoMappingSerializer serializer;
						return serializer;
					}
				};

				template<typename MapType, typename MappingType, typename ElementSerializer>
				class FBXUidMappingSerializer : public virtual Serialization::SerializerList::From<MapType> {
				public:
					inline FBXUidMappingSerializer(const std::string_view& name) : Serialization::ItemSerializer(name) {}

					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, MapType* target)const final override {
						std::vector<MappingType> mappings(target->begin(), target->end());
						{
							static const auto countSerializer = Serialization::ValueSerializer<int64_t>::For<std::vector<MappingType>>(
								"Count", "Number of entries",
								[](std::vector<MappingType>* mappings) -> int64_t { return static_cast<int64_t>(mappings->size()); },
								[](const int64_t& size, std::vector<MappingType>* mappings) { mappings->resize(static_cast<size_t>(size)); });
							recordElement(countSerializer->Serialize(mappings));
						}
						bool dirty = (mappings.size() != target->size());
						for (size_t i = 0; i < mappings.size(); i++) {
							MappingType& mapping = mappings[i];
							MappingType oldMapping = mapping;
							recordElement(ElementSerializer::Instance().Serialize(mapping));
							if (oldMapping.first != mapping.first || oldMapping.second != mapping.second)
								dirty = true;
						}
						if (dirty) {
							target->clear();
							for (size_t i = 0; i < mappings.size(); i++) {
								MappingType& mapping = mappings[i];
								target->operator[](mapping.first) = mapping.second;
							};
						}
					}
				};

				using FBXUidToPolyMeshInfoSerializer = FBXUidMappingSerializer<FBXUidToPolyMeshInfo, PolyMeshInfoMapping, PolyMeshInfoMappingSerializer>;
				using FBXUidToGUIDSerializer = FBXUidMappingSerializer<FBXUidToGUID, GuidMapping, GuidMappingSerializer>;
				using FBXUidToAnimationInfoSerializer = FBXUidMappingSerializer<FBXUidToAnimationInfo, AnimationInfoMapping, AnimationInfoMappingSerializer>;
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
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("Hierarchy", "FBX Scene");
						recordElement(serializer->Serialize(importer->m_HierarchyId));
					}
					{
						static const Reference<FBXImporter::FBXUidToPolyMeshInfoSerializer> polyMeshGUIDSerializer =
							Object::Instantiate<FBXImporter::FBXUidToPolyMeshInfoSerializer>("Polygonal meshes");
						recordElement(polyMeshGUIDSerializer->Serialize(importer->m_polyMeshGUIDs));
					}
					{
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> triMeshGUIDSerializer = 
							Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Triangle meshes");
						recordElement(triMeshGUIDSerializer->Serialize(importer->m_triMeshGUIDs));
					}
					{
						static const Reference<FBXImporter::FBXUidToGUIDSerializer> triMeshGUIDSerializer = 
							Object::Instantiate<FBXImporter::FBXUidToGUIDSerializer>("Collision meshes");
						recordElement(triMeshGUIDSerializer->Serialize(importer->m_collisionMeshGUIDs));
					}
					{
						static const Reference<FBXImporter::FBXUidToAnimationInfoSerializer> animationGUIDSerializer = 
							Object::Instantiate<FBXImporter::FBXUidToAnimationInfoSerializer>("Animations");
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
