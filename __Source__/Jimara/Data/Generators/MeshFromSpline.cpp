#include "MeshFromSpline.h"


namespace Jimara {
	namespace MeshFromSpline {
		namespace {
			inline static void AddFace(const TriMesh::Writer& writer, uint32_t a, uint32_t b, uint32_t c) { writer.AddFace(TriangleFace(a, b, c)); }
			inline static void AddFace(const TriMesh::Writer& writer, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
				writer.AddFace(TriangleFace(a, b, c));
				writer.AddFace(TriangleFace(a, c, d));
			}
			template<typename... Indices>
			inline static void AddFace(const PolyMesh::Writer& writer, Indices... indices) { writer.AddFace(PolygonFace({ indices... })); }
			inline static void AddFace(const TriMesh::Writer& writer, const PolygonFace& face) {
				for (size_t i = 2; i < face.Size(); i++)
					writer.AddFace(TriangleFace(face[0], face[i - 1], face[i]));
			}
			inline static void Addface(const PolyMesh::Writer& writer, const PolygonFace& face) { writer.AddFace(face); }

			template<typename MeshType>
			inline static Reference<MeshType> GenerateMeshFromSpline(
				SplineCurve spline, uint32_t splineSteps,
				RingCurve shape, uint32_t shapeSteps,
				Flags flags, const std::string_view& name) {
				
				// Create mesh:
				const Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				if (splineSteps <= 1 || shapeSteps <= 1) return mesh;
				typename MeshType::Writer writer(mesh);

				// Extract flags:
				auto hasFlags = [&](Flags f) { return (static_cast<uint8_t>(f) & static_cast<uint8_t>(flags)) == static_cast<uint8_t>(f); };
				const bool closeSpline = hasFlags(Flags::CLOSE_SPLINE);
				const bool closeShape = hasFlags(Flags::CLOSE_SHAPE);
				const bool capEnds = (!closeSpline) && hasFlags(Flags::CAP_ENDS);

				// Tools:
				auto safeNormalize = [&](const auto& value) {
					float magn = Math::Magnitude(value);
					if (magn < std::numeric_limits<float>::epsilon()) return value;
					else return value / magn;
				};



				// Extract extrusion shape:
				static thread_local std::vector<Vector2> shapeVerts;
				shapeVerts.clear();
				for (uint32_t i = 0; i < shapeSteps; i++)
					shapeVerts.push_back(shape(i));



				// Create main rings:
				{
					const Vector2 uvStep = Vector2(1.0f / static_cast<float>(shapeSteps - 1), 1.0f / static_cast<float>(splineSteps));
					for (uint32_t i = 0; i < splineSteps; i++) {
						const SplineVertex splineVert = spline(i);
						for (uint32_t j = 0; j < shapeSteps; j++) {
							const Vector2 shapeVert = shapeVerts[j];
							MeshVertex vertex;
							vertex.position =
								splineVert.position +
								splineVert.right * shapeVert.x +
								splineVert.up * shapeVert.y;
							vertex.normal = Vector3(0.0f);
							vertex.uv = uvStep * Vector2(static_cast<float>(j), static_cast<float>(i));
							writer.AddVert(vertex);
						}
					}
				}



				// Bridge main rings and calculate normals
				{
					auto addCornerNormal = [&](uint32_t a, uint32_t b, uint32_t c) {
						const MeshVertex& vA = writer.Vert(a);
						MeshVertex& vB = writer.Vert(b);
						const MeshVertex& vC = writer.Vert(c);
						vB.normal += safeNormalize(Math::Cross(vA.position - vB.position, vC.position - vB.position));
					};
					auto bridgeRings = [&](uint32_t a, uint32_t b) {
						const uint32_t baseA = a * shapeSteps;
						const uint32_t baseB = b * shapeSteps;
						auto bridgeLines = [&](uint32_t start, uint32_t end) {
							const uint32_t fA = baseA + start;
							const uint32_t fB = baseB + start;
							const uint32_t fC = baseB + end;
							const uint32_t fD = baseA + end;
							AddFace(writer, fA, fB, fC, fD);
							addCornerNormal(fD, fA, fB);
							addCornerNormal(fA, fB, fC);
							addCornerNormal(fB, fC, fD);
							addCornerNormal(fC, fD, fA);
						};
						for (uint32_t i = 1; i < shapeSteps; i++)
							bridgeLines(i, i - 1);
						if (closeShape)
							bridgeLines(0, shapeSteps - 1);
					};
					for (uint32_t i = 1; i < splineSteps; i++)
						bridgeRings(i - 1, i);
					if (closeSpline)
						bridgeRings(splineSteps - 1, 0);
					for (uint32_t i = 0; i < writer.VertCount(); i++) {
						MeshVertex& vertex = writer.Vert(i);
						vertex.normal = safeNormalize(vertex.normal);
					}
				}


				// Cap ends:
				if (capEnds) {
					Vector2 start = shapeVerts[0];
					Vector2 end = start;
					for (uint32_t i = 0; i < shapeSteps; i++) {
						const Vector2& pos = shapeVerts[i];
						if (start.x > pos.x) start.x = pos.x;
						else if (end.x < pos.x) end.x = pos.x;
						if (start.y > pos.y) start.y = pos.y;
						else if (end.y < pos.y) end.y = pos.y;
					}
					Vector2 delta = (end - start);
					float maxDimm;
					if (delta.x > delta.y) {
						maxDimm = delta.x;
						start.x -= (delta.x - delta.y) * 0.5f;
					}
					else {
						maxDimm = delta.y;
						start.y -= (delta.y - delta.x) * 0.5f;
					}
					const float scale = (maxDimm > std::numeric_limits<float>::epsilon()) ? (1.0f / maxDimm) : 1.0f;

					static PolygonFace capFace;
					auto capEnd = [&](uint32_t ring, bool inverse) {
						capFace.Clear();
						const SplineVertex splineVert = spline(ring);
						const Vector3 normal = safeNormalize(Math::Cross(splineVert.right, splineVert.up) * (inverse ? -1.0f : 1.0f));
						const uint32_t base = (ring * shapeSteps);
						uint32_t startIndex = writer.VertCount();
						for (uint32_t i = 0; i < shapeSteps; i++) {
							const MeshVertex& vert = writer.Vert(base + i);
							writer.AddVert(MeshVertex(vert.position, normal, (shapeVerts[i] - start) * scale));
						}
						if (inverse) {
							for (uint32_t i = 0; i < shapeSteps; i++)
								capFace.Push(startIndex + i);
						}
						else for (uint32_t i = 0; i < shapeSteps; i++)
							capFace.Push(startIndex + (shapeSteps - 1 - i));
						AddFace(writer, capFace);
					};
					capEnd(0, false);
					capEnd(splineSteps - 1, true);
					capFace.Clear();
				}

				shapeVerts.clear();
				return mesh;
			}
		}

		Reference<TriMesh> Tri(
			SplineCurve spline, uint32_t ringCount,
			RingCurve ring, uint32_t ringSegments,
			Flags flags, const std::string_view& name) {
			return GenerateMeshFromSpline<TriMesh>(spline, ringCount, ring, ringSegments, flags, name);
		}

		Reference<PolyMesh> Poly(
			SplineCurve spline, uint32_t ringCount,
			RingCurve ring, uint32_t ringSegments,
			Flags flags, const std::string_view& name) {
			return GenerateMeshFromSpline<PolyMesh>(spline, ringCount, ring, ringSegments, flags, name);
		}
	}
}
