#include "Mesh.h"
#include <stdio.h>


namespace Jimara {
	namespace {
		inline static void CopyVertex(MeshVertex& dst, const MeshVertex& src) { dst = src; }

		inline static void PushFaces(const TriMesh::Writer& writer, const PolygonFace& face) {
			if (face.Size() < 3) return;
			TriangleFace triFace = {};
			triFace.a = face[0];
			triFace.c = face[1];
			for (size_t j = 2; j < face.Size(); j++) {
				triFace.b = triFace.c;
				triFace.c = face[j];
				writer.AddFace(triFace);
			}
		}

		inline static void PushFaces(const PolyMesh::Writer& writer, const TriangleFace& face) {
			writer.AddFace(PolygonFace({ face.a, face.b, face.c }));
		}

		template<typename ResultMesh, typename SourceMesh>
		inline static Reference<ResultMesh> TranslateMesh(const SourceMesh* source) {
			if (source == nullptr) return nullptr;
			typename SourceMesh::Reader reader(source);
			Reference<ResultMesh> result = Object::Instantiate<ResultMesh>(reader.Name());
			typename ResultMesh::Writer writer(result);
			for (size_t i = 0; i < reader.VertCount(); i++) {
				typename ResultMesh::Vertex vertex;
				CopyVertex(vertex, reader.Vert(i));
				writer.AddVert(std::move(vertex));
			}
			for (size_t i = 0; i < reader.FaceCount(); i++)
				PushFaces(writer, reader.Face(i));
			return result;
		}
	}

	Reference<TriMesh> ToTriMesh(const PolyMesh* polyMesh) { return TranslateMesh<TriMesh>(polyMesh); }

	Reference<PolyMesh> ToPolyMesh(const TriMesh* triMesh) { return TranslateMesh<PolyMesh>(triMesh); }
}
