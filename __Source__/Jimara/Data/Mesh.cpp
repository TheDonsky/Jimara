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

		inline static uint32_t GetVertexId(const tinyobj::index_t& index, const tinyobj::attrib_t& attrib, std::map<OBJVertex, uint32_t>& vertexIndexCache, TriMesh::Writer& mesh) {
			OBJVertex vert = {};
			vert.vertexId = static_cast<uint32_t>(index.vertex_index);
			vert.normalId = static_cast<uint32_t>(index.normal_index);
			vert.uvId = static_cast<uint32_t>(index.texcoord_index);

			std::map<OBJVertex, uint32_t>::const_iterator it = vertexIndexCache.find(vert);
			if (it != vertexIndexCache.end()) return it->second;

			uint32_t vertId = static_cast<uint32_t>(mesh.Verts().size());

			MeshVertex vertex = {};

			size_t baseVertex = static_cast<size_t>(3u) * vert.vertexId;
			vertex.position = Vector3(
				static_cast<float>(attrib.vertices[baseVertex]),
				static_cast<float>(attrib.vertices[baseVertex + 1]),
				- static_cast<float>(attrib.vertices[baseVertex + 2]));

			size_t baseNormal = static_cast<size_t>(3u) * vert.normalId;
			vertex.normal = Vector3(
				static_cast<float>(attrib.normals[baseNormal]),
				static_cast<float>(attrib.normals[baseNormal + 1]),
				- static_cast<float>(attrib.normals[baseNormal + 2]));

			size_t baseUV = static_cast<size_t>(2u) * vert.uvId;
			vertex.uv = Vector2(
				static_cast<float>(attrib.texcoords[baseUV]),
				1.0f - static_cast<float>(attrib.texcoords[baseUV + 1]));

			mesh.Verts().push_back(vertex);

			vertexIndexCache.insert(std::make_pair(vert, vertId));
			return vertId;
		}

		inline static Reference<TriMesh> ExtractMesh(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape) {
			Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(shape.name);
			TriMesh::Writer writer(mesh);
			std::map<OBJVertex, uint32_t> vertexIndexCache;

			size_t indexStart = 0;
			for (size_t faceId = 0; faceId < shape.mesh.num_face_vertices.size(); faceId++) {
				size_t indexEnd = indexStart + shape.mesh.num_face_vertices[faceId];
				if ((indexEnd - indexStart) > 2) {
					TriangleFace face = {};
					face.a = GetVertexId(shape.mesh.indices[indexStart], attrib, vertexIndexCache, writer);
					face.c = GetVertexId(shape.mesh.indices[indexStart + 1], attrib, vertexIndexCache, writer);
					for (size_t i = indexStart + 2; i < indexEnd; i++) {
						face.b = face.c;
						face.c = GetVertexId(shape.mesh.indices[i], attrib, vertexIndexCache, writer);
						writer.Faces().push_back(face);
					}
				}
				indexStart = indexEnd;
			}
			return mesh;
		}
	}

	TriMesh::TriMesh(const std::string_view& name)
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

	Reference<TriMesh> TriMesh::Box(const Vector3& start, const Vector3& end, const std::string_view& name) {
		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);
		TriMesh::Writer writer(mesh);
		auto addFace = [&](const Vector3& bl, const Vector3& br, const Vector3& tl, const Vector3& tr, const Vector3& normal) {
			uint32_t baseIndex = static_cast<uint32_t>(writer.Verts().size());
			writer.Verts().push_back(MeshVertex(bl, normal, Vector2(0.0f, 1.0f)));
			writer.Verts().push_back(MeshVertex(br, normal, Vector2(1.0f, 1.0f)));
			writer.Verts().push_back(MeshVertex(tl, normal, Vector2(0.0f, 0.0f)));
			writer.Verts().push_back(MeshVertex(tr, normal, Vector2(1.0f, 0.0f)));
			writer.Faces().push_back(TriangleFace(baseIndex, baseIndex + 1, baseIndex + 2));
			writer.Faces().push_back(TriangleFace(baseIndex + 1, baseIndex + 3, baseIndex + 2));
		};
		addFace(Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(0.0f, 0.0f, -1.0f));
		addFace(Vector3(end.x, start.y, start.z), Vector3(end.x, start.y, end.z), Vector3(end.x, end.y, start.z), Vector3(end.x, end.y, end.z), Vector3(1.0f, 0.0f, 0.0f));
		addFace(Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, end.z), Vector3(end.x, end.y, end.z), Vector3(start.x, end.y, end.z), Vector3(0.0f, 0.0f, 1.0f));
		addFace(Vector3(start.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(start.x, end.y, end.z), Vector3(start.x, end.y, start.z), Vector3(-1.0f, 0.0f, 0.0f));
		addFace(Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(start.x, end.y, end.z), Vector3(end.x, end.y, end.z), Vector3(0.0f, 1.0f, 0.0f));
		addFace(Vector3(start.x, start.y, end.z), Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(0.0f, -1.0f, 0.0f));
		return mesh;
	}


	namespace {
		class SphereVertexHelper {
		private:
			const uint32_t m_segments;
			const uint32_t m_rings;
			const float m_segmentStep;
			const float m_ringStep;
			const float m_uvHorStep;
			const float m_radius;
			uint32_t m_baseVert = 0;
			TriMesh::Writer m_writer;

		public:
			Vector3 center = Vector3(0.0f);


			inline MeshVertex GetSphereVertex(uint32_t ring, uint32_t segment) {
				const float segmentAngle = static_cast<float>(segment) * m_segmentStep;
				const float segmentCos = cos(segmentAngle);
				const float segmentSin = sin(segmentAngle);

				const float ringAngle = static_cast<float>(ring) * m_ringStep;
				const float ringCos = cos(ringAngle);
				const float ringSin = sqrt(1.0f - (ringCos * ringCos));

				const Vector3 normal(ringSin * segmentCos, ringCos, ringSin * segmentSin);
				return MeshVertex(normal * m_radius + center, normal, Vector2(m_uvHorStep * static_cast<float>(segment), 1.0f - (ringCos + 1.0f) * 0.5f));
			}

			inline SphereVertexHelper(TriMesh* mesh, uint32_t segments, uint32_t rings, float radius, Vector3 c)
				: m_segments(segments), m_rings(rings)
				, m_segmentStep(Math::Radians(360.0f / static_cast<float>(segments)))
				, m_ringStep(Math::Radians(180.0f / static_cast<float>(rings)))
				, m_uvHorStep(1.0f / static_cast<float>(segments))
				, m_radius(radius), m_writer(mesh), center(c) {
				for (uint32_t segment = 0; segment < m_segments; segment++) {
					MeshVertex vertex = GetSphereVertex(0, segment);
					vertex.uv.x += m_uvHorStep * 0.5f;
					m_writer.Verts().push_back(vertex);
				}
				for (uint32_t segment = 0; segment < m_segments; segment++) {
					m_writer.Verts().push_back(GetSphereVertex(1, segment));
					m_writer.Faces().push_back(TriangleFace(segment, m_segments + segment, segment + m_segments + 1));
				}
				m_writer.Verts().push_back(GetSphereVertex(1, m_segments));
				m_baseVert = m_segments;
			}

			inline void AddMidRing(uint32_t ring) {
				for (uint32_t segment = 0; segment < m_segments; segment++) {
					m_writer.Verts().push_back(GetSphereVertex(ring, segment));
					m_writer.Faces().push_back(TriangleFace(m_baseVert + segment, m_baseVert + m_segments + segment + 1, m_baseVert + segment + m_segments + 2));
					m_writer.Faces().push_back(TriangleFace(m_baseVert + segment, m_baseVert + m_segments + segment + 2, m_baseVert + segment + 1));
				}
				m_writer.Verts().push_back(GetSphereVertex(ring, m_segments));
				m_baseVert += m_segments + 1;
			}

			~SphereVertexHelper() {
				for (uint32_t segment = 0; segment < m_segments; segment++) {
					MeshVertex vertex = GetSphereVertex(m_rings, segment);
					vertex.uv.x += m_uvHorStep * 0.5f;
					m_writer.Verts().push_back(vertex);
					m_writer.Faces().push_back(TriangleFace(m_baseVert + segment, m_baseVert + m_segments + 1 + segment, m_baseVert + segment + 1));
				}
			}

			inline size_t VertCount()const { return m_writer.Verts().size(); }
		};
	}

	Reference<TriMesh> TriMesh::Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name) {
		if (segments < 3) segments = 3;
		if (rings < 2) rings = 2;
		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);
		{
			SphereVertexHelper helper(mesh, segments, rings, radius, center);
			for (uint32_t ring = 2; ring < rings; ring++) helper.AddMidRing(ring);
		}
		return mesh;
	}

	Reference<TriMesh> TriMesh::Capsule(
		const Vector3& center, float radius, float midHeight,
		uint32_t segments, uint32_t tipRings, uint32_t midDivisions,
		const std::string_view& name) {
		if (segments < 3) segments = 3;
		if (tipRings < 1) tipRings = 1;
		if (midDivisions < 1) midDivisions = 1;
		size_t bottomVertEnd;
		size_t topVertStart;
		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);
		{
			SphereVertexHelper helper(mesh, segments, tipRings * 2, radius, center + Vector3(0.0f, midHeight * 0.5f, 0.0f));
			for (uint32_t ring = 2; ring <= tipRings; ring++) helper.AddMidRing(ring);
			bottomVertEnd = helper.VertCount();
			const Vector3 centerStep = Vector3(0.0f, -midHeight / static_cast<float>(midDivisions), 0.0f);
			for (uint32_t i = 0; i < midDivisions; i++) {
				helper.center += centerStep;
				helper.AddMidRing(tipRings);
			}
			topVertStart = helper.VertCount();
			for (uint32_t ring = tipRings + 1; ring < (tipRings << 1); ring++) helper.AddMidRing(ring);
		}
		{
			const float tipHeight = abs(Math::Pi() * radius);
			const float totalHeight = (tipHeight + abs(midHeight));
			const float tipSquish = (tipHeight * ((totalHeight > 0.0f) ? (1.0f / totalHeight) : 1.0f));
			TriMesh::Writer writer(mesh);
			for (size_t i = 0; i < bottomVertEnd; i++)
				writer.Verts()[i].uv.y *= tipSquish;
			size_t midRingId = 0;
			for (size_t i = bottomVertEnd; i < topVertStart; i += (static_cast<size_t>(segments) + 1u)) {
				midRingId++;
				float h = ((1.0f - tipSquish) / static_cast<float>(midDivisions) * midRingId) + (tipSquish * 0.5f);
				for (size_t j = i; j <= (i + segments); j++) writer.Verts()[j].uv.y = h;
			}
			for (size_t i = topVertStart; i < writer.Verts().size(); i++)
				writer.Verts()[i].uv.y = (1.0f - (1.0f - writer.Verts()[i].uv.y) * tipSquish);
		}
		return mesh;
	}

	Reference<TriMesh> TriMesh::Plane(const Vector3& center, const Vector3& u, const Vector3& v, Size2 divisions, const std::string_view& name) {
		if (divisions.x < 1) divisions.x = 1;
		if (divisions.y < 1) divisions.y = 1;
		
		const Vector3 START = center - (u + v) * 0.5f;
		const Vector3 UP = [&] { 
			const Vector3 cross = Math::Cross(v, u);
			const float magn = sqrt(Math::Dot(cross, cross));
			return magn > 0.0f ? (cross / magn) : cross;
		}();

		const float U_TEX_STEP = 1.0f / ((float)divisions.x);
		const float V_TEX_STEP = 1.0f / ((float)divisions.x);

		const Vector3 U_STEP = (u * U_TEX_STEP);
		const Vector3 V_STEP = (v * V_TEX_STEP);

		const uint32_t U_POINTS = divisions.x + 1;
		const uint32_t V_POINTS = divisions.y + 1;

		Reference<TriMesh> mesh = Object::Instantiate<TriMesh>(name);
		TriMesh::Writer writer(mesh);
		
		auto addVert = [&](uint32_t i, uint32_t j) {
			MeshVertex vert = {};
			vert.position = START + (U_STEP * ((float)i)) + (V_STEP * ((float)j));
			vert.normal = UP;
			vert.uv = Vector2(i * U_TEX_STEP, 1.0f - j * V_TEX_STEP);
			writer.Verts().push_back(vert);
		};

		for (uint32_t j = 0; j < V_POINTS; j++)
			for (uint32_t i = 0; i < U_POINTS; i++)
				addVert(i, j);

		for (uint32_t i = 0; i < divisions.x; i++)
			for (uint32_t j = 0; j < divisions.y; j++) {
				const uint32_t a = (i * U_POINTS) + j;
				const uint32_t b = a + 1;
				const uint32_t c = b + U_POINTS;
				const uint32_t d = c - 1;
				writer.Faces().push_back(TriangleFace(a, b, c));
				writer.Faces().push_back(TriangleFace(a, c, d));
			}
		
		return mesh;
	}

	Reference<TriMesh> TriMesh::ShadeFlat(const TriMesh* mesh, const std::string& name) {
		Reference<TriMesh> flatMesh = Object::Instantiate<TriMesh>(name);
		TriMesh::Writer writer(flatMesh);
		TriMesh::Reader reader(mesh);
		for (uint32_t i = 0; i < reader.FaceCount(); i++) {
			const TriangleFace face = reader.Face(i);
			MeshVertex a = reader.Vert(face.a);
			MeshVertex b = reader.Vert(face.b);
			MeshVertex c = reader.Vert(face.c);
			Vector3 sum = (a.normal + b.normal + c.normal);
			float magnitude = sqrt(Math::Dot(sum, sum));
			a.normal = b.normal = c.normal = sum / magnitude;
			writer.Verts().push_back(a);
			writer.Verts().push_back(b);
			writer.Verts().push_back(c);
			writer.Faces().push_back(TriangleFace(i * 3u, i * 3u + 1, i * 3u + 2));
		}
		return flatMesh;
	}
}
