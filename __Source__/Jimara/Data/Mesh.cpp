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
		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);
		auto addFace = [&](const Vector3& bl, const Vector3& br, const Vector3& tl, const Vector3& tr, const Vector3& normal) {
			uint32_t baseIndex = mesh->VertCount();
			mesh->AddVert(MeshVertex(bl, normal, Vector2(1.0f, 1.0f)));
			mesh->AddVert(MeshVertex(br, normal, Vector2(0.0f, 1.0f)));
			mesh->AddVert(MeshVertex(tl, normal, Vector2(1.0f, 0.0f)));
			mesh->AddVert(MeshVertex(tr, normal, Vector2(0.0f, 0.0f)));
			mesh->AddFace(TriangleFace(baseIndex, baseIndex + 2, baseIndex + 1));
			mesh->AddFace(TriangleFace(baseIndex + 1, baseIndex + 2, baseIndex + 3));
		};
		addFace(Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(0.0f, 0.0f, -1.0f));
		addFace(Vector3(end.x, start.y, start.z), Vector3(end.x, start.y, end.z), Vector3(end.x, end.y, start.z), Vector3(end.x, end.y, end.z), Vector3(1.0f, 0.0f, 0.0f));
		addFace(Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, end.z), Vector3(end.x, end.y, end.z), Vector3(start.x, end.y, end.z), Vector3(0.0f, 0.0f, 1.0f));
		addFace(Vector3(start.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(start.x, end.y, end.z), Vector3(start.x, end.y, start.z), Vector3(-1.0f, 0.0f, 0.0f));
		addFace(Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(start.x, end.y, end.z), Vector3(end.x, end.y, end.z), Vector3(0.0f, 1.0f, 0.0f));
		addFace(Vector3(start.x, start.y, end.z), Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(0.0f, -1.0f, 0.0f));
		return mesh;
	}

	Reference<TriMesh> TriMesh::Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string& name) {
		if (segments < 3) segments = 3;
		if (rings < 2) rings = 2;

		const float segmentStep = Radians(360.0f / static_cast<float>(segments));
		const float ringStep = Radians(180.0f / static_cast<float>(rings));
		const float uvHorStep = (1.0f / static_cast<float>(segments));

		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);

		auto addVert = [&](uint32_t ring, uint32_t segment) {
			const float segmentAngle = static_cast<float>(segment) * segmentStep;
			const float segmentCos = cos(segmentAngle);
			const float segmentSin = sin(segmentAngle);

			const float ringAngle = static_cast<float>(ring) * ringStep;
			const float ringCos = cos(ringAngle);
			const float ringSin = sqrt(1.0f - (ringCos * ringCos));

			const Vector3 normal(ringSin * segmentCos, ringCos, ringSin * segmentSin);
			mesh->AddVert(MeshVertex(normal * radius + center, normal, Vector2(1.0f - uvHorStep * static_cast<float>(segment), 1.0f - (ringCos + 1.0f) * 0.5f)));
		};

		for (uint32_t segment = 0; segment < segments; segment++) {
			addVert(0, segment);
			mesh->Vert(segment).uv.x += (uvHorStep * 0.5f);
		}
		for (uint32_t segment = 0; segment < segments; segment++) {
			addVert(1, segment);
			mesh->AddFace(TriangleFace(segment, segments + (segment + 1) % segments, segment + segments));
		}
		for (uint32_t ring = 2; ring < rings; ring++) {
			const uint32_t baseVert = (segments * (ring - 1));
			for (uint32_t segment = 0; segment < segments; segment++) {
				addVert(ring, segment);
				mesh->AddFace(TriangleFace(baseVert + segment, baseVert + segments + (segment + 1) % segments, baseVert + segment + segments));
				mesh->AddFace(TriangleFace(baseVert + segment, baseVert + (segment + 1) % segments, baseVert + segments + (segment + 1) % segments));
			}
		}
		const uint32_t baseVert = (segments * rings);
		for (uint32_t segment = 0; segment < segments; segment++) {
			addVert(rings, segment);
			mesh->Vert(baseVert + segment).uv.x += (uvHorStep * 0.5f);
			mesh->AddFace(TriangleFace(baseVert - segments + segment, baseVert - segments + (segment + 1) % segments, baseVert + segment));
		}

		return mesh;
	}

	Reference<TriMesh> TriMesh::ShadeFlat(const TriMesh* mesh, const std::string& name) {
		Reference<TriMesh> flatMesh = Object::Instantiate<TriMesh>(name);
		for (uint32_t i = 0; i < mesh->FaceCount(); i++) {
			const TriangleFace face = mesh->Face(i);
			MeshVertex a = mesh->Vert(face.a);
			MeshVertex b = mesh->Vert(face.b);
			MeshVertex c = mesh->Vert(face.c);
			a.normal = b.normal = c.normal = (a.normal + b.normal + c.normal) / 3.0f;
			flatMesh->AddVert(a).AddVert(b).AddVert(c).AddFace(TriangleFace(i * 3u, i * 3u + 1, i * 3u + 2));
		}
		return flatMesh;
	}
}
