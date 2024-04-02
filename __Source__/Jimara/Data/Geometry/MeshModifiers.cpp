#include "MeshModifiers.h"
#include "MeshAnalysis.h"
#include <map>


namespace Jimara {
	namespace ModifyMesh {
		namespace {
			template<typename ActionType>
			inline static void ForEachVertex(const TriangleFace& face, const ActionType& action) {
				action(face.a);
				action(face.b);
				action(face.c);
			}

			template<typename ActionType>
			inline static void ForEachVertex(const PolygonFace& face, const ActionType& action) {
				for (size_t i = 0; i < face.Size(); i++) action(face[i]);
			}

			struct FaceAssembler {
				PolygonFace face;

				void AddTo(const TriMesh::Writer& writer)const {
					for (size_t i = 2; i < face.Size(); i++)
						writer.AddFace(TriangleFace(face[0], face[i - 1], face[i]));
				}

				void AddTo(const PolyMesh::Writer& writer)const { writer.AddFace(face); }
			};

			template<typename MeshType>
			inline static Reference<MeshType> FlatShadedMesh(const typename MeshType::Reader& reader, const std::string_view& name) {
				Reference<MeshType> flatMesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(flatMesh);
				FaceAssembler faceAssembler;
				for (uint32_t i = 0; i < reader.FaceCount(); i++) {
					const auto& face = reader.Face(i);
					Vector3 normal = Vector3(0.0f);
					ForEachVertex(face, [&](uint32_t vertexId) { normal += reader.Vert(vertexId).normal; });
					float magnitude = Math::Magnitude(normal);
					if (magnitude > std::numeric_limits<float>::epsilon())
						normal /= magnitude;
					faceAssembler.face.Clear();
					ForEachVertex(face, [&](uint32_t vertexId) {
						MeshVertex vertex = reader.Vert(vertexId);
						vertex.normal = normal;
						faceAssembler.face.Push(writer.VertCount());
						writer.AddVert(vertex);
						});
					faceAssembler.AddTo(writer);
				}
				return flatMesh;
			}

			template<typename MeshType>
			inline static Reference<MeshType> SmoothShadedMesh(const typename MeshType::Reader& reader, bool ignoreUV, const std::string_view& name) {
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(mesh);
				struct VertexId {
					Vector3 position = {};
					Vector2 uv = {};

					inline bool operator<(const VertexId& other)const {
						return
							(position.x < other.position.x) ? true : (position.x > other.position.x) ? false :
							(position.y < other.position.y) ? true : (position.y > other.position.y) ? false :
							(position.z < other.position.z) ? true : (position.z > other.position.z) ? false :
							(uv.x < other.uv.x) ? true : (uv.x > other.uv.x) ? false : (uv.y < other.uv.y);
					}
				};
				std::vector<Stacktor<uint32_t, 8u>> identicalVerts;
				std::map<VertexId, size_t> vertexIdBuckets;
				auto toVertexId = [&](const MeshVertex& vert) -> VertexId {
					VertexId id = {};
					id.position = vert.position;
					id.uv = ignoreUV ? Vector2(0.0f) : vert.uv;
					return id;
				};
				for (uint32_t i = 0u; i < reader.VertCount(); i++) {
					const VertexId id = toVertexId(reader.Vert(i));
					auto it = vertexIdBuckets.find(id);
					if (it == vertexIdBuckets.end()) {
						vertexIdBuckets[id] = identicalVerts.size();
						Stacktor<uint32_t, 8u>& bucket = identicalVerts.emplace_back();
						bucket.Push(i);
					}
				}
				for (size_t i = 0u; i < identicalVerts.size(); i++) {
					const Stacktor<uint32_t, 8u>& bucket = identicalVerts[i];
					MeshVertex vert = reader.Vert(bucket[0u]);
					for (size_t j = 1u; j < bucket.Size(); j++) {
						const MeshVertex& v = reader.Vert(bucket[j]);
						vert.normal += v.normal;
						vert.uv = Math::Lerp(vert.uv, v.uv, 1.0f / float(j + 1u));
					}
					vert.normal = Math::Normalize(vert.normal);
					writer.AddVert(vert);
				}
				FaceAssembler faceAssembler;
				for (uint32_t i = 0; i < reader.FaceCount(); i++) {
					const auto& face = reader.Face(i);
					ForEachVertex(face, [&](uint32_t vertexId) {
						faceAssembler.face.Push(static_cast<uint32_t>(vertexIdBuckets[toVertexId(reader.Vert(vertexId))]));
						});
					faceAssembler.AddTo(writer);
					faceAssembler.face.Clear();
				}
				return mesh;
			}

			template<typename MeshType>
			inline static Reference<MeshType> TransformedMesh(const Matrix4& transformation, const typename MeshType::Reader& reader, const std::string_view& name) {
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(mesh);
				for (uint32_t i = 0; i < reader.VertCount(); i++) {
					const MeshVertex& vertex = reader.Vert(i);
					writer.AddVert(MeshVertex(
						(Vector3)(transformation * Vector4(vertex.position, 1.0f)),
						(Vector3)(transformation * Vector4(vertex.normal, 0.0f)),
						vertex.uv));
				}
				for (uint32_t i = 0; i < reader.FaceCount(); i++)
					writer.AddFace(reader.Face(i));
				return mesh;
			}

			template<typename MeshType>
			inline static Reference<MeshType> MergedMesh(const MeshType* a, const MeshType* b, const std::string_view& name) {
				Reference<MeshType> mesh = Object::Instantiate<MeshType>(name);
				typename MeshType::Writer writer(mesh);
				if (a != nullptr) {
					typename MeshType::Reader reader(a);
					for (uint32_t i = 0; i < reader.VertCount(); i++) 
						writer.AddVert(reader.Vert(i));
					for (uint32_t i = 0; i < reader.FaceCount(); i++)
						writer.AddFace(reader.Face(i));
				}
				if (b != nullptr) {
					typename MeshType::Reader reader(b);
					const uint32_t baseVertex = writer.VertCount();
					for (uint32_t i = 0; i < reader.VertCount(); i++)
						writer.AddVert(reader.Vert(i));
					FaceAssembler faceAssembler;
					for (uint32_t i = 0; i < reader.FaceCount(); i++) {
						faceAssembler.face.Clear();
						ForEachVertex(reader.Face(i), [&](uint32_t vertexId) { faceAssembler.face.Push(vertexId + baseVertex); });
						faceAssembler.AddTo(writer);
					}
				}
				return mesh;
			}
		}

		Reference<TriMesh> ShadeFlat(const TriMesh* mesh, const std::string_view& name) {
			TriMesh::Reader reader(mesh);
			return FlatShadedMesh<TriMesh>(reader, name);
		}

		Reference<TriMesh> ShadeFlat(const TriMesh* mesh) {
			TriMesh::Reader reader(mesh);
			return FlatShadedMesh<TriMesh>(reader, reader.Name());
		}

		Reference<PolyMesh> ShadeFlat(const PolyMesh* mesh, const std::string_view& name) {
			PolyMesh::Reader reader(mesh);
			return FlatShadedMesh<PolyMesh>(reader, name);
		}

		Reference<PolyMesh> ShadeFlat(const PolyMesh* mesh) {
			PolyMesh::Reader reader(mesh);
			return FlatShadedMesh<PolyMesh>(reader, reader.Name());
		}



		Reference<TriMesh> ShadeSmooth(const TriMesh* mesh, bool ignoreUV, const std::string_view& name) {
			TriMesh::Reader reader(mesh);
			return SmoothShadedMesh<TriMesh>(reader, ignoreUV, name);
		}

		Reference<TriMesh> ShadeSmooth(const TriMesh* mesh, bool ignoreUV) {
			TriMesh::Reader reader(mesh);
			return SmoothShadedMesh<TriMesh>(reader, ignoreUV, reader.Name());
		}

		Reference<PolyMesh> ShadeSmooth(const PolyMesh* mesh, bool ignoreUV, const std::string_view& name) {
			PolyMesh::Reader reader(mesh);
			return SmoothShadedMesh<PolyMesh>(reader, ignoreUV, name);
		}

		Reference<PolyMesh> ShadeSmooth(const PolyMesh* mesh, bool ignoreUV) {
			PolyMesh::Reader reader(mesh);
			return SmoothShadedMesh<PolyMesh>(reader, ignoreUV, reader.Name());
		}



		Reference<TriMesh> Transform(const TriMesh* mesh, const Matrix4& transformation, const std::string_view& name) {
			TriMesh::Reader reader(mesh);
			return TransformedMesh<TriMesh>(transformation, reader, name);
		}

		Reference<TriMesh> Transform(const TriMesh* mesh, const Matrix4& transformation) {
			TriMesh::Reader reader(mesh);
			return TransformedMesh<TriMesh>(transformation, reader, reader.Name());
		}

		Reference<PolyMesh> Transform(const PolyMesh* mesh, const Matrix4& transformation, const std::string_view& name) {
			PolyMesh::Reader reader(mesh);
			return TransformedMesh<PolyMesh>(transformation, reader, name);
		}

		Reference<PolyMesh> Transform(const PolyMesh* mesh, const Matrix4& transformation) {
			PolyMesh::Reader reader(mesh);
			return TransformedMesh<PolyMesh>(transformation, reader, reader.Name());
		}



		Reference<TriMesh> Merge(const TriMesh* a, const TriMesh* b, const std::string_view& name) {
			return MergedMesh<TriMesh>(a, b, name);
		}

		Reference<PolyMesh> Merge(const PolyMesh* a, const PolyMesh* b, const std::string_view& name) {
			return MergedMesh<PolyMesh>(a, b, name);
		}


		Reference<TriMesh> SmoothMesh(const TriMesh* mesh, const std::string_view& name) {
			Reference<TriMesh> result = Object::Instantiate<TriMesh>(name);
			TriMesh::Reader src(mesh);
			TriMesh::Writer dst(result);
			const std::vector<Stacktor<uint32_t, 8u>> meshVertFaces = GetMeshVertexFaceIndices(src);
			for (uint32_t vId = 0u; vId < src.VertCount(); vId++) {
				const Stacktor<uint32_t, 8u>& faces = meshVertFaces[vId];
				if (faces.Size() <= 0u) {
					dst.AddVert(src.Vert(vId));
					continue;
				}
				Vector3 posSum(0.0f);
				Vector3 normalSum(0.0f);
				for (size_t fIdId = 0u; fIdId < faces.Size(); fIdId++) {
					const TriangleFace& face = src.Face(faces[fIdId]);
					for (size_t fvId = 0u; fvId < 3u; fvId++) {
						const MeshVertex& vert = src.Vert(face[fvId]);
						posSum += vert.position;
						normalSum += vert.normal;
					}
				}
				dst.AddVert(MeshVertex(posSum / float(faces.Size() * 3u), Math::Normalize(normalSum), src.Vert(vId).uv));
			}
			for (uint32_t fId = 0u; fId < src.FaceCount(); fId++)
				dst.AddFace(src.Face(fId));
			return result;
		}
	}
}
