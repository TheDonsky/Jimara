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
			for (uint32_t i = 0; i < reader.VertCount(); i++) {
				typename ResultMesh::Vertex vertex;
				CopyVertex(vertex, reader.Vert(i));
				writer.AddVert(std::move(vertex));
			}
			for (uint32_t i = 0; i < reader.FaceCount(); i++)
				PushFaces(writer, reader.Face(i));
			return result;
		}

		template<typename ResultMesh, typename SourceMesh>
		inline static void TransferSkinning(ResultMesh* result, const SourceMesh* source) {
			if (result == nullptr) return;
			bool resultSkinned = [&]() {
				typename ResultMesh::Reader reader(result);
				for (uint32_t i = 0; i < reader.VertCount(); i++) 
					if (reader.WeightCount(i) > 0) return true;
				return false;
			}();
			typename ResultMesh::Writer writer(result);
			if (resultSkinned) for (uint32_t i = 0; i < writer.VertCount(); i++)
				for (uint32_t j = 0; j < writer.BoneCount(); j++) writer.Weight(i, j) = 0;
			while (writer.BoneCount() > 0) writer.PopBone();

			if (source == nullptr) return;
			typename SourceMesh::Reader reader(source);
			for (uint32_t i = 0; i < reader.BoneCount(); i++) 
				writer.AddBone(reader.BoneData(i));
			for (uint32_t i = 0; i < reader.VertCount(); i++)
				for (uint32_t j = 0; j < reader.WeightCount(i); j++) {
					const typename SourceMesh::BoneWeight& boneWeight = reader.Weight(i, j);
					writer.Weight(i, boneWeight.boneIndex) = boneWeight.boneWeight;
				}
		}
	}

	Reference<TriMesh> ToTriMesh(const PolyMesh* polyMesh) { return TranslateMesh<TriMesh>(polyMesh); }

	Reference<PolyMesh> ToPolyMesh(const TriMesh* triMesh) { return TranslateMesh<PolyMesh>(triMesh); }

	Reference<SkinnedTriMesh> ToSkinnedTriMesh(const PolyMesh* polyMesh) {
		Reference<SkinnedTriMesh> result = TranslateMesh<SkinnedTriMesh>(polyMesh);
		TransferSkinning(result.operator->(), dynamic_cast<const SkinnedPolyMesh*>(polyMesh));
		return result;
	}

	Reference<SkinnedPolyMesh> ToSkinnedPolyMesh(const TriMesh* triMesh) {
		Reference<SkinnedPolyMesh> result = TranslateMesh<SkinnedPolyMesh>(triMesh);
		TransferSkinning(result.operator->(), dynamic_cast<const SkinnedTriMesh*>(triMesh));
		return result;
	}
}
