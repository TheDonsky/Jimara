#include "MeshAnalysis.h"


namespace Jimara {
	std::vector<Stacktor<uint32_t, 8u>> GetMeshVertexFaceIndices(const TriMesh::Reader& reader) {
		std::vector<Stacktor<uint32_t, 8u>> vertFaceIds;
		vertFaceIds.resize(reader.VertCount());
		for (uint32_t i = 0u; i < reader.FaceCount(); i++) {
			const TriangleFace& face = reader.Face(i);
			if (face.a < vertFaceIds.size())
				vertFaceIds[face.a].Push(i);
			if (face.b != face.a && face.b < vertFaceIds.size())
				vertFaceIds[face.b].Push(i);
			if (face.c != face.b && face.c != face.a && face.c < vertFaceIds.size())
				vertFaceIds[face.c].Push(i);
		}
		return vertFaceIds;
	}

	std::vector<Stacktor<uint32_t, 3u>> GetMeshFaceNeighborIndices(const TriMesh::Reader& reader, bool connectCCwAndCwPairs) {
		const std::vector<Stacktor<uint32_t, 8u>> vertFaceIndices = GetMeshVertexFaceIndices(reader);
		std::vector<Stacktor<uint32_t, 3u>> faceNeighbors;
		faceNeighbors.resize(reader.FaceCount());
		for (size_t vId = 0u; vId < vertFaceIndices.size(); vId++) {
			const Stacktor<uint32_t, 8u>& vertFaces = vertFaceIndices[vId];
			for (size_t i = 0u; i < vertFaces.Size(); i++)
				for (size_t j = 0u; j < vertFaces.Size(); j++) {
					const uint32_t fA = vertFaces[i];
					const uint32_t fB = vertFaces[j];
					if (fA >= fB)
						continue;
					bool alreadyLinked = false;
					Stacktor<uint32_t, 3u>& neighborsOfA = faceNeighbors[fA];
					for (size_t nId = 0u; nId < neighborsOfA.Size(); nId++)
						if (neighborsOfA[nId] == fB) {
							alreadyLinked = true;
							break;
						}
					if (alreadyLinked)
						continue;
					if (!FindSharedEdgeIndex(reader.Face(fA), reader.Face(fB), connectCCwAndCwPairs).has_value())
						continue;
					neighborsOfA.Push(fB);
					faceNeighbors[fB].Push(fA);
				}
		}
		return faceNeighbors;
	}

	std::optional<std::pair<uint8_t, uint8_t>> FindSharedEdgeIndex(const TriangleFace& a, const TriangleFace& b, bool connectCCwAndCwPairs) {
		for (uint8_t i = uint8_t(0u); i < uint8_t(3u); i++) {
			const uint32_t vS = a[i];
			const uint32_t vE = a[(i + 1u) % 3u];
			for (uint8_t j = uint8_t(0u); j < uint8_t(3u); j++) {
				const uint8_t k = (j + 2u) % 3u;
				if (vS == b[j] && vE == b[k])
					return std::make_pair(i, k);
			}
		}
		if (connectCCwAndCwPairs) for (uint8_t i = uint8_t(0u); i < uint8_t(3u); i++) {
			const uint32_t vS = a[i];
			const uint32_t vE = a[(i + 1u) % 3u];
			for (uint8_t j = uint8_t(0u); j < uint8_t(3u); j++)
				if (vS == b[j] && vE == b[(j + 1u) % 3u])
					return std::make_pair(i, j);
		}
		return {};
	}
}
