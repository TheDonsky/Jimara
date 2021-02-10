#include "Mesh.h"
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
		inline static bool LoadObjData(const std::string& filename, OS::Logger* logger, tinyobj::attrib_t& attrib, std::vector<tinyobj::shape_t>& shapes) {
			std::vector<tinyobj::material_t> materials;
			std::string warning, error;

			if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, filename.c_str())) {
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

		inline static uint32_t GetVertexId(const tinyobj::index_t& index, const tinyobj::attrib_t& attrib, std::map<OBJVertex, uint32_t>& vertexIndexCache, TriMesh* mesh) {
			OBJVertex vert = {};
			vert.vertexId = static_cast<uint32_t>(index.vertex_index);
			vert.normalId = static_cast<uint32_t>(index.normal_index);
			vert.uvId = static_cast<uint32_t>(index.texcoord_index);

			std::map<OBJVertex, uint32_t>::const_iterator it = vertexIndexCache.find(vert);
			if (it != vertexIndexCache.end()) return it->second;

			uint32_t vertId = mesh->VertCount();

			MeshVertex vertex = {};

			size_t baseVertex = static_cast<size_t>(3u) * vert.vertexId;
			vertex.position = Vector3(
				static_cast<float>(attrib.vertices[baseVertex]),
				static_cast<float>(attrib.vertices[baseVertex + 1]),
				static_cast<float>(attrib.vertices[baseVertex + 2]));

			size_t baseNormal = static_cast<size_t>(3u) * vert.normalId;
			vertex.normal = Vector3(
				static_cast<float>(attrib.normals[baseNormal]),
				static_cast<float>(attrib.normals[baseNormal + 1]),
				static_cast<float>(attrib.normals[baseNormal + 2]));

			size_t baseUV = static_cast<size_t>(2u) * vert.uvId;
			vertex.uv = Vector2(
				static_cast<float>(attrib.texcoords[baseUV]),
				1.0f - static_cast<float>(attrib.texcoords[baseUV + 1]));

			mesh->AddVert(vertex);

			vertexIndexCache.insert(std::make_pair(vert, vertId));
			return vertId;
		}

		inline static Reference<TriMesh> ExtractMesh(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape) {
			Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(shape.name);
			std::map<OBJVertex, uint32_t> vertexIndexCache;

			size_t indexStart = 0;
			for (size_t faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++) {
				size_t indexEnd = indexStart + shape.mesh.num_face_vertices[faceId];
				if ((indexEnd - indexStart) > 2) {
					TriangleFace face = {};
					face.a = GetVertexId(shape.mesh.indices[indexStart], attrib, vertexIndexCache, mesh);
					face.c = GetVertexId(shape.mesh.indices[indexStart + 1], attrib, vertexIndexCache, mesh);
					for (size_t i = indexStart + 2; i < indexEnd; i++) {
						face.b = face.c;
						face.c = GetVertexId(shape.mesh.indices[i], attrib, vertexIndexCache, mesh);
						mesh->AddFace(face);
					}
				}
				indexStart = indexEnd;
			}
			return mesh;
		}
	}

	TriMesh::TriMesh(const std::string& name) 
		: Mesh<MeshVertex, TriangleFace>(name) {}

	TriMesh::~TriMesh() {}

	TriMesh::TriMesh(const Mesh<MeshVertex, TriangleFace>& other) 
		: Mesh<MeshVertex, TriangleFace>(other) {}

	TriMesh& TriMesh::operator=(const Mesh<MeshVertex, TriangleFace>& other) {
		*((Mesh<MeshVertex, TriangleFace>*)this) = other;
		return (*this);
	}

	TriMesh::TriMesh(Mesh<MeshVertex, TriangleFace>&& other) noexcept
		: Mesh<MeshVertex, TriangleFace>(std::move(other)) {}

	TriMesh& TriMesh::operator=(Mesh<MeshVertex, TriangleFace>&& other) noexcept {
		*((Mesh<MeshVertex, TriangleFace>*)this) = std::move(other);
		return (*this);
	}

	Reference<TriMesh> TriMesh::FromOBJ(const std::string& filename, const std::string& objectName, OS::Logger* logger) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		if (!LoadObjData(filename, logger, attrib, shapes)) return nullptr;
		for (size_t i = 0; i < shapes.size(); i++) {
			const tinyobj::shape_t& shape = shapes[i];
			if (shape.name == objectName) 
				return ExtractMesh(attrib, shape);
		}
		if (logger != nullptr)
			logger->Error("TriMesh::FromOBJ - '" + objectName + "' could not be found in '" + filename + "'");
		return nullptr;
	}

	std::vector<Reference<TriMesh>> TriMesh::FromOBJ(const std::string& filename, OS::Logger* logger) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		if (!LoadObjData(filename, logger, attrib, shapes)) return std::vector<Reference<TriMesh>>();
		std::vector<Reference<TriMesh>> meshes;
		for (size_t i = 0; i < shapes.size(); i++)
			meshes.push_back(ExtractMesh(attrib, shapes[i]));
		return meshes;
	}

	Reference<TriMesh> TriMesh::Box(const Vector3& start, const Vector3& end, const std::string& name) {
		// __TODO__: Implement this crap...
		return nullptr;
	}

	Reference<TriMesh> TriMesh::Sphere(uint32_t segments, uint32_t rings, float radius, const std::string& name) {
		// __TODO__: Implement this crap...
		return nullptr;
	}
}
