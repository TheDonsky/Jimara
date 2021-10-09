#pragma once
#include "FBXObjectIndex.h"
#include "../../Mesh.h"


namespace Jimara {
	namespace FBXHelpers {
		/// <summary>
		/// Utility for extracting mesh data from an FBX node.
		/// Note: Objects of this class are meant to be used for optimal memory management
		/// </summary>
		class FBXMeshExtractor {
		public:
			/// <summary>
			/// Attempts to extract a polygonal mesh from a FBX node
			/// </summary>
			/// <param name="objectNode"> Object node to extract Mesh from (should be a "Geometry" type) </param>
			/// <param name="logger"> Logger for error/warning reporting </param>
			/// <returns> Polygonal mesh if successful, nullptr otherwise </returns>
			Reference<PolyMesh> ExtractMesh(const FBXObjectNode& objectNode, OS::Logger* logger);



		private:
			// Vertex-Normal-UV polygon vertex index:
			struct VNUIndex {
				uint32_t vertexId = 0;
				uint32_t normalId = 0;
				uint32_t uvId = 0;

				inline bool operator<(const VNUIndex& other)const {
					return vertexId < other.vertexId || (vertexId == other.vertexId && (normalId < other.normalId || (normalId == other.normalId && uvId < other.uvId)));
				}
			};

			// Full index with smoothing and everything per polygon vertex
			struct Index : public VNUIndex {
				uint32_t smoothId = 0;
				uint32_t nextIndexOnPoly = 0;

				inline static constexpr size_t NormalIdOffset() { Index index; return ((size_t)(&(index.normalId))) - ((size_t)(&index)); }
				inline static constexpr size_t SmoothIdOffset() { Index index; return ((size_t)(&(index.smoothId))) - ((size_t)(&index)); }
				inline static constexpr size_t UvIdOffset() { Index index; return ((size_t)(&(index.uvId))) - ((size_t)(&index)); }
			};

			// Vertex position buffer
			std::vector<Vector3> m_nodeVertices;

			// Vertex normal buffer
			std::vector<Vector3> m_normals;

			// Vertex UV buffer
			std::vector<Vector2> m_uvs;

			// Smoothing status buffer (Filled in ExtractSmoothing(); used from FixNormals() call)
			std::vector<bool> m_smooth;

			// m_indices element indexes for face ends (ei if first 4 m_indices make a quad, the first element of m_faceEnds will be 4 and etc)
			std::vector<size_t> m_faceEnds;

			// Polygon vertex index data
			std::vector<Index> m_indices;

			// Each entry describes an edge by storing all polygon vertex indices
			std::vector<Stacktor<uint32_t, 4>> m_nodeEdges;

			// Arbitrary list index buffer for storing some temporary data
			std::vector<uint32_t> m_layerIndexBuffer;


			// Here are the steps, more or less in chronological order, as of how the mesh data is extracted:
			
			// 0. First step is to clear all of the buffers described above;
			void Clear();

			// 1. Next, we extract vertex positions from 'Vertices' sub-node from objectNode tree;
			bool ExtractVertices(const FBXContent::Node* objectNode, OS::Logger* logger);

			// 2. After that we extract faces from 'PolygonVertexIndex' sub-node and fill m_indices and m_faceEnds accordingly (note that only vertexId-s and nextIndexOnPoly will be filled up to this point);
			bool ExtractFaces(const FBXContent::Node* objectNode, OS::Logger* logger);

			// 3. Logically, here comes the extraction of the optional 'Edges' data;
			bool ExtractEdges(const FBXContent::Node* objectNode, OS::Logger* logger);

			// This one is a helper that will automagically fill in m_indices data like normalId, uvId or smoothId
			inline bool ExtractLayerIndexInformation(
				char* indexBuffer,
				const FBXContent::Node* layerElement, size_t layerElemCount,
				const std::string_view& layerElementName, const std::string_view& indexSubElementName, OS::Logger* logger);
			
			// 4. This one extracts normals from the lowest 'LayerElementNormal' layer (others are ignored since PolyMesh does not currently support arbitrary amount of layers);
			bool ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger);

			// 5. This one extracts smoothing flags from the lowest 'LayerElementSmoothing' layer (others are ignored since PolyMesh does not currently support arbitrary amount of layers);
			bool ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger);

			// 6. This one extracts UV coordinates from the lowest 'LayerElementUV' layer (others are ignored since PolyMesh does not currently support arbitrary amount of layers);
			bool ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger);

			// 7. Normals may be missing or smoothing flags may require to merge some. This method takes the responsibility to merge and/or generate normals when needed;
			bool FixNormals(OS::Logger* logger);

			// 8. Final step is to actually create a mesh from the buffers we've spent time carefully filling up.
			Reference<PolyMesh> CreateMesh(const std::string_view& name);
		};
	}
}
