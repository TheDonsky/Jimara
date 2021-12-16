#include "WavefrontOBJ.h"
#include "../AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
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
			std::vector<Reference<const PolyMesh>> meshes;
			
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
						std::vector<Reference<PolyMesh>> meshes = PolyMeshesFromOBJ(pathAndRevision.path, logger);
						for (size_t i = 0; i < meshes.size(); i++)
							cache->meshes.push_back(meshes[i]);
						return cache;
						});
				}
			};
		};

		class OBJPolyMeshAsset : public virtual Asset {
		private:
			const Reference<FileSystemDatabase::AssetImporter> m_importer;
			const size_t m_revision;
			const size_t m_meshIndex;

			Reference<OBJAssetDataCache> m_cache;

		public:
			inline OBJPolyMeshAsset(const GUID& guid, FileSystemDatabase::AssetImporter* importer, size_t revision, size_t meshIndex) 
				: Asset(guid), m_importer(importer), m_revision(revision), m_meshIndex(meshIndex) {}

			virtual Reference<const Resource> LoadResource() final override {
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
					Reference<const PolyMesh>& reference = m_cache->meshes[m_meshIndex];
					const Reference<const Resource> result = reference;
					reference = nullptr;
					return result;
				}
			}

			inline virtual void UnloadResource(Reference<const Resource> resource) final override {
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
					Reference<const PolyMesh>& reference = m_cache->meshes[m_meshIndex];
					if (reference != nullptr) {
						if (logger != nullptr)
							logger->Error("OBJPolyMeshAsset::UnloadResource - Possible circular dependencies detected! <internal error>");
					}
					else reference = resource;
				}
				m_cache = nullptr;
			}
		};

		class OBJTriMeshAsset : public virtual Asset {
		private:
			const Reference<OBJPolyMeshAsset> m_meshAsset;

			Reference<const PolyMesh> m_sourceMesh;

		public:
			inline OBJTriMeshAsset(const GUID& guid, OBJPolyMeshAsset* meshAsset)
				: Asset(guid), m_meshAsset(meshAsset) {
				assert(m_meshAsset != nullptr);
			}

		protected:
			virtual Reference<const Resource> LoadResource() final override {
				m_sourceMesh = m_meshAsset->LoadAs<const PolyMesh>();
				if (m_sourceMesh != nullptr) return ToTriMesh(m_sourceMesh);
				else return nullptr;
			}

			inline virtual void UnloadResource(Reference<const Resource> resource) final override {
				m_sourceMesh = nullptr; // This will let go of the reference to the FBXDataCache
			}
		};

		class OBJAssetImporterSerializer;

		class OBJAssetImporter : public virtual FileSystemDatabase::AssetImporter {
		private:
			std::atomic<size_t> m_revision = 0;
			typedef std::pair<std::string, std::pair<GUID, GUID>> NameToGuids;
			typedef std::unordered_map<std::string, std::pair<GUID, GUID>> NameToGUID;
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
						recordElement(serializer->Serialize(target->second.first));
					}
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("TriMesh");
						recordElement(serializer->Serialize(target->second.second));
					}
				}

				inline static const NameToGUIDSerializer& Instance() {
					static const NameToGUIDSerializer serializer;
					return serializer;
				}
			};

			friend class OBJAssetImporterSerializer;

		public:
			inline virtual bool Import(Callback<Asset*> reportAsset) final override {
				// __TODO__: If last write time matches the one from the previous import, probably no need to rescan the FBX (will make startup times faster)...
				size_t revision = m_revision.fetch_add(1);
				Reference<OBJAssetDataCache> cache = OBJAssetDataCache::Cache::For({ AssetFilePath(), revision }, Log());
				if (cache == nullptr) return false;

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
				
				auto getGuid = [&](const std::string& name, NameToGUID& nameToGuid, const NameToGUID& oldNamesToGuid) -> std::pair<GUID, GUID> {
					NameToGUID::const_iterator it = oldNamesToGuid.find(name);
					std::pair<GUID, GUID> guids;
					if (it != oldNamesToGuid.end()) guids = it->second;
					else guids = std::make_pair(GUID::Generate(), GUID::Generate());
					nameToGuid[name] = guids;
					return guids;
				};
				
				for (size_t i = 0; i < cache->meshes.size(); i++) {
					const std::string name = getName(*cache->meshes[i]);
					std::pair<GUID, GUID> guids = getGuid(name, nameToGuid, m_nameToGUID);
					const Reference<OBJPolyMeshAsset> polyMeshAsset = Object::Instantiate<OBJPolyMeshAsset>(guids.first, this, revision, i);
					const Reference<OBJTriMeshAsset> triMeshAsset = Object::Instantiate<OBJTriMeshAsset>(guids.second, polyMeshAsset);
					reportAsset(polyMeshAsset);
					reportAsset(triMeshAsset);
				}
				
				nameCounts.clear();
				m_nameToGUID = std::move(nameToGuid);
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
					if (oldMapping.first != mapping.first || oldMapping.second.first != mapping.second.first || oldMapping.second.second != mapping.second.second)
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
