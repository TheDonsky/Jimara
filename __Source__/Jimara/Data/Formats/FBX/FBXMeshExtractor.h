#pragma once
#include "FBXContent.h"
#include "../../Mesh.h"


namespace Jimara {
	namespace FBXHelpers {
		class FBXMeshExtractor {
		public:
			Reference<PolyMesh> ExtractMesh(const FBXContent::Node* objectNode, const std::string_view& name, OS::Logger* logger);

		private:
			struct VNUIndex {
				uint32_t vertexId = 0;
				uint32_t normalId = 0;
				uint32_t uvId = 0;

				inline bool operator<(const VNUIndex& other)const {
					return vertexId < other.vertexId || (vertexId == other.vertexId && (normalId < other.normalId || (normalId == other.normalId && uvId < other.uvId)));
				}
			};

			struct Index : public VNUIndex {
				uint32_t smoothId = 0;
				uint32_t nextIndexOnPoly = 0;

				inline static constexpr size_t NormalIdOffset() { Index index; return ((size_t)(&(index.normalId))) - ((size_t)(&index)); }
				inline static constexpr size_t SmoothIdOffset() { Index index; return ((size_t)(&(index.smoothId))) - ((size_t)(&index)); }
				inline static constexpr size_t UvIdOffset() { Index index; return ((size_t)(&(index.uvId))) - ((size_t)(&index)); }
			};

			std::vector<Vector3> m_nodeVertices;
			std::vector<Vector3> m_normals;
			std::vector<Vector2> m_uvs;
			std::vector<bool> m_smooth;
			std::vector<size_t> m_faceEnds;
			std::vector<Index> m_indices;
			std::vector<Stacktor<uint32_t, 4>> m_nodeEdges;
			std::vector<uint32_t> m_layerIndexBuffer;

			void Clear();
			bool ExtractVertices(const FBXContent::Node* objectNode, OS::Logger* logger);
			bool ExtractFaces(const FBXContent::Node* objectNode, OS::Logger* logger);
			
			template<size_t IndexOffset>
			inline bool ExtractLayerIndexInformation(
				const FBXContent::Node* layerElement, size_t layerElemCount,
				const std::string_view& layerElementName, const std::string_view& indexSubElementName, OS::Logger* logger);
			
			bool ExtractEdges(const FBXContent::Node* objectNode, OS::Logger* logger);
			bool ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger);
			bool ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger);
			bool ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger);
			bool FixNormals(OS::Logger* logger);
			Reference<PolyMesh> CreateMesh(const std::string_view& name);
		};
	}
}
