#include "WavefrontOBJ.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../ComponentHeirarchySpowner.h"
#include "../../Components/GraphicsObjects/MeshRenderer.h"
#include "../../Math/Helpers.h"
#include <fstream>
#include <stdio.h>
#include <map>

#pragma warning(disable: 26812)
#pragma warning(disable: 26495)
#pragma warning(disable: 26451)
#pragma warning(disable: 26498)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#pragma warning(default: 26812)
#pragma warning(default: 26495)
#pragma warning(default: 26451)
#pragma warning(default: 26498)

namespace Jimara {
	namespace {
		inline static bool LoadObjData(const OS::Path& filename, OS::Logger* logger, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes) {
			std::vector<tinyobj::material_t> materials;
			std::string warning, error;

			std::ifstream stream(filename);
			if (!stream) {
				if (logger != nullptr) logger->Error("LoadObjData failed: Could not open file: '", filename, "'");
				return false;
			}

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, &stream, nullptr, false)) {
				if (logger != nullptr) logger->Error("LoadObjData failed: " + error);
				return false;
			}

			if (warning.length() > 0 && logger != nullptr)
				logger->Warning("LoadObjData: " + warning);

			return true;
		}

		struct OBJVertex {
			uint32_t vertexId;
			uint32_t normalId;
			uint32_t uvId;

			inline bool operator<(const OBJVertex& other)const {
				return vertexId < other.vertexId || (vertexId == other.vertexId && (normalId < other.normalId || (normalId == other.normalId && uvId < other.uvId)));
			}
		};

		template<typename MeshType>
		inline static uint32_t GetVertexId(const tinyobj::index_t& index, const tinyobj::attrib_t& attrib, std::map<OBJVertex, uint32_t>& vertexIndexCache, typename MeshType::Writer& mesh) {
			OBJVertex vert = {};
			vert.vertexId = static_cast<uint32_t>(index.vertex_index);
			vert.normalId = static_cast<uint32_t>(index.normal_index);
			vert.uvId = static_cast<uint32_t>(index.texcoord_index);

			std::map<OBJVertex, uint32_t>::const_iterator it = vertexIndexCache.find(vert);
			if (it != vertexIndexCache.end()) return it->second;

			uint32_t vertId = static_cast<uint32_t>(mesh.VertCount());

			MeshVertex vertex = {};

			size_t baseVertex = static_cast<size_t>(3u) * vert.vertexId;
			vertex.position = Vector3(
				static_cast<float>(attrib.vertices[baseVertex]),
				static_cast<float>(attrib.vertices[baseVertex + 1]),
				-static_cast<float>(attrib.vertices[baseVertex + 2]));

			size_t baseNormal = static_cast<size_t>(3u) * vert.normalId;
			vertex.normal = Vector3(
				static_cast<float>(attrib.normals[baseNormal]),
				static_cast<float>(attrib.normals[baseNormal + 1]),
				-static_cast<float>(attrib.normals[baseNormal + 2]));

			size_t baseUV = static_cast<size_t>(2u) * vert.uvId;
			vertex.uv = Vector2(
				static_cast<float>(attrib.texcoords[baseUV]),
				1.0f - static_cast<float>(attrib.texcoords[baseUV + 1]));

			mesh.AddVert(vertex);

			vertexIndexCache.insert(std::make_pair(vert, vertId));
			return vertId;
		}

		template<typename MeshType>
		struct MeshFaceExtractor {};

		template<>
		struct MeshFaceExtractor<TriMesh> {
			inline static void Extract(
				const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape, size_t indexStart, size_t indexEnd,
				std::map<OBJVertex, uint32_t>& vertexIndexCache, TriMesh::Writer& writer) {
				if ((indexEnd - indexStart) <= 2) return;
				TriangleFace face = {};
				face.a = GetVertexId<TriMesh>(shape.mesh.indices[indexStart], attrib, vertexIndexCache, writer);
				face.c = GetVertexId<TriMesh>(shape.mesh.indices[indexStart + 1], attrib, vertexIndexCache, writer);
				for (size_t i = indexStart + 2; i < indexEnd; i++) {
					face.b = face.c;
					face.c = GetVertexId<TriMesh>(shape.mesh.indices[i], attrib, vertexIndexCache, writer);
					writer.AddFace(face);
				}
			}
		};

		template<>
		struct MeshFaceExtractor<PolyMesh> {
			inline static void Extract(
				const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape, size_t indexStart, size_t indexEnd,
				std::map<OBJVertex, uint32_t>& vertexIndexCache, PolyMesh::Writer& writer) {
				writer.AddFace(PolygonFace());
				PolygonFace& face = writer.Face(writer.FaceCount() - 1);
				for (size_t i = indexStart; i < indexEnd; i++)
					face.Push(GetVertexId<PolyMesh>(shape.mesh.indices[i], attrib, vertexIndexCache, writer));
			}
		};

		template<typename MeshType>
		inline static Reference<MeshType> ExtractMesh(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape) {
			Reference<MeshType> mesh = Object::Instantiate<MeshType>(shape.name);
			typename MeshType::Writer writer(mesh);
			std::map<OBJVertex, uint32_t> vertexIndexCache;

			size_t indexStart = 0;
			for (size_t faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++) {
				size_t indexEnd = indexStart + shape.mesh.num_face_vertices[faceId];
				MeshFaceExtractor<MeshType>::Extract(attrib, shape, indexStart, indexEnd, vertexIndexCache, writer);
				indexStart = indexEnd;
			}
			return mesh;
		}

		template<typename MeshType>
		inline static std::vector<Reference<MeshType>> LoadMeshesFromOBJ(const OS::Path& filename, OS::Logger* logger) {
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			if (!LoadObjData(filename, logger, attrib, shapes)) return std::vector<Reference<MeshType>>();
			std::vector<Reference<MeshType>> meshes;
			for (size_t i = 0; i < shapes.size(); i++)
				meshes.push_back(ExtractMesh<MeshType>(attrib, shapes[i]));
			return meshes;
		}

		template<typename MeshType>
		inline static Reference<MeshType> LoadMeshFromOBJ(const OS::Path& filename, const std::string_view& objectName, OS::Logger* logger) {
			tinyobj::attrib_t attrib;
			std::vector<tinyobj::shape_t> shapes;
			if (!LoadObjData(filename, logger, attrib, shapes)) return nullptr;
			for (size_t i = 0; i < shapes.size(); i++) {
				const tinyobj::shape_t& shape = shapes[i];
				if (shape.name == objectName)
					return ExtractMesh<MeshType>(attrib, shape);
			}
			if (logger != nullptr)
				logger->Error("LoadMeshFromObj - '", objectName, "' could not be found in '", filename, "'");
			return nullptr;
		}
	}

	std::vector<Reference<TriMesh>> TriMeshesFromOBJ(const OS::Path& filename, OS::Logger* logger) {
		return LoadMeshesFromOBJ<TriMesh>(filename, logger);
	}

	Reference<TriMesh> TriMeshFromOBJ(const OS::Path& filename, const std::string_view& objectName, OS::Logger* logger) {
		return LoadMeshFromOBJ<TriMesh>(filename, objectName, logger);
	}

	std::vector<Reference<PolyMesh>> PolyMeshesFromOBJ(const OS::Path& filename, OS::Logger* logger) {
		return LoadMeshesFromOBJ<PolyMesh>(filename, logger);
	}

	Reference<PolyMesh> PolyMeshFromOBJ(const OS::Path& filename, const std::string_view& objectName, OS::Logger* logger) {
		return LoadMeshFromOBJ<PolyMesh>(filename, objectName, logger);
	}
}


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
	namespace {
		struct OBJAssetDataCache : public virtual ObjectCache<PathAndRevision>::StoredObject {
			std::vector<Reference<PolyMesh>> meshes;
			
			class Cache : public virtual ObjectCache<PathAndRevision> {
			public:
				inline static Reference<OBJAssetDataCache> For(const PathAndRevision& pathAndRevision, OS::Logger* logger) {
					static Cache cache;
					return cache.GetCachedOrCreate(pathAndRevision, false, [&]() -> Reference<OBJAssetDataCache> {
						Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(pathAndRevision.path);
						if (mapping == nullptr) {
							if (logger != nullptr) logger->Error("Could not open file: '", pathAndRevision.path, "'!");
							return nullptr;
						}
						Reference<OBJAssetDataCache> cache = Object::Instantiate<OBJAssetDataCache>();
						cache->meshes = PolyMeshesFromOBJ(pathAndRevision.path, logger);
						return cache;
						});
				}
			};
		};

		class OBJPolyMeshAsset : public virtual Asset::Of<PolyMesh> {
		private:
			const Reference<FileSystemDatabase::AssetImporter> m_importer;
			const size_t m_revision;
			const size_t m_meshIndex;

			Reference<OBJAssetDataCache> m_cache;

		public:
			inline OBJPolyMeshAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, size_t revision, size_t meshIndex) 
				: Asset(guid), m_importer(importer), m_revision(revision), m_meshIndex(meshIndex) {}

			virtual Reference<PolyMesh> LoadItem() final override {
				Reference<OS::Logger> logger = m_importer->Log();
				if (m_cache != nullptr) {
					if (logger != nullptr) 
						logger->Error("OBJPolyMeshAsset::LoadResource - Resource already loaded! <internal error>");
					return nullptr;
				}
				const OS::Path path = m_importer->AssetFilePath();
				m_cache = OBJAssetDataCache::Cache::For({ path, m_revision }, logger);
				if (m_cache == nullptr) return nullptr;
				else if (m_cache->meshes.size() <= m_meshIndex) {
					if (logger != nullptr)
						logger->Error("OBJPolyMeshAsset::LoadResource - Invalid mesh index! File: '", path, "'");
					m_cache = nullptr;
					return nullptr;
				}
				else {
					Reference<PolyMesh>& reference = m_cache->meshes[m_meshIndex];
					const Reference<Resource> result = reference;
					reference = nullptr;
					return result;
				}
			}

			inline virtual void UnloadItem(PolyMesh* resource) final override {
				Reference<OS::Logger> logger = m_importer->Log();
				if (m_cache == nullptr) {
					if (logger != nullptr)
						logger->Error("OBJPolyMeshAsset::UnloadResource - Resource was not loaded! <internal error>");
				}
				else if (m_cache->meshes.size() <= m_meshIndex) {
					if (logger != nullptr)
						logger->Error("OBJPolyMeshAsset::UnloadResource - Resource index out of bounds! <internal error>");
				}
				else {
					Reference<PolyMesh>& reference = m_cache->meshes[m_meshIndex];
					if (reference != nullptr) {
						if (logger != nullptr)
							logger->Error("OBJPolyMeshAsset::UnloadResource - Possible circular dependencies detected! <internal error>");
					}
					else reference = resource;
				}
				m_cache = nullptr;
			}
		};

#pragma warning(disable: 4250)
		class OBJTriMeshAsset : public virtual Physics::CollisionMesh::MeshAsset::Of<TriMesh> {
		private:
			const Reference<OBJPolyMeshAsset> m_meshAsset;

			Reference<const PolyMesh> m_sourceMesh;

		public:
			inline OBJTriMeshAsset(const GUID& guid, const GUID& collisionMeshId, OBJPolyMeshAsset* meshAsset)
				: Asset(guid), Physics::CollisionMesh::MeshAsset::Of<TriMesh>(collisionMeshId), m_meshAsset(meshAsset) {
				assert(m_meshAsset != nullptr);
			}

		protected:
			virtual Reference<TriMesh> LoadItem() final override {
				m_sourceMesh = m_meshAsset->LoadAs<PolyMesh>();
				if (m_sourceMesh != nullptr) return ToTriMesh(m_sourceMesh);
				else return nullptr;
			}

			inline virtual void UnloadItem(TriMesh* resource) final override {
				m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
			}
		};
#pragma warning(default: 4250)

		class OBJHeirarchyAsset : public virtual Asset::Of<ComponentHeirarchySpowner> {
		private:
			const Reference<FileSystemDatabase::AssetImporter> m_importer;
			const std::vector<Reference<OBJTriMeshAsset>> m_assets;

			class Spowner : public virtual ComponentHeirarchySpowner {
			private:
				const std::vector<Reference<TriMesh>> m_meshes;
				const std::string m_name;

			public:
				inline Spowner(std::vector<Reference<TriMesh>>&& meshes, std::string&& name) : m_meshes(std::move(meshes)), m_name(std::move(name)) {}

				inline virtual Reference<Component> SpownHeirarchy(Component* parent) final override {
					if (parent == nullptr)
						return nullptr;
					std::unique_lock<std::recursive_mutex> lock(parent->Context()->UpdateLock());
					Reference<Transform> transform = Object::Instantiate<Transform>(parent, m_name);
					for (size_t i = 0; i < m_meshes.size(); i++)
						Object::Instantiate<MeshRenderer>(transform, TriMesh::Reader(m_meshes[i]).Name(), m_meshes[i]);
					return transform;
				}
			};

		public:
			inline OBJHeirarchyAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, std::vector<Reference<OBJTriMeshAsset>>&& assets)
				: Asset(guid), m_importer(importer), m_assets(std::move(assets)) {}

		protected:
			inline virtual Reference<ComponentHeirarchySpowner> LoadItem()final override {
				// Path and name:
				const OS::Path path = m_importer->AssetFilePath();
				std::string name = OS::Path(path.stem());

				// Preload meshes:
				std::vector<Reference<TriMesh>> meshes;
				for (size_t i = 0; i < m_assets.size(); i++) {
					Reference<TriMesh> mesh = m_assets[i]->Load();
					if (mesh != nullptr) meshes.push_back(mesh);
					else {
						m_importer->Log()->Error("OBJHeirarchyAsset::LoadItem - Failed to load object ", i, " from \"", path, "\"!");
						return nullptr;
					}
				}

				// Create spowner;
				Reference<Spowner> spowner = new Spowner(std::move(meshes), std::move(name));
				spowner->ReleaseRef();
				return spowner;
			}
		};

		class OBJAssetImporterSerializer;

		class OBJAssetImporter : public virtual FileSystemDatabase::AssetImporter {
		private:
			std::atomic<size_t> m_revision = 0;
			struct MeshIds {
				GUID polyMesh = GUID::Generate();
				GUID triMesh = GUID::Generate();
				GUID collisionMesh = GUID::Generate();
				size_t index = 0u;
			};
			typedef std::pair<std::string, MeshIds> NameToGuids;
			typedef std::map<std::string, MeshIds> NameToGUID;
			GUID m_heirarchyId = GUID::Generate();
			NameToGUID m_nameToGUID;

			class NameToGUIDSerializer : public virtual Serialization::SerializerList::From<NameToGuids> {
			private:
				inline NameToGUIDSerializer() : ItemSerializer("Mesh") {}

			public:
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, NameToGuids* target)const final override {
					{
						static const Reference<const Serialization::ItemSerializer::Of<std::string>> serializer = Serialization::ValueSerializer<std::string_view>::For<std::string>(
							"Name", "Index-decorated name of the mesh",
							[](std::string* name) -> std::string_view { return *name; },
							[](const std::string_view& newName, std::string* name) { (*name) = newName; });
						recordElement(serializer->Serialize(target->first));
					}
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("PolyMesh");
						recordElement(serializer->Serialize(target->second.polyMesh));
					}
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("TriMesh");
						recordElement(serializer->Serialize(target->second.triMesh));
					}
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("CollisionMesh");
						recordElement(serializer->Serialize(target->second.collisionMesh));
					}
					{
						static const Reference<const Serialization::ItemSerializer::Of<size_t>> serializer = Serialization::ValueSerializer<size_t>::Create("Mesh Index");
						recordElement(serializer->Serialize(target->second.index));
					}
				}

				inline static const NameToGUIDSerializer& Instance() {
					static const NameToGUIDSerializer serializer;
					return serializer;
				}
			};

			friend class OBJAssetImporterSerializer;

		public:
			inline virtual bool Import(Callback<const AssetInfo&> reportAsset) final override {
				static const std::string alreadyLoadedState = "Imported";
				size_t revision;
				if (PreviousImportData() != alreadyLoadedState) {
					revision = m_revision.fetch_add(1);
					Reference<OBJAssetDataCache> cache = OBJAssetDataCache::Cache::For({ AssetFilePath(), revision }, Log());
					if (cache == nullptr)
						return false;
					else PreviousImportData() = alreadyLoadedState;

					NameToGUID nameToGuid;
					std::unordered_map<std::string_view, size_t> nameCounts;

					auto getName = [&](const PolyMesh& mesh) -> std::string {
						const std::string& name = PolyMesh::Reader(mesh).Name();
						std::unordered_map<std::string_view, size_t>::iterator it = nameCounts.find(name);
						size_t count;
						if (it == nameCounts.end()) {
							count = 0;
							nameCounts[name] = 1;
						}
						else {
							count = it->second;
							it->second++;
						}
						std::stringstream stream;
						stream << name << '_' << count;
						return stream.str();
					};

					auto getGuid = [&](const std::string& name, NameToGUID& nameToGuid, const NameToGUID& oldNamesToGuid, size_t meshIndex) -> MeshIds {
						NameToGUID::const_iterator it = oldNamesToGuid.find(name);
						MeshIds guids;
						if (it != oldNamesToGuid.end()) guids = it->second;
						else guids = MeshIds{ GUID::Generate(), GUID::Generate(), GUID::Generate() };
						guids.index = meshIndex;
						nameToGuid[name] = guids;
						return guids;
					};

					for (size_t i = 0; i < cache->meshes.size(); i++) {
						const PolyMesh& mesh = *cache->meshes[i];
						const std::string name = getName(mesh);
						getGuid(name, nameToGuid, m_nameToGUID, i);
					}

					nameCounts.clear();
					m_nameToGUID = std::move(nameToGuid);
				}
				else revision = m_revision.load();

				std::vector<Reference<OBJTriMeshAsset>> triMeshAssets;
				for (auto it = m_nameToGUID.begin(); it != m_nameToGUID.end(); ++it) {
					const MeshIds& guids = it->second;
					const Reference<OBJPolyMeshAsset> polyMeshAsset = Object::Instantiate<OBJPolyMeshAsset>(guids.polyMesh, this, revision, guids.index);
					const Reference<OBJTriMeshAsset> triMeshAsset = Object::Instantiate<OBJTriMeshAsset>(guids.triMesh, guids.collisionMesh, polyMeshAsset);
					const Reference<Asset> collisionMesh = Physics::CollisionMesh::GetAsset(triMeshAsset, PhysicsInstance());
					AssetInfo info;
					const std::string& key = it->first;
					size_t nameLength = key.length();
					while (nameLength > 0u) {
						nameLength--;
						if (key[nameLength] == '_')
							break;
					}
					info.resourceName = key.substr(0u, nameLength);
					{
						info.asset = polyMeshAsset;
						reportAsset(info);
					}
					{
						info.asset = triMeshAsset;
						reportAsset(info);
						triMeshAssets.push_back(triMeshAsset);
					}
					{
						info.asset = collisionMesh;
						reportAsset(info);
					}
				}

				{
					Reference<OBJHeirarchyAsset> heirarchy = new OBJHeirarchyAsset(m_heirarchyId, this, std::move(triMeshAssets));
					heirarchy->ReleaseRef();
					AssetInfo info;
					info.resourceName = OS::Path(AssetFilePath().stem());
					info.asset = heirarchy;
					reportAsset(info);
				}

				return true;
			}
		};

		class OBJAssetImporterSerializer : public virtual FileSystemDatabase::AssetImporter::Serializer {
		public:
			inline OBJAssetImporterSerializer() : Serialization::ItemSerializer("OBJAssetImporterSerializer") {}

			inline virtual Reference<FileSystemDatabase::AssetImporter> CreateReader() final override {
				return Object::Instantiate<OBJAssetImporter>();
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, FileSystemDatabase::AssetImporter* target)const final override {
				if (target == nullptr) return;
				OBJAssetImporter* importer = dynamic_cast<OBJAssetImporter*>(target);
				if (importer == nullptr) {
					target->Log()->Error("OBJAssetImporterSerializer::GetFields - Target not of the correct type!");
					return;
				}
				{
					static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("Heirarchy", "All meshes under one transform");
					recordElement(serializer->Serialize(importer->m_heirarchyId));
				}
				std::vector<OBJAssetImporter::NameToGuids> mappings(importer->m_nameToGUID.begin(), importer->m_nameToGUID.end());
				{
					static const Reference<const ItemSerializer::Of<std::vector<OBJAssetImporter::NameToGuids>>> countSerializer =
						Serialization::ValueSerializer<int64_t>::For<std::vector<OBJAssetImporter::NameToGuids>>(
						"Count", "Number of entries",
						[](std::vector<OBJAssetImporter::NameToGuids>* mappings) -> int64_t { return static_cast<int64_t>(mappings->size()); },
						[](const int64_t& size, std::vector<OBJAssetImporter::NameToGuids>* mappings) { mappings->resize(static_cast<size_t>(size)); });
					recordElement(countSerializer->Serialize(mappings));
				}
				bool dirty = (mappings.size() != importer->m_nameToGUID.size());
				for (size_t i = 0; i < mappings.size(); i++) {
					OBJAssetImporter::NameToGuids& mapping = mappings[i];
					OBJAssetImporter::NameToGuids oldMapping = mapping;
					recordElement(OBJAssetImporter::NameToGUIDSerializer::Instance().Serialize(mapping));
					if (oldMapping.first != mapping.first 
						|| oldMapping.second.polyMesh != mapping.second.polyMesh
						|| oldMapping.second.triMesh != mapping.second.triMesh
						|| oldMapping.second.collisionMesh != mapping.second.collisionMesh)
						dirty = true;
				}
				if (dirty) {
					importer->m_nameToGUID.clear();
					for (size_t i = 0; i < mappings.size(); i++) {
						OBJAssetImporter::NameToGuids& mapping = mappings[i];
						importer->m_nameToGUID[mapping.first] = mapping.second;
					};
				}
			}

			inline static OBJAssetImporterSerializer* Instance() {
				static const Reference<OBJAssetImporterSerializer> instance = Object::Instantiate<OBJAssetImporterSerializer>();
				return instance;
			}

			inline static const OS::Path& Extension() {
				static const OS::Path extension = ".obj";
				return extension;
			}
		};
	}

	template<> void TypeIdDetails::OnRegisterType<WavefrontOBJAssetImporter>() {
		OBJAssetImporterSerializer::Instance()->Register(OBJAssetImporterSerializer::Extension());
	}
	template<> void TypeIdDetails::OnUnregisterType<WavefrontOBJAssetImporter>() {
		OBJAssetImporterSerializer::Instance()->Unregister(OBJAssetImporterSerializer::Extension());
	}
}
