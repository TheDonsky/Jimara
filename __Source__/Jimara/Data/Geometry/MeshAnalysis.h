#pragma once
#include "Mesh.h"


namespace Jimara {
	/// <summary>
	/// For each vertex index, returns a list of faces indices that includes the vertex
	/// </summary>
	/// <param name="reader"> Mesh reader </param>
	/// <returns> Face-id buffer </returns>
	JIMARA_API std::vector<Stacktor<uint32_t, 8u>> GetMeshVertexFaceIndices(const TriMesh::Reader& reader);

	/// <summary>
	/// For each face, generates a list of other face indices that shat share an edge with it
	/// </summary>
	/// <param name="reader"> Mesh reader </param>
	/// <param name="connectCCwAndCwPairs"> If true, pairs of clockwise and counter-clockwise triangles will also be reported </param>
	/// <returns> For each face, a list of 'neighboring' face indices </returns>
	JIMARA_API std::vector<Stacktor<uint32_t, 3u>> GetMeshFaceNeighborIndices(const TriMesh::Reader& reader, bool connectCCwAndCwPairs = false);

	/// <summary>
	/// Tries to find an index of an edge the two triangle faces share
	/// <para/> 'Edge index i' means Edge 'mesh.vert(face[i]) and mesh.vert(face[(i + 1u) % 3u]);
	/// </summary>
	/// <param name="a"> First face </param>
	/// <param name="b"> Second face </param>
	/// <param name="connectCCwAndCwPairs"> If true, pairs of clockwise and counter-clockwise triangles will also be reported </param>
	/// <returns> Common edge indices </returns>
	JIMARA_API std::optional<std::pair<uint8_t, uint8_t>> FindSharedEdgeIndex(const TriangleFace& a, const TriangleFace& b, bool connectCCwAndCwPairs = false);
}
