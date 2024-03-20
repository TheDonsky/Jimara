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

				auto getOuterEdge = [&](const TriangleFace& face, uint32_t vId) {
					for (uint32_t e = 0u; e < 3u; e++)
						if (face[e] != vId && face[(size_t(e) + 1u) % 3u] != vId)
							return e;
					return ~uint32_t(0u);
				};

				struct CornerSplit {
					size_t vertFaceA = {}, vertFaceB = {};
				};

				auto findEdgeLoopSplit = [&](uint32_t vId) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						neighborVertexCounts[face.a]++;
						neighborVertexCounts[face.b]++;
						neighborVertexCounts[face.c]++;
					}
					std::optional<CornerSplit> split;
					size_t splitCount = 0u;
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);

						if ((neighborVertexCounts[face.a] <= 1u) ||
							(neighborVertexCounts[face.b] <= 1u) ||
							(neighborVertexCounts[face.c] <= 1u)) {
							if (!split.has_value())
								split = CornerSplit{ ~size_t(0u), ~size_t(0u) };
						}
						else continue;

						const uint32_t outerEdge = getOuterEdge(face, vId);
						if (outerEdge >= 3u)
							continue;
						else if (neighborVertexCounts[face[outerEdge]] <= 1u) {
							split.value().vertFaceA = fIdId;
							splitCount++;
						}
						else if (neighborVertexCounts[face[(outerEdge + 1u) % 3u]] <= 1u) {
							split.value().vertFaceB = fIdId;
							splitCount++;
						}
					}
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
						const TriangleFace& face = src.Face(faces[fIdId]);
						neighborVertexCounts[face.a] = 0u;
						neighborVertexCounts[face.b] = 0u;
						neighborVertexCounts[face.c] = 0u;
					}

					if (splitCount > 2u)
						split = CornerSplit{ ~size_t(0u), ~size_t(0u) };

					return split;
				};

				auto traiangleNormal = [](const Vector3& a, const Vector3& b, const Vector3& c) {
					return Math::Normalize(Math::Cross(c - a, b - a));
				};

				auto faceNormal = [&](uint32_t faceId) {
					const TriangleFace& face = src.Face(faceId);
					const Vector3& a = src.Vert(face.a).position;
					const Vector3& b = src.Vert(face.b).position;
					const Vector3& c = src.Vert(face.c).position;
					return traiangleNormal(a, b, c);
				};

				auto getAverageNormal = [&](uint32_t vId, const auto& filter) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					Vector3 normalSum = Vector3(0.0f);
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++)
						if (filter(fIdId))
							normalSum += faceNormal(faces[fIdId]);
					return Math::Normalize(normalSum);
				};

				auto averageNormal = [&](uint32_t vId) {
					return getAverageNormal(vId, [](auto) { return true; });
				};

				auto checkFaceGroupAreAligned = [&](uint32_t vId, const auto& filter) {
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					const Vector3 normal = getAverageNormal(vId, filter);
					for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++)
						if (filter(fIdId) && Math::Dot(faceNormal(faces[fIdId]), normal) < cosineThresh)
							return false;
					return true;
				};

				auto facesAreAligned = [&](uint32_t vId) {
					return checkFaceGroupAreAligned(vId, [](auto) { return true; });
				};

				auto findNonEdgeCornerSplit = [&](uint32_t vId) {
					std::optional<CornerSplit> result;
					float bestCosine = 2.0f;
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
					const Vector3 o = src.Vert(vId).position;
					for (size_t i = 0u; i < faces.Size(); i++) {
						const TriangleFace faceI = src.Face(faces[i]);
						const uint32_t outerEdgeI = getOuterEdge(faceI, vId);
						if (outerEdgeI >= 3u)
							continue;
						const Vector3 dirI = Math::Normalize(src.Vert(faceI[outerEdgeI]).position - o);
						const Vector3 dirNext = Math::Normalize(src.Vert(faceI[(outerEdgeI + 1u) % 3u]).position - o);
						const Vector3 sideDir = Math::Normalize(dirNext - dirI * Math::Dot(dirI, dirNext));
						const Vector3 normalI = faceNormal(faces[i]);
						for (size_t j = (i + 1u); j < faces.Size(); j++) {
							const TriangleFace faceJ = src.Face(faces[j]);
							const uint32_t outerEdgeJ = getOuterEdge(faceJ, vId);
							if (outerEdgeJ >= 3u)
								continue;
							const Vector3 dirJ = Math::Normalize(src.Vert(faceJ[outerEdgeJ]).position - o);
							const float cosine = Math::Dot(dirI, dirJ);
							if (cosine >= (-cosineThresh))
								continue;
							if (cosine >= bestCosine)
								continue;
							auto filterRange = [&](size_t fIdId) {
								const TriangleFace face = src.Face(faces[fIdId]);
								const uint32_t outerEdge = getOuterEdge(face, vId);
								if (outerEdge >= 3u)
									return false;
								const Vector3 dir = Math::Normalize(
									src.Vert(face[outerEdge]).position - o +
									src.Vert(face[(outerEdge + 1u) % 3u]).position - o);
								if (Math::Dot(dir, dirI) <= cosine)
									return false;
								if (Math::Dot(dir, sideDir) <= 0.0f)
									return false;
								const Vector3 normal = faceNormal(faces[fIdId]);
								if (Math::Dot(normal, normalI) < 0.0f)
									return false;
								return true;
							};
							if (!checkFaceGroupAreAligned(vId, filterRange))
								continue;
							if (!checkFaceGroupAreAligned(vId, [&](size_t fIdId) { return !filterRange(fIdId); }))
								continue;
							result = CornerSplit{ i, j };
							bestCosine = cosine;
						}
					}
					return result;
				};
				
				auto findCornerSplit = [&](uint32_t vId, const std::optional<CornerSplit>& edgeLoopSplit) {
					if (!edgeLoopSplit.has_value())
						return findNonEdgeCornerSplit(vId);
					const Vector3 o = src.Vert(vId).position;
					const CornerSplit& split = edgeLoopSplit.value();
					const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];

					const TriangleFace& fA = src.Face(faces[split.vertFaceA]);
					const Vector3 a = src.Vert(fA[getOuterEdge(fA, vId)]).position;
					const Vector3 dirA = Math::Normalize(a - o);
					
					const TriangleFace& fB = src.Face(faces[split.vertFaceB]);
					const Vector3 b = src.Vert(fB[(getOuterEdge(fB, vId) + 1u) % 3u]).position;
					const Vector3 dirB = Math::Normalize(b - o);

					const float cosine = Math::Dot(dirA, dirB);
					if (cosine >= (-cosineThresh))
						return std::optional<CornerSplit>();
					else return edgeLoopSplit;
				};

				size_t removedVertexCount = 0u;
				size_t vertexSplitCount = 0u;
				for (uint32_t vId = 0u; vId < src.VertCount(); vId++) {
					if (anyNeighborScheduledForRemoval(vId))
						continue;
					const std::optional<CornerSplit> edgeLoopSplit = findEdgeLoopSplit(vId);
					if ((!edgeLoopSplit.has_value()) && facesAreAligned(vId)) {
						vertCanBeRemoved[vId] = true;
						removedVertexCount++;
						continue;
					}
					const std::optional<CornerSplit> cornerSplit = findCornerSplit(vId, edgeLoopSplit);
					if (cornerSplit.has_value()) {
						vertCanBeRemoved[vId] = true;
						vertexSplitCount++;
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

						// Make sure all faces have an 'outer edge' to keep:
						const Stacktor<uint32_t, 8u>& faces = vertexFaceIndices[vId];
						if (faces.Size() > 0u) {
							bool notAllOuterEdgesPresent = false;
							for (size_t i = 0u; i < faces.Size(); i++)
								if (getOuterEdge(src.Face(faces[i]), vId) >= 3u) {
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
						const std::optional<CornerSplit> edgeLoopSplit = findEdgeLoopSplit(vId);
						loopFaceIndices.push_back(static_cast<uint32_t>(edgeLoopSplit.value_or(CornerSplit{ 0u, 0u }).vertFaceA));
						triIncludedInLoop[loopFaceIndices[0u]] = true;
						while (true) {
							const size_t chainSizeSoFar = loopFaceIndices.size();
							const uint32_t lastFaceId = faces[loopFaceIndices[chainSizeSoFar - 1u]];
							const TriangleFace& lastFace = src.Face(lastFaceId);
							const uint32_t outerEdge = getOuterEdge(lastFace, vId);
							const uint32_t lastVert = lastFace[(size_t(getOuterEdge(lastFace, vId)) + 1u) % 3u];
							for (size_t i = 0u; i < faces.Size(); i++) {
								if (triIncludedInLoop[i])
									continue;
								const TriangleFace& face = src.Face(faces[i]);
								const uint32_t edge = getOuterEdge(face, vId);
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
							continue;
						}


						// Build projected polygon:
						auto buildLoopPolygon = [&](size_t loopSize, const auto& getVertId) {
							loopPolygon.clear();
							const Vector3 origin = src.Vert(vId).position;
							for (size_t i = 0u; i < loopSize; i++) {
								const Vector3 relPos = src.Vert(getVertId(i)).position - origin;
								loopPolygon.push_back(Vector2(Math::Dot(right, relPos), Math::Dot(up, relPos)));
							}
						};


						// Triangulate and add triangles back:
						auto triangulateLoop = [&](size_t loopSize, const auto& getSrcVertId) -> bool {
							if (loopSize <= 2)
								return true;
							auto addFace = [&](size_t ai, size_t bi, size_t ci) {
								Stacktor<TriangleFace, 4u> faces;
								faces.Push(TriangleFace(getSrcVertId(bi), getSrcVertId(ai), getSrcVertId(ci)));
								for (size_t i = 0u; i < loopSize; i++) {
									const uint32_t srcVertId = getSrcVertId(i);
									const Vector3 point = src.Vert(srcVertId).position;
									for (size_t tId = 0u; tId < faces.Size(); tId++) {
										const TriangleFace face = faces[tId];
										if (srcVertId == face.a || srcVertId == face.b || srcVertId == face.c)
											continue;
										bool faceSplit = false;
										for (size_t eId = 0u; eId < 3u; eId++) {
											const uint32_t ea = face[eId];
											const uint32_t eb = face[(eId + 1u) % 3u];
											const Vector3 a = src.Vert(ea).position;
											const Vector3 b = src.Vert(eb).position;
											const Vector3 dir = Math::Normalize(b - a);
											const Vector3 dirA = Math::Normalize(point - a);
											const Vector3 dirB = Math::Normalize(point - b);
											const constexpr float thresh = 0.99999f;
											if (Math::Dot(dirA, dir) <= thresh || Math::Dot(dirB, dir) >= -thresh)
												continue;

											faceSplit = true;
											faces[tId] = faces[faces.Size() - 1u];
											faces.Pop();
											const uint32_t ec = face[(eId + 2u) % 3u];
											faces.Push(TriangleFace(ea, srcVertId, ec));
											faces.Push(TriangleFace(srcVertId, eb, ec));
											break;
										}
										if (faceSplit)
											break;
									}
								}
								for (size_t tId = 0u; tId < faces.Size(); tId++)
									copyFace(faces[tId]);
							};
							const uint32_t faceCount = dst.FaceCount();
							PolygonTools::Triangulate(loopPolygon.data(), loopPolygon.size(), Callback<size_t, size_t, size_t>::FromCall(&addFace));
							return (dst.FaceCount() - faceCount) == (loopSize - 2u);
						};

						bool vertexRemoved = false;
						const size_t initialFaceCount = dst.FaceCount();
						if (edgeLoopSplit.has_value()) {
							auto getSrcVertId = [&](size_t polyId) -> uint32_t {
								size_t delta = (polyId >= loopFaceIndices.size()) ? 1u : 0u;
								const TriangleFace& face = src.Face(faces[loopFaceIndices[polyId - delta]]);
								const uint32_t edgeId = getOuterEdge(face, vId);
								return face[(edgeId + delta) % 3u];
							};
							buildLoopPolygon(loopFaceIndices.size() + 1u, getSrcVertId);
							vertexRemoved = triangulateLoop(loopFaceIndices.size() + 1u, getSrcVertId);
						}
						else if (facesAreAligned(vId)) {
							auto getSrcVertId = [&](size_t polyId) -> uint32_t {
								const TriangleFace& face = src.Face(faces[loopFaceIndices[polyId]]);
								const uint32_t edgeId = getOuterEdge(face, vId);
								return face[edgeId];
							};
							buildLoopPolygon(loopFaceIndices.size(), getSrcVertId);
							vertexRemoved = triangulateLoop(loopFaceIndices.size(), getSrcVertId);
						}
						else {
							auto trySplit = [&]() {
								const std::optional<CornerSplit> splitOpt = findCornerSplit(vId, edgeLoopSplit);
								if (!splitOpt.has_value())
									return;
								CornerSplit loopSplit;
								loopSplit.vertFaceA = loopSplit.vertFaceB = ~0u;
								for (size_t i = 0u; i < loopFaceIndices.size(); i++) {
									const uint32_t faceIdId = loopFaceIndices[i];
									if (faceIdId == splitOpt.value().vertFaceA)
										loopSplit.vertFaceA = i;
									if (faceIdId == splitOpt.value().vertFaceB)
										loopSplit.vertFaceB = i;
								}
								assert(loopFaceIndices.size() == faces.Size());
								if (loopSplit.vertFaceA >= faces.Size() || loopSplit.vertFaceB >= faces.Size())
									return;
								const size_t loopStart = loopFaceIndices.size();
								auto eraseLoop = [&]() {
									while (loopFaceIndices.size() > loopStart)
										loopFaceIndices.pop_back();
								};

								auto buildRange = [&](size_t start, size_t end) {
									for (size_t i = start; i != end; i = (i + 1u) % faces.Size()) {
										const TriangleFace& face = src.Face(faces[loopFaceIndices[i]]);
										loopFaceIndices.push_back(face[getOuterEdge(face, vId)]);
									}
									{
										const TriangleFace& face = src.Face(faces[loopFaceIndices[end]]);
										loopFaceIndices.push_back(face[getOuterEdge(face, vId)]);
									}
									auto getSrcVertId = [&](size_t i) { return loopFaceIndices[loopStart + i]; };
									buildLoopPolygon(loopFaceIndices.size() - loopStart, getSrcVertId);
									const bool rv = triangulateLoop(loopFaceIndices.size() - loopStart, getSrcVertId);
									eraseLoop();
									return rv;
								};
								
								if (!buildRange(loopSplit.vertFaceA, loopSplit.vertFaceB))
									return;
								if (!buildRange(loopSplit.vertFaceB, loopSplit.vertFaceA))
									return;
								vertexRemoved = true;
							};
							trySplit();
						}
						if (vertexRemoved) 
							vertsRemoved = true; 
						else {
							while (dst.FaceCount() > initialFaceCount)
								dst.PopFace();
							for (size_t i = 0u; i < faces.Size(); i++)
								copyFace(src.Face(faces[i]));
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
