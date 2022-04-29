#include "MeshModifiers.h"


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

				void AddTo(const TriMesh::Writer& writer) {
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

		namespace Tri {
			Reference<TriMesh> ShadedFlat(const TriMesh* mesh, const std::string_view& name) {
				TriMesh::Reader reader(mesh);
				return FlatShadedMesh<TriMesh>(reader, name);
			}

			Reference<TriMesh> ShadedFlat(const TriMesh* mesh) {
				TriMesh::Reader reader(mesh);
				return FlatShadedMesh<TriMesh>(reader, reader.Name());
			}

			Reference<TriMesh> Transformed(const Matrix4& transformation, const TriMesh* mesh, const std::string_view& name) {
				TriMesh::Reader reader(mesh);
				return TransformedMesh<TriMesh>(transformation, reader, name);
			}

			Reference<TriMesh> Transformed(const Matrix4& transformation, const TriMesh* mesh) {
				TriMesh::Reader reader(mesh);
				return TransformedMesh<TriMesh>(transformation, reader, reader.Name());
			}

			Reference<TriMesh> Merge(const TriMesh* a, const TriMesh* b, const std::string_view& name) {
				return MergedMesh<TriMesh>(a, b, name);
			}
		}

		namespace Poly {
			Reference<PolyMesh> ShadedFlat(const PolyMesh* mesh, const std::string_view& name) {
				PolyMesh::Reader reader(mesh);
				return FlatShadedMesh<PolyMesh>(reader, name);
			}

			Reference<PolyMesh> ShadedFlat(const PolyMesh* mesh) {
				PolyMesh::Reader reader(mesh);
				return FlatShadedMesh<PolyMesh>(reader, reader.Name());
			}

			Reference<PolyMesh> Transformed(const Matrix4& transformation, const PolyMesh* mesh, const std::string_view& name) {
				PolyMesh::Reader reader(mesh);
				return TransformedMesh<PolyMesh>(transformation, reader, name);
			}

			Reference<PolyMesh> Transformed(const Matrix4& transformation, const PolyMesh* mesh) {
				PolyMesh::Reader reader(mesh);
				return TransformedMesh<PolyMesh>(transformation, reader, reader.Name());
			}

			Reference<PolyMesh> Merge(const PolyMesh* a, const PolyMesh* b, const std::string_view& name) {
				return MergedMesh<PolyMesh>(a, b, name);
			}
		}
	}
}
