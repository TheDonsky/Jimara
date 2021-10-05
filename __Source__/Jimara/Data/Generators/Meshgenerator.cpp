#include "MeshGenerator.h"


namespace Jimara {
	namespace GenerateMesh {
		namespace {
			inline static void AddFace(const TriMesh::Writer& writer, uint32_t a, uint32_t b, uint32_t c) { writer.AddFace(TriangleFace(a, b, c)); }
			inline static void AddFace(const TriMesh::Writer& writer, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
				writer.AddFace(TriangleFace(a, b, c));
				writer.AddFace(TriangleFace(a, c, d));
			}
			template<typename... Indices>
			inline static void AddFace(const PolyMesh::Writer& writer, Indices... indices) { writer.AddFace(PolygonFace({ indices... })); }

			template<typename MeshType>
			inline static Reference<MeshType> CreateBox(const Vector3& start, const Vector3& end, const std::string_view& name) {
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(mesh);
				auto addFace = [&](const Vector3& bl, const Vector3& br, const Vector3& tl, const Vector3& tr, const Vector3& normal) {
					uint32_t baseIndex = static_cast<uint32_t>(writer.VertCount());
					writer.AddVert(MeshVertex(bl, normal, Vector2(0.0f, 1.0f)));
					writer.AddVert(MeshVertex(br, normal, Vector2(1.0f, 1.0f)));
					writer.AddVert(MeshVertex(tr, normal, Vector2(1.0f, 0.0f)));
					writer.AddVert(MeshVertex(tl, normal, Vector2(0.0f, 0.0f)));
					AddFace(writer, baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 3);
				};
				addFace(Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(0.0f, 0.0f, -1.0f));
				addFace(Vector3(end.x, start.y, start.z), Vector3(end.x, start.y, end.z), Vector3(end.x, end.y, start.z), Vector3(end.x, end.y, end.z), Vector3(1.0f, 0.0f, 0.0f));
				addFace(Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, end.z), Vector3(end.x, end.y, end.z), Vector3(start.x, end.y, end.z), Vector3(0.0f, 0.0f, 1.0f));
				addFace(Vector3(start.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(start.x, end.y, end.z), Vector3(start.x, end.y, start.z), Vector3(-1.0f, 0.0f, 0.0f));
				addFace(Vector3(start.x, end.y, start.z), Vector3(end.x, end.y, start.z), Vector3(start.x, end.y, end.z), Vector3(end.x, end.y, end.z), Vector3(0.0f, 1.0f, 0.0f));
				addFace(Vector3(start.x, start.y, end.z), Vector3(end.x, start.y, end.z), Vector3(start.x, start.y, start.z), Vector3(end.x, start.y, start.z), Vector3(0.0f, -1.0f, 0.0f));
				return mesh;
			}

			template<typename MeshType>
			class SphereVertexHelper {
			private:
				const uint32_t m_segments;
				const uint32_t m_rings;
				const float m_segmentStep;
				const float m_ringStep;
				const float m_uvHorStep;
				const float m_radius;
				uint32_t m_baseVert = 0;
				typename MeshType::Writer m_writer;

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

				inline SphereVertexHelper(MeshType* mesh, uint32_t segments, uint32_t rings, float radius, Vector3 c)
					: m_segments(segments), m_rings(rings)
					, m_segmentStep(Math::Radians(360.0f / static_cast<float>(segments)))
					, m_ringStep(Math::Radians(180.0f / static_cast<float>(rings)))
					, m_uvHorStep(1.0f / static_cast<float>(segments))
					, m_radius(radius), m_writer(mesh), center(c) {
					for (uint32_t segment = 0; segment < m_segments; segment++) {
						MeshVertex vertex = GetSphereVertex(0, segment);
						vertex.uv.x += m_uvHorStep * 0.5f;
						m_writer.AddVert(vertex);
					}
					for (uint32_t segment = 0; segment < m_segments; segment++) {
						m_writer.AddVert(GetSphereVertex(1, segment));
						AddFace(m_writer, segment, m_segments + segment, segment + m_segments + 1);
					}
					m_writer.AddVert(GetSphereVertex(1, m_segments));
					m_baseVert = m_segments;
				}

				inline void AddMidRing(uint32_t ring) {
					for (uint32_t segment = 0; segment < m_segments; segment++) {
						m_writer.AddVert(GetSphereVertex(ring, segment));
						AddFace(m_writer, m_baseVert + segment, m_baseVert + m_segments + segment + 1, m_baseVert + m_segments + segment + 2, m_baseVert + segment + 1);
					}
					m_writer.AddVert(GetSphereVertex(ring, m_segments));
					m_baseVert += m_segments + 1;
				}

				~SphereVertexHelper() {
					for (uint32_t segment = 0; segment < m_segments; segment++) {
						MeshVertex vertex = GetSphereVertex(m_rings, segment);
						vertex.uv.x += m_uvHorStep * 0.5f;
						m_writer.AddVert(vertex);
						AddFace(m_writer, m_baseVert + segment, m_baseVert + m_segments + 1 + segment, m_baseVert + segment + 1);
					}
				}

				inline uint32_t VertCount()const { return m_writer.VertCount(); }
			};

			template<typename MeshType>
			inline static Reference<MeshType> CreateSphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name) {
				if (segments < 3) segments = 3;
				if (rings < 2) rings = 2;
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				{
					SphereVertexHelper<MeshType> helper(mesh, segments, rings, radius, center);
					for (uint32_t ring = 2; ring < rings; ring++) helper.AddMidRing(ring);
				}
				return mesh;
			}

			template<typename MeshType>
			inline static Reference<MeshType> CreateCapsule(const Vector3& center, float radius, float midHeight, uint32_t segments, uint32_t tipRings, uint32_t midDivisions, const std::string_view& name) {
				if (segments < 3) segments = 3;
				if (tipRings < 1) tipRings = 1;
				if (midDivisions < 1) midDivisions = 1;
				uint32_t bottomVertEnd;
				uint32_t topVertStart;
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				{
					SphereVertexHelper<MeshType> helper(mesh, segments, tipRings * 2, radius, center + Vector3(0.0f, midHeight * 0.5f, 0.0f));
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
					typename MeshType::Writer writer(mesh);
					for (uint32_t i = 0; i < bottomVertEnd; i++)
						writer.Vert(i).uv.y *= tipSquish;
					uint32_t midRingId = 0;
					for (uint32_t i = bottomVertEnd; i < topVertStart; i += (static_cast<uint32_t>(segments) + 1u)) {
						midRingId++;
						float h = ((1.0f - tipSquish) / static_cast<float>(midDivisions) * midRingId) + (tipSquish * 0.5f);
						for (uint32_t j = i; j <= (i + segments); j++) writer.Vert(j).uv.y = h;
					}
					for (uint32_t i = topVertStart; i < writer.VertCount(); i++)
						writer.Vert(i).uv.y = (1.0f - (1.0f - writer.Vert(i).uv.y) * tipSquish);
				}
				return mesh;
			}

			template<typename MeshType>
			inline static Reference<MeshType> CreatePlane(const Vector3& center, const Vector3& u, const Vector3& v, Size2 divisions, const std::string_view& name) {
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

				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(mesh);

				auto addVert = [&](uint32_t i, uint32_t j) {
					MeshVertex vert = {};
					vert.position = START + (U_STEP * ((float)i)) + (V_STEP * ((float)j));
					vert.normal = UP;
					vert.uv = Vector2(i * U_TEX_STEP, 1.0f - j * V_TEX_STEP);
					writer.AddVert(vert);
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
						AddFace(writer, a, b, c, d);
					}

				return mesh;
			}
		}

		namespace Tri {
			Reference<TriMesh> Box(const Vector3& start, const Vector3& end, const std::string_view& name) { 
				return CreateBox<TriMesh>(start, end, name); 
			}
			
			Reference<TriMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name) { 
				return CreateSphere<TriMesh>(center, radius, segments, rings, name); 
			}
			
			Reference<TriMesh> Capsule(const Vector3& center, float radius, float midHeight, uint32_t segments, uint32_t tipRings, uint32_t midDivisions, const std::string_view& name) {
				return CreateCapsule<TriMesh>(center, radius, midHeight, segments, tipRings, midDivisions, name);
			}

			Reference<TriMesh> Plane(const Vector3& center, const Vector3& u, const Vector3& v, Size2 divisions, const std::string_view& name) {
				return CreatePlane<TriMesh>(center, u, v, divisions, name);
			}
		}

		namespace Poly {
			Reference<PolyMesh> Box(const Vector3& start, const Vector3& end, const std::string_view& name) {
				return CreateBox<PolyMesh>(start, end, name);
			}

			Reference<PolyMesh> Sphere(const Vector3& center, float radius, uint32_t segments, uint32_t rings, const std::string_view& name) {
				return CreateSphere<PolyMesh>(center, radius, segments, rings, name);
			}

			Reference<PolyMesh> Capsule(const Vector3& center, float radius, float midHeight, uint32_t segments, uint32_t tipRings, uint32_t midDivisions, const std::string_view& name) {
				return CreateCapsule<PolyMesh>(center, radius, midHeight, segments, tipRings, midDivisions, name);
			}

			Reference<PolyMesh> Plane(const Vector3& center, const Vector3& u, const Vector3& v, Size2 divisions, const std::string_view& name) {
				return CreatePlane<PolyMesh>(center, u, v, divisions, name);
			}
		}

		Reference<TriMesh> ShadeFlat(const TriMesh* mesh, const std::string_view& name) {
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
				writer.AddVert(a);
				writer.AddVert(b);
				writer.AddVert(c);
				writer.AddFace(TriangleFace(i * 3u, i * 3u + 1, i * 3u + 2));
			}
			return flatMesh;
		}

		Reference<TriMesh> ShadeFlat(const TriMesh* mesh) {
			return ShadeFlat(mesh, TriMesh::Reader(mesh).Name());
		}
	}
}
