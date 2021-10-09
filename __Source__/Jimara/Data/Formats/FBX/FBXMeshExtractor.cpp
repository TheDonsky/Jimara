#include "FBXMeshExtractor.h"
#include <map>

namespace Jimara {
	namespace FBXHelpers {
		namespace {
			template<typename ReturnType, typename... MessageTypes>
			inline static ReturnType Error(OS::Logger* logger, const ReturnType& returnValue, const MessageTypes&... message) {
				if (logger != nullptr) logger->Error(message...);
				return returnValue;
			}

			
		}

		Reference<PolyMesh> FBXMeshExtractor::ExtractMesh(const FBXObjectIndex::NodeWithConnections& objectNode, OS::Logger* logger) {
			auto error = [&](auto... message) ->Reference<PolyMesh> { return Error<Reference<PolyMesh>>(logger, nullptr, message...); };
			if (objectNode.node.Node() == nullptr) return error("FBXMeshExtractor::ExtractMesh - null Node provided!");
			else if (objectNode.node.NodeAttribute() != "Geometry") { if (logger != nullptr) logger->Warning("FBXMeshExtractor::ExtractMesh - Object not not named 'Geometry'!"); }
			if (objectNode.node.Class() != objectNode.node.NodeAttribute()) {
				if (logger != nullptr) logger->Warning("FBXMeshExtractor::ExtractMesh - Class(from Name::Class)<'", objectNode.node.Class(), "'> is not not '", objectNode.node.NodeAttribute(), "'!");
			}
			else if (objectNode.node.SubClass() != "Mesh") return error("FBXMeshExtractor::ExtractMesh - Sub-Class<'", objectNode.node.SubClass(), "'> is not not 'Mesh'!");
			Clear();
			if (!ExtractVertices(objectNode.node.Node(), logger)) return nullptr;
			else if (!ExtractFaces(objectNode.node.Node(), logger)) return nullptr;
			else if (!ExtractEdges(objectNode.node.Node(), logger)) return nullptr;
			std::pair<const FBXContent::Node*, int64_t> normalLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
			std::pair<const FBXContent::Node*, int64_t> smoothingLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
			std::pair<const FBXContent::Node*, int64_t> uvLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
			for (size_t i = 0; i < objectNode.node.Node()->NestedNodeCount(); i++) {
				const FBXContent::Node* layerNode = &objectNode.node.Node()->NestedNode(i);
				const std::string_view elementName = layerNode->Name();
				std::pair<const FBXContent::Node*, int64_t>* layerElementToReplace = (
					(elementName == "LayerElementNormal") ? (&normalLayerElement) :
					(elementName == "LayerElementSmoothing") ? (&normalLayerElement) :
					(elementName == "LayerElementUV") ? (&uvLayerElement) : nullptr);
				if (layerElementToReplace == nullptr) continue;
				else if (layerNode->PropertyCount() <= 0) return error("FBXMeshExtractor::ExtractData - Layer element does not have layer id!");
				const FBXContent::Property& layerIdProp = layerNode->NodeProperty(0);
				int64_t layerIndex;
				if (layerIdProp.Type() == FBXContent::PropertyType::INT_64) layerIndex = static_cast<int64_t>(layerIdProp.operator int64_t());
				else if (layerIdProp.Type() == FBXContent::PropertyType::INT_32) layerIndex = static_cast<int64_t>(layerIdProp.operator int32_t());
				else if (layerIdProp.Type() == FBXContent::PropertyType::INT_16) layerIndex = static_cast<int64_t>(layerIdProp.operator int16_t());
				else return error("FBXMeshExtractor::ExtractData - Layer element does not have an integer layer id!");
				bool hasAnotherLayer = (layerElementToReplace->first != nullptr);
				if ((!hasAnotherLayer) || layerElementToReplace->second > layerIndex)
					(*layerElementToReplace) = std::make_pair(layerNode, layerIndex);
				else hasAnotherLayer = true;
				if (hasAnotherLayer && logger != nullptr)
					logger->Warning("FBXMeshExtractor::ExtractData - Multiple layer elements<", elementName, "> not [currently] supported...");
			}
			if (!ExtractNormals(normalLayerElement.first, logger)) return nullptr;
			else if (!ExtractSmoothing(smoothingLayerElement.first, logger)) return nullptr;
			else if (!ExtractUVs(uvLayerElement.first, logger)) return nullptr;
			else if (!FixNormals(logger)) return nullptr;
			else return CreateMesh(objectNode, logger);
		}

		void FBXMeshExtractor::Clear() {
			m_nodeVertices.clear();
			m_normals.clear();
			m_uvs.clear();
			m_smooth.clear();
			m_indices.clear();
			m_faceEnds.clear();
			m_nodeEdges.clear();
		}

		bool FBXMeshExtractor::ExtractVertices(const FBXContent::Node* objectNode, OS::Logger* logger) {
			const FBXContent::Node* verticesNode = objectNode->FindChildNodeByName("Vertices", 0);
			if (verticesNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractVertices - Vertices node missing!");
			else if (verticesNode->PropertyCount() >= 1)
				if (!verticesNode->NodeProperty(0).Fill(m_nodeVertices, true))
					return Error(logger, false, "FBXMeshExtractor::ExtractVertices - Vertices node invalid!");
			return true;
		}

		bool FBXMeshExtractor::ExtractFaces(const FBXContent::Node* objectNode, OS::Logger* logger) {
			const FBXContent::Node* indicesNode = objectNode->FindChildNodeByName("PolygonVertexIndex", 0);
			if (indicesNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractFaces - Indices node missing!");
			else if (indicesNode->PropertyCount() >= 1) {
				const FBXContent::Property& prop = indicesNode->NodeProperty(0);
				const size_t vertexCount = m_nodeVertices.size();
				auto addNodeIndex = [&](int32_t value) -> bool {
					auto pushIndex = [&](uint32_t index) -> bool {
						if (index >= vertexCount) return Error(logger, false, "FBXMeshExtractor::ExtractFaces - Vertex index overflow!");
						Index id;
						id.vertexId = index;
						id.nextIndexOnPoly = static_cast<uint32_t>(m_indices.size()) + 1u;
						m_indices.push_back(id);
						return true;
					};
					if (value < 0) {
						if (!pushIndex(static_cast<uint32_t>(value ^ int32_t(-1)))) return false;
						m_indices.back().nextIndexOnPoly = static_cast<uint32_t>((m_faceEnds.size() <= 0) ? 0 : m_faceEnds.back());
						m_faceEnds.push_back(m_indices.size());
						return true;
					}
					else return pushIndex(static_cast<uint32_t>(value));
				};
				if (prop.Type() == FBXContent::PropertyType::INT_32_ARR) {
					for (size_t i = 0; i < prop.Count(); i++)
						if (!addNodeIndex(prop.Int32Elem(i))) return false;
				}
				else if (prop.Type() == FBXContent::PropertyType::INT_64_ARR) {
					for (size_t i = 0; i < prop.Count(); i++) {
						int64_t value = prop.Int64Elem(i);
						int32_t truncatedValue = static_cast<int32_t>(value);
						if (value != truncatedValue) return Error(logger, false, "FBXMeshExtractor::ExtractFaces - Face index can not fit into an int32_t value!");
						else if (!addNodeIndex(truncatedValue)) return false;
					}
				}
				else return Error(logger, false, "FBXMeshExtractor::ExtractFaces - Indices does not have an integer array property where expected!");
				if ((m_indices.size() > 0) && ((m_faceEnds.size() <= 0) || (m_faceEnds.back() != m_indices.size()))) {
					if (logger != nullptr) logger->Warning("FBXMeshExtractor::ExtractFaces - Last index not negative... Pretending as if it was xor-ed with -1...");
					m_indices.back().nextIndexOnPoly = static_cast<uint32_t>((m_faceEnds.size() <= 0) ? 0 : m_faceEnds.back());
					m_faceEnds.push_back(m_indices.size());
				}
			}
			return true;
		}

		bool FBXMeshExtractor::ExtractEdges(const FBXContent::Node* objectNode, OS::Logger* logger) {
			const FBXContent::Node* verticesNode = objectNode->FindChildNodeByName("Edges", 0);
			if (verticesNode == nullptr) return true;
			m_layerIndexBuffer.clear();
			if (verticesNode->PropertyCount() >= 1)
				if (!verticesNode->NodeProperty(0).Fill(m_layerIndexBuffer, true))
					return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - Edges buffer invalid!");
			if (m_indices.size() > 1) {
				for (size_t i = 0; i < m_layerIndexBuffer.size(); i++) {
					if (m_layerIndexBuffer[i] >= m_indices.size())
						return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - Edge value exceeds maximal valid edge index!");
				}
				auto getEdgeKey = [&](uint32_t polyVertId) -> uint64_t {
					const Index& index = m_indices[polyVertId];
					uint64_t vertexA = static_cast<uint64_t>(index.vertexId);
					uint64_t vertexB = static_cast<uint64_t>(m_indices[index.nextIndexOnPoly].vertexId);
					if (vertexA > vertexB) std::swap(vertexA, vertexB);
					return ((vertexB << 32) | vertexA);
				};
				m_nodeEdges.resize(m_layerIndexBuffer.size());
				std::unordered_map<uint64_t, size_t> edgeIndex;
				for (size_t i = 0; i < m_layerIndexBuffer.size(); i++)
					edgeIndex[getEdgeKey(m_layerIndexBuffer[i])] = i;
				bool edgeSetIncomplete = false;
				for (uint32_t i = 0; i < m_indices.size(); i++) {
					std::unordered_map<uint64_t, size_t>::const_iterator it = edgeIndex.find(getEdgeKey(i));
					if (it == edgeIndex.end()) {
						edgeSetIncomplete = true;
						continue;
					}
					Stacktor<uint32_t, 4>& edgeIndices = m_nodeEdges[it->second];
					edgeIndices.Push(i);
					edgeIndices.Push(m_indices[i].nextIndexOnPoly);
				}
				if (edgeSetIncomplete && logger != nullptr)
					logger->Warning("BXData::FBXMeshExtractor::ExtractEdges - Edge set incomplete!");
			}
			else if (m_layerIndexBuffer.size() > 0)
				return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - We have less than 2 indices and therefore, can not have any edges!");
			return true;
		}

		inline bool FBXMeshExtractor::ExtractLayerIndexInformation(
			char* indexBuffer,
			const FBXContent::Node* layerElement, size_t layerElemCount,
			const std::string_view& layerElementName, const std::string_view& indexSubElementName, OS::Logger* logger) {
			
			auto findInformationType = [&](const std::string_view& name) -> const FBXContent::Property* {
				const FBXContent::Node* informationTypeNode = layerElement->FindChildNodeByName(name);
				if (informationTypeNode == nullptr)
					return Error(logger, nullptr, "FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node missing for ", layerElementName, "!");
				else if (informationTypeNode->PropertyCount() <= 0)
					return Error(logger, nullptr, "FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node has no value for ", layerElementName, "!");
				const FBXContent::Property* informationTypeProp = &informationTypeNode->NodeProperty(0);
				if (informationTypeProp->Type() != FBXContent::PropertyType::STRING)
					return Error(logger, nullptr, "FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " not string for ", layerElementName, "!");
				return informationTypeProp;
			};

			// ReferenceInformationType:
			const FBXContent::Property* referenceInformationTypeProp = findInformationType("ReferenceInformationType");
			if (referenceInformationTypeProp == nullptr) return false;
			const std::string_view referenceInformationType = *referenceInformationTypeProp;

			// Index:
			m_layerIndexBuffer.clear();
			if (referenceInformationType == "Direct") {
				for (uint32_t i = 0; i < layerElemCount; i++) m_layerIndexBuffer.push_back(i);
			}
			else if (referenceInformationType == "IndexToDirect" || referenceInformationType == "Index") {
				const FBXContent::Node* indexNode = layerElement->FindChildNodeByName(indexSubElementName);
				if (indexNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " node missing!");
				else if (indexNode->PropertyCount() <= 0) return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " has no values!");
				else if (!indexNode->NodeProperty(0).Fill(m_layerIndexBuffer, true)) return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " node invalid!");
				else for (size_t i = 0; i < m_layerIndexBuffer.size(); i++)
					if (m_layerIndexBuffer[i] >= layerElemCount)
						return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - ",
							indexSubElementName, " contains indices greater than or equal to ", indexSubElementName, ".size()<", layerElemCount, ">!");
			}
			else return Error(logger, false,
				"FBXMeshExtractor::ExtractLayerIndexInformation - ReferenceInformationType<", referenceInformationType, "> not supported for ", layerElementName, "!");

			// MappingInformationType:
			const FBXContent::Property* mappingInformationTypeProp = findInformationType("MappingInformationType");
			if (mappingInformationTypeProp == nullptr) return false;
			const std::string_view mappingInformationType = *mappingInformationTypeProp;

			// Fill m_indices:
			const size_t indexCount = m_layerIndexBuffer.size();
			auto indexValue = [&](size_t index) -> uint32_t& {
				return *reinterpret_cast<uint32_t*>(indexBuffer + (sizeof(Index) * index));
			};
			if (mappingInformationType == "ByVertex" || mappingInformationType == "ByVertice") {
				if (indexCount != m_nodeVertices.size())
					return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the vertex count!");
				for (size_t i = 0; i < m_indices.size(); i++)
					indexValue(i) = m_layerIndexBuffer[m_indices[i].vertexId];
			}
			else if (mappingInformationType == "ByPolygonVertex") {
				if (indexCount != m_indices.size())
					return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon vertex count!");
				for (size_t i = 0; i < m_indices.size(); i++)
					indexValue(i) = m_layerIndexBuffer[i];
			}
			else if (mappingInformationType == "ByPolygon") {
				if (indexCount != m_faceEnds.size())
					return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon count!");
				size_t faceId = 0;
				for (size_t i = 0; i < m_indices.size(); i++) {
					indexValue(i) = m_layerIndexBuffer[faceId];
					if (i == m_faceEnds[faceId]) faceId++;
				}
			}
			else if (mappingInformationType == "ByEdge") {
				if (indexCount != m_nodeEdges.size())
					return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the edge count!");
				for (size_t i = 0; i < m_indices.size(); i++)
					indexValue(i) = static_cast<uint32_t>(layerElemCount);
				for (size_t i = 0; i < m_nodeEdges.size(); i++) {
					const Stacktor<uint32_t, 4>& edgeIndices = m_nodeEdges[i];
					uint32_t value = m_layerIndexBuffer[i];
					for (size_t j = 0; j < edgeIndices.Size(); j++)
						indexValue(edgeIndices[j]) = value;
				}
				for (size_t i = 0; i < m_indices.size(); i++)
					if (indexValue(i) >= layerElemCount)
						return Error(logger, false,
							"FBXMeshExtractor::ExtractLayerIndexInformation - Edges do not cover all indices and can not be used for layer elements for ", layerElementName, "!");
				if (logger != nullptr)
					logger->Warning("FBXMeshExtractor::ExtractLayerIndexInformation - ", layerElementName, " layer was set 'ByEdge'; not sure if the interpretation is correct...");
			}
			else if (mappingInformationType == "AllSame") {
				if (layerElemCount <= 0)
					return Error(logger, false, "FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " is zero!");
				for (size_t i = 0; i < m_indices.size(); i++)
					indexValue(i) = 0;
			}
			else return Error(logger, false,
				"FBXMeshExtractor::ExtractLayerIndexInformation - MappingInformationType<", mappingInformationType, "> not supported for ", layerElementName, "!");
			return true;
		}

		// One of the compilers will hate me if I don't do this...
#ifdef _WIN32
#define NORMAL_ID_OFFSET offsetof(Index, normalId)
#define SMOOTH_ID_OFFSET offsetof(Index, smoothId)
#define UV_ID_OFFSET offsetof(Index, uvId)
#else
#define NORMAL_ID_OFFSET Index::NormalIdOffset()
#define SMOOTH_ID_OFFSET Index::SmoothIdOffset()
#define UV_ID_OFFSET Index::UvIdOffset()
#endif

		bool FBXMeshExtractor::ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger) {
			if (layerElement == nullptr) return true;
			const FBXContent::Node* normalsNode = layerElement->FindChildNodeByName("Normals");
			if (normalsNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractNormals - Normals node missing!");
			else if (normalsNode->PropertyCount() >= 1)
				if (!normalsNode->NodeProperty(0).Fill(m_normals, true))
					return Error(logger, false, "FBXMeshExtractor::ExtractNormals - Normals node invalid!");
			const Index index;
			const size_t OFFSET = ((char*)(&index.normalId) - (char*)(&index));
			return ExtractLayerIndexInformation(reinterpret_cast<char*>(m_indices.data()) + NORMAL_ID_OFFSET, layerElement, m_normals.size(), "Normals", "NormalsIndex", logger);
		}

		bool FBXMeshExtractor::ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger) {
			if (layerElement == nullptr) return true;
			const FBXContent::Node* smoothingNode = layerElement->FindChildNodeByName("Smoothing");
			if (smoothingNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractSmoothing - Smoothing node missing!");
			else if (smoothingNode->PropertyCount() >= 1)
				if (!smoothingNode->NodeProperty(0).Fill(m_smooth, true))
					return Error(logger, false, "FBXMeshExtractor::ExtractSmoothing - Smoothing node invalid!");
			return ExtractLayerIndexInformation(reinterpret_cast<char*>(m_indices.data()) + SMOOTH_ID_OFFSET, layerElement, m_smooth.size(), "Smoothing", "SmoothingIndex", logger);
		}

		bool FBXMeshExtractor::ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger) {
			if (layerElement == nullptr) {
				m_uvs = { Vector2(0.0f) };
				return true;
			}
			const FBXContent::Node* uvNode = layerElement->FindChildNodeByName("UV");
			if (uvNode == nullptr) return Error(logger, false, "FBXMeshExtractor::ExtractUVs - UV node missing!");
			else if (uvNode->PropertyCount() >= 1)
				if (!uvNode->NodeProperty(0).Fill(m_uvs, true))
					return Error(logger, false, "FBXMeshExtractor::ExtractUVs - UV node invalid!");
			if (!ExtractLayerIndexInformation(reinterpret_cast<char*>(m_indices.data()) + UV_ID_OFFSET, layerElement, m_uvs.size(), "UV", "UVIndex", logger)) return false;
			else if (m_uvs.size() <= 0) m_uvs.push_back(Vector2(0.0f));
			return true;
		}

#undef NORMAL_ID_OFFSET
#undef SMOOTH_ID_OFFSET
#undef UV_ID_OFFSET

		bool FBXMeshExtractor::FixNormals(OS::Logger* logger) {
			// Make sure there are normals for each face vertex:
			bool noNormals = (m_normals.size() <= 0);
			if (noNormals && m_faceEnds.size() > 0 && m_faceEnds[0] > 0) {
				size_t faceId = 0;
				size_t prev = m_faceEnds[faceId] - 1;
				for (size_t i = 0; i < m_indices.size(); i++) {
					if (i >= m_faceEnds[faceId]) {
						faceId++;
						prev = m_faceEnds[faceId] - 1;
					}
					Index& index = m_indices[i];
					const Vector3 o = m_nodeVertices[index.vertexId];
					const Vector3 a = m_nodeVertices[m_indices[index.nextIndexOnPoly].vertexId];
					const Vector3 b = m_nodeVertices[m_indices[prev].vertexId];
					const Vector3 cross = Math::Cross(a - o, b - o);
					const float crossSqrMagn = Math::Dot(cross, cross);
					index.normalId = static_cast<uint32_t>(m_normals.size());
					m_normals.push_back(crossSqrMagn <= 0.0f ? Vector3(0.0f) : (cross / std::sqrt(crossSqrMagn)));
					prev = i;
				}
			}

			// Make sure each face vertex has a smoothing value:
			bool hasSmoothing = false;
			if (m_smooth.size() <= 0) {
				m_smooth.push_back(noNormals);
				for (size_t i = 0; i < m_indices.size(); i++)
					m_indices[i].smoothId = 0;
				hasSmoothing = noNormals;
			}
			else for (size_t i = 0; i < m_indices.size(); i++)
				if (m_smooth[m_indices[i].smoothId]) {
					hasSmoothing = true;
					break;
				}

			// If there are smooth vertices, their normals should be averaged and merged:
			if (!hasSmoothing) return true;
			size_t desiredNormalCount = m_normals.size() + m_nodeVertices.size();
			if (desiredNormalCount > UINT32_MAX) return Error(logger, false, "FBXMeshExtractor::ExtractSmoothing - Too many normals!");
			const uint32_t baseSmoothNormal = static_cast<uint32_t>(m_normals.size());
			for (size_t i = 0; i < m_nodeVertices.size(); i++)
				m_normals.push_back(Vector3(0.0f));
			for (size_t i = 0; i < m_indices.size(); i++) {
				Index& index = m_indices[i];
				if (!m_smooth[index.smoothId]) continue;
				uint32_t vertexId = index.vertexId;
				uint32_t normalIndex = baseSmoothNormal + vertexId;
				m_normals[normalIndex] += m_normals[index.normalId];
				index.normalId = normalIndex;
			}
			for (size_t i = 0; i < m_nodeVertices.size(); i++) {
				Vector3& normal = m_normals[static_cast<size_t>(baseSmoothNormal) + m_indices[i].vertexId];
				const float sqrMagn = Math::Dot(normal, normal);
				if (sqrMagn > 0.0f) normal /= std::sqrt(sqrMagn);
			}
			return true;
		}

		namespace {
			bool ExtractSkinData(FBXSkinDataExtractor& skinDataExtractor, const FBXObjectIndex::NodeWithConnections& meshNode, OS::Logger* logger) {
				for (size_t i = 0; i < meshNode.childConnections.Size(); i++)
					if (FBXSkinDataExtractor::IsSkin(meshNode.childConnections[i].connection))
						if (skinDataExtractor.Extract(*meshNode.childConnections[i].connection, logger)) return true;
				return false;
			}
		}

		Reference<PolyMesh> FBXMeshExtractor::CreateMesh(const FBXObjectIndex::NodeWithConnections& objectNode, OS::Logger* logger) {
			std::map<VNUIndex, uint32_t> vertexIndexMap;
			Reference<PolyMesh> mesh;
			if (ExtractSkinData(m_skinDataExtractor, objectNode, logger)) 
				mesh = Object::Instantiate<SkinnedPolyMesh>(objectNode.node.Name());
			else mesh = Object::Instantiate<PolyMesh>(objectNode.node.Name());
			PolyMesh::Writer writer(mesh);
			for (size_t faceId = 0; faceId < m_faceEnds.size(); faceId++) {
				const size_t faceEnd = m_faceEnds[faceId];
				if (faceEnd <= 0 || (faceId > 0 && m_faceEnds[faceId] <= m_faceEnds[faceId - 1])) continue;
				writer.AddFace(PolygonFace());
				PolygonFace& face = writer.Face(writer.FaceCount() - 1);
				for (size_t i = m_indices[faceEnd - 1].nextIndexOnPoly; i < faceEnd; i++) {
					const VNUIndex& index = m_indices[i];
					uint32_t vertexIndex;
					{
						std::map<VNUIndex, uint32_t>::const_iterator it = vertexIndexMap.find(index);
						if (it == vertexIndexMap.end()) {
							vertexIndex = static_cast<uint32_t>(writer.VertCount());
							vertexIndexMap[index] = vertexIndex;
							const Vector3 vertex = m_nodeVertices[index.vertexId];
							const Vector3 normal = m_normals[index.normalId];
							const Vector2 uv = m_uvs[index.uvId];
							writer.AddVert(MeshVertex(Vector3(vertex.x, vertex.y, -vertex.z), Vector3(normal.x, normal.y, -normal.z), Vector2(uv.x, 1.0f - uv.y)));
						}
						else vertexIndex = it->second;
					}
					face.Push(vertexIndex);
				}
			}
			return mesh;
		}
	}
}
