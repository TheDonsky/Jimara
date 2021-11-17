#include "WavefrontOBJ.h"
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
