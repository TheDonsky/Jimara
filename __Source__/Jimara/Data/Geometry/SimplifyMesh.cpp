#include "MeshModifiers.h"
#include "MeshAnalysis.h"
#include "../../Math/Algorithms/PolygonTools.h"
#include "../../Math/Random.h"
#include <iostream>


namespace Jimara {
	namespace ModifyMesh {
		Reference<TriMesh> SimplifyMesh(const TriMesh* mesh, float angleThreshold, size_t maxIterations, const std::string_view& name) {
			Reference<TriMesh> back;
			Reference<TriMesh> front;
			const float cosineThresh = std::cos(Math::Radians(angleThreshold));

			std::vector<size_t> neighborVertexCounts;
			std::vector<uint32_t> insertionIndex;
			std::vector<bool> vertCanBeRemoved;

			std::vector<bool> triIncludedInLoop;
			std::vector<uint32_t> loopFaceIndices;
			std::vector<Vector2> loopPolygon;

			for (size_t iteration = 0u; iteration < maxIterations; iteration++) {
				TriMesh::Reader src(back == nullptr ? mesh : static_cast<const TriMesh*>(back));

				neighborVertexCounts.clear();
				vertCanBeRemoved.clear();
				insertionIndex.clear();
				for (uint32_t vId = 0u; vId < src.VertCount(); vId++) {
					vertCanBeRemoved.push_back(false);
					neighborVertexCounts.push_back(0u);
					insertionIndex.push_back(~uint32_t(0u));
				}

				const std::vector<Stacktor<uint32_t, 8u>> vertexFaceIndices = GetMeshVertexFaceIndices(src);

				auto faceMarkedForRemoval = [&](const TriangleFace& face) {
					return
						vertCanBeRemoved[face.a] ||
						vertCanBeRemoved[face.b] ||
						vertCanBeRemoved[face.c];
				};

				auto anyNeighborScheduledForRemoval = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++)
						if (faceMarkedForRemoval(src.Face(faces[fIdId])))
							return true;
					return false;
				};

				auto isEdgeVertex = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						neighborVertexCounts[face.a]++;
						neighborVertexCounts[face.b]++;
						neighborVertexCounts[face.c]++;
					}
					bool isEdge = false;
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						isEdge |=
							(neighborVertexCounts[face.a] <= 1u) ||
							(neighborVertexCounts[face.b] <= 1u) ||
							(neighborVertexCounts[face.c] <= 1u);
					}
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						neighborVertexCounts[face.a] = 0u;
						neighborVertexCounts[face.b] = 0u;
						neighborVertexCounts[face.c] = 0u;
					}
					return isEdge;
				};

				/*
				auto hasEdgeNeighbor = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						for (size_t i = 0u; i < 3u; i++) {
							const uint32_t v = face[i];
							if (v != vId && isEdgeVertex(v))
								return true;
						}
					}
					return false;
				};
				*/

				auto faceNormal = [&](uint32_t faceId) {
					const TriangleFace& face = src.Face(faceId);
					const Vector3& a = src.Vert(face.a).position;
					const Vector3& b = src.Vert(face.b).position;
					const Vector3& c = src.Vert(face.c).position;
					return Math::Normalize(Math::Cross(c - a, b - a));
				};

				auto averageNormal = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					Vector3 normalSum = Vector3(0.0f);
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++)
						normalSum += faceNormal(faces[fIdId]);
					return Math::Normalize(normalSum);
				};

				auto facesAreAligned = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					const Vector3 normal = averageNormal(vId);
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++)
						if (Math::Dot(faceNormal(faces[fIdId]), normal) < cosineThresh)
							return false;
					return true;
				};

				size_t removedVertexCount = 0u;
				for (uint32_t vId = 0u; vId < src.VertCount(); vId++) {
					if (anyNeighborScheduledForRemoval(vId))
						continue;
					else if (isEdgeVertex(vId))
						continue;
					else if (facesAreAligned(vId)) {
						vertCanBeRemoved[vId] = true;
						removedVertexCount++;
					}
				}

				if (removedVertexCount <= 0u) {
					if (back != nullptr)
						return back;
					else {
						if (front == nullptr)
							front = Object::Instantiate<TriMesh>(name);
						TriMesh::Writer dst(front);
						while (dst.FaceCount() > 0u)
							dst.PopFace();
						while (dst.VertCount() > 0u)
							dst.PopVert();
						for (uint32_t vId = 0u; vId < src.VertCount(); vId++)
							dst.AddVert(src.Vert(vId));
						for (uint32_t fId = 0u; fId < src.FaceCount(); fId++)
							dst.AddFace(src.Face(fId));
					}
					return front;
				}
				else {
					if (front == nullptr)
						front = Object::Instantiate<TriMesh>(name);
					TriMesh::Writer dst(front);
					while (dst.FaceCount() > 0u)
						dst.PopFace();
					while (dst.VertCount() > 0u)
						dst.PopVert();
					bool vertsRemoved = false;

					auto copiedVertId = [&](uint32_t srcVId) -> uint32_t {
						uint32_t& record = insertionIndex[srcVId];
						const uint32_t vertCount = dst.VertCount();
						if (record >= vertCount) {
							record = vertCount;
							dst.AddVert(src.Vert(srcVId));
						}
						return record;
					};

					auto copyFace = [&](const TriangleFace& face) {
						const uint32_t a = copiedVertId(face.a);
						const uint32_t b = copiedVertId(face.b);
						const uint32_t c = copiedVertId(face.c);
						dst.AddFace(TriangleFace(a, b, c));
					};

					for (uint32_t fId = 0u; fId < src.FaceCount(); fId++) {
						const TriangleFace& face = src.Face(fId);
						if (!faceMarkedForRemoval(face))
							copyFace(face);
					}

					for (uint32_t vId = 0u; vId < src.VertCount(); vId++) {
						if (!vertCanBeRemoved[vId])
							continue;

						const Vector3 normal = averageNormal(vId);
						Vector3 tmpUp = std::abs(Math::Dot(Math::Forward(), normal)) < 0.5f
							? Math::Forward() : Math::Right();
						const Vector3 right = Math::Normalize(Math::Cross(normal, tmpUp));
						const Vector3 up = Math::Normalize(Math::Cross(right, normal));
						//const float rotAngle = ;
						//const Vector3 right = Math::Normalize(std::cos(rotAngle) * right0 + std::sin(rotAngle) * up0);
						//const Vector3 up = Math::Normalize(Math::Cross(right, normal));

						auto getOuterEdge = [&](const TriangleFace& face) {
							for (uint32_t e = 0u; e < 3u; e++)
								if (face[e] != vId && face[(size_t(e) + 1u) % 3u] != vId)
									return e;
							return ~uint32_t(0u);
						};

						// Make sure all faces have an 'outer edge' to keep:
						const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
						if (faces.Size() > 0u) {
							bool notAllOuterEdgesPresent = false;
							for (size_t i = 0u; i < faces.Size(); i++)
								if (getOuterEdge(src.Face(faces[i])) >= 3u) {
									notAllOuterEdgesPresent = true;
									break;
								}
							if (notAllOuterEdgesPresent) {
								for (size_t i = 0u; i < faces.Size(); i++)
									copyFace(src.Face(faces[i]));
								continue;
							}
						}
						else continue;


						// Find looping order:
						triIncludedInLoop.clear();
						loopFaceIndices.clear();
						for (size_t i = 0u; i < faces.Size(); i++)
							triIncludedInLoop.push_back(false);
						triIncludedInLoop[0u] = true;
						loopFaceIndices.push_back(0u);
						while (true) {
							const size_t chainSizeSoFar = loopFaceIndices.size();
							const uint32_t lastFaceId = faces[loopFaceIndices[chainSizeSoFar - 1u]];
							const TriangleFace& lastFace = src.Face(lastFaceId);
							const uint32_t outerEdge = getOuterEdge(lastFace);
							const uint32_t lastVert = lastFace[(size_t(getOuterEdge(lastFace)) + 1u) % 3u];
							for (size_t i = 0u; i < faces.Size(); i++) {
								if (triIncludedInLoop[i])
									continue;
								const TriangleFace& face = src.Face(faces[i]);
								const uint32_t edge = getOuterEdge(face);
								if (face[edge] != lastVert)
									continue;
								triIncludedInLoop[i] = true;
								loopFaceIndices.push_back(static_cast<uint32_t>(i));
								break;
							}
							if (chainSizeSoFar == loopFaceIndices.size())
								break;
						}

						// If the loop is incomplete, we have a problem of sorts and the vertex can not be removed:
						if (loopFaceIndices.size() != faces.Size()) {
							for (size_t i = 0u; i < faces.Size(); i++)
								copyFace(src.Face(faces[i]));
						}


						// Build projected polygon:
						{
							loopPolygon.clear();
							const Vector3 origin = src.Vert(vId).position;
							for (size_t i = 0u; i < loopFaceIndices.size(); i++) {
								const TriangleFace& face = src.Face(faces[loopFaceIndices[i]]);
								const Vector3 relPos = src.Vert(face[getOuterEdge(face)]).position - origin;
								loopPolygon.push_back(Vector2(Math::Dot(right, relPos), Math::Dot(up, relPos)));
							}
						}


						// Triangulate and add triangles back:
						{
							auto getSrcVertId = [&](size_t polyId) -> uint32_t {
								const TriangleFace& face = src.Face(faces[loopFaceIndices[polyId]]);
								const uint32_t edgeId = getOuterEdge(face);
								return face[edgeId];
							};
							auto addFace = [&](size_t ai, size_t bi, size_t ci) {
								copyFace(TriangleFace(getSrcVertId(bi), getSrcVertId(ai), getSrcVertId(ci)));
							};
							const uint32_t faceCount = dst.FaceCount();
							PolygonTools::Triangulate(loopPolygon.data(), loopPolygon.size(), Callback<size_t, size_t, size_t>::FromCall(&addFace));
							if (faceCount == dst.FaceCount()) {
								for (size_t i = 0u; i < faces.Size(); i++)
									copyFace(src.Face(faces[i]));
							}
							else vertsRemoved = true;
						}
					}

					std::swap(front, back);
					if (!vertsRemoved)
						break;
				}
			}

			return back;
		}
	}
}
