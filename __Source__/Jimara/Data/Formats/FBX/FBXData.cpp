#include "FBXData.h"
#include <stddef.h>
#include <unordered_map>


namespace Jimara {
	namespace {
		// Some meaningless helper for 
		template<typename ReturnType, typename... MessageTypes>
		inline static ReturnType Error(OS::Logger* logger, const ReturnType& returnValue, const MessageTypes&... message) {
			if (logger != nullptr) logger->Error(message...);
			return returnValue;
		}

		// Just a handy utility to find child nodes by name:
		inline static const FBXContent::Node* FindChildNode(const FBXContent::Node* parentNode, const std::string_view& childName, size_t expectedIndex = 0) {
			if (parentNode == nullptr) return nullptr;
			size_t nestedNodeCount = parentNode->NestedNodeCount();
			if (nestedNodeCount <= 0) return nullptr;
			expectedIndex %= nestedNodeCount;
			size_t i = expectedIndex;
			while (true) {
				const FBXContent::Node* childNode = &parentNode->NestedNode(i);
				if (childNode->Name() == childName) return childNode;
				i++;
				if (i == expectedIndex) return nullptr;
				else if (i >= nestedNodeCount) i -= nestedNodeCount;
			}
		}
		template<typename... MessageTypes>
		inline static const FBXContent::Node* FindChildNode(
			const FBXContent::Node* parentNode, const std::string_view& childName, size_t expectedIndex, OS::Logger* logger, const MessageTypes&... warningMessage) {
			const FBXContent::Node* node = FindChildNode(parentNode, childName, expectedIndex);
			if (node == nullptr && logger != nullptr) logger->Warning(warningMessage...);
			return node;
		}

		// Utilities to fill arrays:
		template<typename Type, typename ArrayType>
		inline static Type ExtractVectorElement(const FBXContent::Property&, size_t) { static_assert(false); }
		template<>
		inline static Vector3 ExtractVectorElement<Vector3, float>(const FBXContent::Property& prop, size_t startIndex) {
			return Vector3(prop.Float32Elem(startIndex), prop.Float32Elem(startIndex + 1), prop.Float32Elem(startIndex + 2));
		}
		template<>
		inline static Vector3 ExtractVectorElement<Vector3, double>(const FBXContent::Property& prop, size_t startIndex) {
			return Vector3(static_cast<float>(prop.Float64Elem(startIndex)), static_cast<float>(prop.Float64Elem(startIndex + 1)), static_cast<float>(prop.Float64Elem(startIndex + 2)));
		}
		template<>
		inline static Vector2 ExtractVectorElement<Vector2, float>(const FBXContent::Property& prop, size_t startIndex) {
			return Vector2(prop.Float32Elem(startIndex), prop.Float32Elem(startIndex + 1));
		}
		template<>
		inline static Vector2 ExtractVectorElement<Vector2, double>(const FBXContent::Property& prop, size_t startIndex) {
			return Vector2(static_cast<float>(prop.Float64Elem(startIndex)), static_cast<float>(prop.Float64Elem(startIndex + 1)));
		}

		template<typename Type>
		inline static constexpr size_t VectorDimms() { static_assert(false); }
		template<>
		inline static constexpr size_t VectorDimms<Vector3>() { return 3; }
		template<>
		inline static constexpr size_t VectorDimms<Vector2>() { return 2; }

		template<typename VectorType>
		inline static bool FillVectorBuffer(const FBXContent::Property& prop, const std::string_view& propertyName, std::vector<VectorType>& buffer, OS::Logger* logger) {
			if (prop.Type() != FBXContent::PropertyType::FLOAT_32_ARR && prop.Type() != FBXContent::PropertyType::FLOAT_64_ARR)
				return Error(logger, false, "FBXData::Extract::FillVectorBuffer - property<", propertyName, "> is not a floating point array!");
			else if ((prop.Count() % VectorDimms<VectorType>()) != 0)
				return Error(logger, false, "FBXData::Extract::FillVectorBuffer - property<", propertyName, "> element count is not a multiple of ", VectorDimms<VectorType>(), "!");
			else if (prop.Type() == FBXContent::PropertyType::FLOAT_32_ARR) {
				for (size_t i = 0; i < prop.Count(); i += VectorDimms<VectorType>())
					buffer.push_back(ExtractVectorElement<VectorType, float>(prop, i));
			}
			else if (prop.Type() == FBXContent::PropertyType::FLOAT_64_ARR) {
				for (size_t i = 0; i < prop.Count(); i += VectorDimms<VectorType>())
					buffer.push_back(ExtractVectorElement<VectorType, double>(prop, i));
			}
			else return Error(logger, false, "FBXData::Extract::FillVectorBuffer - Internal error!");
			return true;
		};

		inline static bool FillUint32Buffer(const FBXContent::Property& prop, const std::string_view& propertyName, std::vector<uint32_t>& buffer, OS::Logger* logger) {
			auto negative = [&]() -> bool {
				return Error(logger, false, "FBXData::Extract::FillUint32Buffer - property<", propertyName, "> negative and can not be represented as an unsigned integer!");
			};
			if (prop.Type() == FBXContent::PropertyType::INT_32_ARR) {
				for (size_t i = 0; i < prop.Count(); i++) {
					int32_t value = prop.Int32Elem(i);
					if (value < 0) return negative();
					else buffer.push_back(static_cast<uint32_t>(value));
				}
			}
			else if (prop.Type() == FBXContent::PropertyType::INT_64_ARR) {
				for (size_t i = 0; i < prop.Count(); i++) {
					int64_t value = prop.Int64Elem(i);
					int32_t truncatedValue = static_cast<int32_t>(value);
					if (truncatedValue != value) return Error(logger, false, "FBXData::Extract::FillUint32Buffer - property<", propertyName, "> element does not fit into a 32 bit integer!");
					else if (truncatedValue < 0) return negative();
					else buffer.push_back(static_cast<uint32_t>(truncatedValue));
				}
			}
			else return Error(logger, false, "FBXData::Extract::FillUint32Buffer - property<", propertyName, "> is not an integer array!");
			return true;
		}

		inline static bool FillBoolBuffer(const FBXContent::Property& prop, const std::string_view& propertyName, std::vector<bool>& buffer, OS::Logger* logger) {
			if (prop.Type() == FBXContent::PropertyType::BOOLEAN_ARR) {
				for (size_t i = 0; i < prop.Count(); i++)
					buffer.push_back(prop.BoolElem(i));
			}
			else if (prop.Type() == FBXContent::PropertyType::INT_32_ARR) {
				for (size_t i = 0; i < prop.Count(); i++)
					buffer.push_back(prop.Int32Elem(i) != 0);
			}
			else if (prop.Type() == FBXContent::PropertyType::INT_64_ARR) {
				for (size_t i = 0; i < prop.Count(); i++)
					buffer.push_back(prop.Int64Elem(i) != 0);
			}
			else return Error(logger, false, "FBXData::Extract::FillBoolBuffer - property<", propertyName, "> can not be interpreted as a boolean array!");
			return true;
		}

		// Reads mesh from FBXContent::Node
		class FBXMeshExtractor {
		private:
			struct VNUIndex {
				uint32_t vertexId = 0;
				uint32_t normalId = 0;
				uint32_t uvId = 0;
			};

			struct Index : public VNUIndex {
				uint32_t smoothId = 0;
				uint32_t nextIndexOnPoly = 0;
			};

			std::vector<Vector3> m_nodeVertices;
			std::vector<Vector3> m_normals;
			std::vector<Vector2> m_uvs;
			std::vector<bool> m_smooth;
			std::vector<size_t> m_faceEnds;
			std::vector<Index> m_indices;
			std::vector<Stacktor<uint32_t, 4>> m_nodeEdges;
			std::vector<uint32_t> m_layerIndexBuffer;

			inline void Clear() {
				m_nodeVertices.clear();
				m_normals.clear();
				m_uvs.clear();
				m_smooth.clear();
				m_indices.clear();
				m_faceEnds.clear();
				m_nodeEdges.clear();
			}

			inline bool ExtractVertices(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* verticesNode = FindChildNode(objectNode, "Vertices", 0);
				if (verticesNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractVertices - Vertices node missing!");
				else if (verticesNode->PropertyCount() >= 1)
					return FillVectorBuffer(verticesNode->NodeProperty(0), "FBXData::Objects::Geometry::Vertices", m_nodeVertices, logger);
				return true;
			}

			inline bool ExtractFaces(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* indicesNode = FindChildNode(objectNode, "PolygonVertexIndex", 0);
				if (indicesNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Indices node missing!");
				else if (indicesNode->PropertyCount() >= 1) {
					const FBXContent::Property& prop = indicesNode->NodeProperty(0);
					const size_t vertexCount = m_nodeVertices.size();
					auto addNodeIndex = [&](int32_t value) -> bool {
						auto pushIndex = [&](uint32_t index) -> bool {
							if (index >= vertexCount) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Vertex index overflow!");
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
							if (value != truncatedValue) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Face index can not fit into an int32_t value!");
							else if (!addNodeIndex(truncatedValue)) return false;
						}
					}
					else return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Indices does not have an integer array property where expected!");
					if ((m_indices.size() > 0) && ((m_faceEnds.size() <= 0) || (m_faceEnds.back() != m_indices.size()))) {
						if (logger != nullptr) logger->Warning("FBXData::FBXMeshExtractor::ExtractFaces - Last index not negative... Pretending as if it was xor-ed with -1...");
						m_indices.back().nextIndexOnPoly = static_cast<uint32_t>((m_faceEnds.size() <= 0) ? 0 : m_faceEnds.back());
						m_faceEnds.push_back(m_indices.size());
					}
				}
				return true;
			}

			inline bool ExtractEdges(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* verticesNode = FindChildNode(objectNode, "Edges", 0);
				if (verticesNode == nullptr) return true;
				m_layerIndexBuffer.clear();
				if (verticesNode->PropertyCount() >= 1)
					if (!FillUint32Buffer(verticesNode->NodeProperty(0), "FBXData::Objects::Geometry::Edges", m_layerIndexBuffer, logger)) return false;
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

			template<size_t IndexOffset>
			inline bool ExtractLayerIndexInformation(
				const FBXContent::Node* layerElement, size_t layerElemCount, 
				const std::string_view& layerElementName, const std::string_view& indexSubElementName, OS::Logger* logger) {
				auto findInformationType = [&](const std::string_view& name) -> const FBXContent::Property* {
					const FBXContent::Node* informationTypeNode = FindChildNode(layerElement, name);
					if (informationTypeNode == nullptr)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node missing for ", layerElementName, "!");
					else if (informationTypeNode->PropertyCount() <= 0)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node has no value for ", layerElementName, "!");
					const FBXContent::Property* informationTypeProp = &informationTypeNode->NodeProperty(0);
					if (informationTypeProp->Type() != FBXContent::PropertyType::STRING)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " not string for ", layerElementName, "!");
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
					const FBXContent::Node* indexNode = FindChildNode(layerElement, indexSubElementName);
					if (indexNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " node missing!");
					else if (indexNode->PropertyCount() <= 0) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " has no values!");
					if (!FillUint32Buffer(indexNode->NodeProperty(0), indexSubElementName, m_layerIndexBuffer, logger)) return false;
					else for (size_t i = 0; i < m_layerIndexBuffer.size(); i++)
						if (m_layerIndexBuffer[i] >= layerElemCount)
							return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ",
								indexSubElementName, " contains indices greater than or equal to ", indexSubElementName, ".size()<", layerElemCount, ">!");
				}
				else return Error(logger, false,
					"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ReferenceInformationType<", referenceInformationType, "> not supported for ", layerElementName, "!");

				// MappingInformationType:
				const FBXContent::Property* mappingInformationTypeProp = findInformationType("MappingInformationType");
				if (mappingInformationTypeProp == nullptr) return false;
				const std::string_view mappingInformationType = *mappingInformationTypeProp;

				// Fill m_indices:
				const size_t indexCount = m_layerIndexBuffer.size();
				auto indexValue = [&](size_t index) -> uint32_t& {
					return (*reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(m_indices.data() + index) + IndexOffset));
				};
				if (mappingInformationType == "ByVertex" || mappingInformationType == "ByVertice") {
					if (indexCount != m_nodeVertices.size()) 
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the vertex count!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = m_layerIndexBuffer[m_indices[i].vertexId];
				}
				else if (mappingInformationType == "ByPolygonVertex") {
					if (indexCount != m_indices.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon vertex count!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = m_layerIndexBuffer[i];
				}
				else if (mappingInformationType == "ByPolygon") {
					if (indexCount != m_faceEnds.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon count!");
					size_t faceId = 0;
					for (size_t i = 0; i < m_indices.size(); i++) {
						indexValue(i) = m_layerIndexBuffer[faceId];
						if (i == m_faceEnds[faceId]) faceId++;
					}
				}
				else if (mappingInformationType == "ByEdge") {
					if (indexCount != m_nodeEdges.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the edge count!");
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
								"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Edges do not cover all indices and can not be used for layer elements for ", layerElementName, "!");
				}
				else if (mappingInformationType == "AllSame") {
					if (layerElemCount <= 0)
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " is zero!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = 0;
				}
				else return Error(logger, false,
					"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - MappingInformationType<", mappingInformationType, "> not supported for ", layerElementName, "!");
				return true;
			}

			inline bool ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* normalsNode = FindChildNode(layerElement, "Normals", 0);
				if (normalsNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractNormals - Normals node missing!");
				else if (normalsNode->PropertyCount() >= 1)
					if (!FillVectorBuffer(normalsNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementNormal::Normals", m_normals, logger)) return false;
				return ExtractLayerIndexInformation<offsetof(Index, normalId)>(layerElement, m_normals.size(), "Normals", "NormalsIndex", logger);
			}

			inline bool ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* smoothingNode = FindChildNode(layerElement, "Smoothing", 0);
				if (smoothingNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Smoothing node missing!");
				else if (smoothingNode->PropertyCount() >= 1)
					if (!FillBoolBuffer(smoothingNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementSmoothing::Smoothing", m_smooth, logger)) return false;
				return ExtractLayerIndexInformation<offsetof(Index, smoothId)>(layerElement, m_smooth.size(), "Smoothing", "SmoothingIndex", logger);
			}

			inline bool ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* uvNode = FindChildNode(layerElement, "UV", 0);
				if (uvNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractUVs - UV node missing!");
				else if (uvNode->PropertyCount() >= 1)
					if (!FillVectorBuffer(uvNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementUV::UV", m_uvs, logger)) return false;
				return ExtractLayerIndexInformation<offsetof(Index, uvId)>(layerElement, m_uvs.size(), "UV", "UVIndex", logger);
			}

		public:
			inline bool ExtractData(const FBXContent::Node* objectNode, OS::Logger* logger) {
				auto error = [&](auto... message) { return Error(logger, false, message...); };
				Clear();
				if (!ExtractVertices(objectNode, logger)) return false;
				else if (!ExtractFaces(objectNode, logger)) return false;
				else if (!ExtractEdges(objectNode, logger)) return false;
				std::pair<const FBXContent::Node*, int64_t> normalLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				std::pair<const FBXContent::Node*, int64_t> smoothingLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				std::pair<const FBXContent::Node*, int64_t> uvLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				for (size_t i = 0; i < objectNode->NestedNodeCount(); i++) {
					const FBXContent::Node* layerNode = &objectNode->NestedNode(i);
					const std::string_view elementName = layerNode->Name();
					std::pair<const FBXContent::Node*, int64_t>* layerElementToReplace = (
						(elementName == "LayerElementNormal") ? (&normalLayerElement) :
						(elementName == "LayerElementSmoothing") ? (&normalLayerElement) :
						(elementName == "LayerElementUV") ? (&uvLayerElement) : nullptr);
					if (layerElementToReplace == nullptr) continue;
					else if (layerNode->PropertyCount() <= 0) return error("FBXData::FBXMeshExtractor::ExtractData - Layer element does not have layer id!");
					const FBXContent::Property& layerIdProp = layerNode->NodeProperty(0);
					int64_t layerIndex;
					if (layerIdProp.Type() == FBXContent::PropertyType::INT_64) layerIndex = static_cast<int64_t>(layerIdProp.operator int64_t());
					else if (layerIdProp.Type() == FBXContent::PropertyType::INT_32) layerIndex = static_cast<int64_t>(layerIdProp.operator int32_t());
					else if (layerIdProp.Type() == FBXContent::PropertyType::INT_16) layerIndex = static_cast<int64_t>(layerIdProp.operator int16_t());
					else return error("FBXData::FBXMeshExtractor::ExtractData - Layer element does not have an integer layer id!");
					bool hasAnotherLayer = (layerElementToReplace->first != nullptr);
					if ((!hasAnotherLayer) || layerElementToReplace->second > layerIndex)
						(*layerElementToReplace) = std::make_pair(layerNode, layerIndex);
					else hasAnotherLayer = true;
					if (hasAnotherLayer && logger != nullptr) 
						logger->Warning("FBXData::FBXMeshExtractor::ExtractData - Multiple layer elements<", elementName, "> not [currently] supported...");
				}
				if (!ExtractNormals(normalLayerElement.first, logger)) return false;
				else if (!ExtractSmoothing(smoothingLayerElement.first, logger)) return false;
				else if (!ExtractUVs(uvLayerElement.first, logger)) return false;
				else return true;
			}

			inline Reference<PolyMesh> CreateMesh(const std::string_view& name) {

			}
		};
	}


	Reference<FBXData> FBXData::Extract(const FBXContent* sourceContent, OS::Logger* logger) {
		auto error = [&](auto... message) { return Error<FBXData*>(logger, nullptr, message...); };
		auto warning = [&](auto... message) {  if (logger != nullptr) logger->Warning(message...); };

		if (sourceContent == nullptr) return error("FBXData::Extract - NULL sourceContent provided!");

		// Root level nodes:
		const FBXContent::Node* fbxHeaderExtensionNode = FindChildNode(&sourceContent->RootNode(), "FBXHeaderExtension", 0, logger, "FBXData::Extract - FBXHeaderExtension missing!");
		const FBXContent::Node* fileIdNode = FindChildNode(&sourceContent->RootNode(), "FileId", 1, logger, "FBXData::Extract - FileId missing!");
		const FBXContent::Node* creationTimeNode = FindChildNode(&sourceContent->RootNode(), "CreationTime", 2, logger, "FBXData::Extract - CreationTime missing!");
		const FBXContent::Node* creatorNode = FindChildNode(&sourceContent->RootNode(), "Creator", 3, logger, "FBXData::Extract - Creator missing!");
		const FBXContent::Node* globalSettingsNode = FindChildNode(&sourceContent->RootNode(), "GlobalSettings", 4, logger, "FBXData::Extract - GlobalSettings missing!");
		const FBXContent::Node* documentsNode = FindChildNode(&sourceContent->RootNode(), "Documents", 5);
		const FBXContent::Node* referencesNode = FindChildNode(&sourceContent->RootNode(), "References", 6);
		const FBXContent::Node* definitionsNode = FindChildNode(&sourceContent->RootNode(), "Definitions", 7);
		const FBXContent::Node* objectsNode = FindChildNode(&sourceContent->RootNode(), "Objects", 8);
		const FBXContent::Node* connectionsNode = FindChildNode(&sourceContent->RootNode(), "Connections", 9);
		const FBXContent::Node* takesNode = FindChildNode(&sourceContent->RootNode(), "Takes", 10);

		// Notes:
		// 0. We ignore the contents of FBXHeaderExtension for performance and also, we don't really care for them; invalid FBX files may pass here;
		// 1. We also don't care about FileId, CreationTime Creator, Documents, References and Takes for similar reasons;
		// 2. I've followed this link to make sense of the content: 
		//    https://web.archive.org/web/20160605023014/https://wiki.blender.org/index.php/User:Mont29/Foundation/FBX_File_Structure#Spaces_.26_Parenting
		Reference<FBXData> result = Object::Instantiate<FBXData>();

		// __TODO__: Parse Definitions...

		// Parse Objects:
		if (objectsNode != nullptr) {
			FBXMeshExtractor meshExtractor;

			for (size_t i = 0; i < objectsNode->NestedNodeCount(); i++) {
				const FBXContent::Node* objectNode = &objectsNode->NestedNode(i);

				// NodeAttribute:
				const std::string_view& nodeAttribute = objectNode->Name();

				// Property count:
				if (objectNode->PropertyCount() < 3) {
					warning("FBXData::Extract - Object[", i, "] has less than 3 properties. Expected [UID, Name::Class, Sub-Class]; Object entry will be ignored...");
					continue;
				}

				// UID:
				const FBXContent::Property& objectUidProperty = objectNode->NodeProperty(0);
				int64_t objectUid;
				if (objectUidProperty.Type() == FBXContent::PropertyType::INT_64) objectUid = ((int64_t)objectUidProperty);
				else if (objectUidProperty.Type() == FBXContent::PropertyType::INT_32) objectUid = static_cast<int64_t>((int32_t)objectUidProperty);
				else if (objectUidProperty.Type() == FBXContent::PropertyType::INT_16) objectUid = static_cast<int64_t>((int16_t)objectUidProperty);
				else {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[0]<UID> is not an integer type; Object entry will be ignored...");
					continue;
				}

				// Name::Class
				const FBXContent::Property& nameClassProperty = objectNode->NodeProperty(1);
				if (nameClassProperty.Type() != FBXContent::PropertyType::STRING) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> is not a string; Object entry will be ignored...");
					continue;
				}
				const std::string_view nameClass = nameClassProperty;
				if (nameClassProperty.Count() != (nodeAttribute.size() + nameClass.size() + 2u)) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> not formatted correctly; Object entry will be ignored...");
					continue;
				}
				else if (nameClass.data()[nameClass.size()] != 0x00 || nameClass.data()[nameClass.size() + 1] != 0x01) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> Expected '0x00,0x01' between Name and Class, got something else; Object entry will be ignored...");
					continue;
				}
				else if (std::string_view(nameClass.data() + nameClass.size() + 2) != nodeAttribute) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> 'Name::' not followed by nodeAttribute<", nodeAttribute, ">; Object entry will be ignored...");
					continue;
				}

				// Sub-class:
				const FBXContent::Property& subClassProperty = objectNode->NodeProperty(2);
				if (subClassProperty.Type() != FBXContent::PropertyType::STRING) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[2]<Sub-class> is not a string; Object entry will be ignored...");
					continue;
				}


				// Fallback for reading types that are not yet implemented:
				auto readNotImplemented = [&]() -> bool {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
					return true;
				};

				// Reads a Model:
				auto readModel = [&]() -> bool {
					// __TODO__: Implement this crap!
					return readNotImplemented();
				};

				// Reads Light:
				auto readLight = [&]() -> bool {
					// __TODO__: Implement this crap!
					return readNotImplemented();
				};

				// Reads Camera:
				auto readCamera = [&]() -> bool {
					// __TODO__: Implement this crap!
					return readNotImplemented();
				};

				// Reads a Mesh:
				auto readMesh = [&]() -> bool {
					// __TODO__: Implement this crap!
					if (((std::string_view)subClassProperty) != "Mesh") {
						warning("FBXData::Extract::readMesh - subClassProperty<'", ((std::string_view)subClassProperty).data(), "'> is nor 'Mesh'!; Ignoring the node...");
						return true;
					}
					if (!meshExtractor.ExtractData(objectNode, logger)) return false;
					

					return true;
				};

				// Ignores unknown data type with a message:
				auto readUnknownType = [&]() -> bool {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "': Unknown type! Object entry will be ignored...");
					return true;
				};

				// Distinguish object types:
				bool success;
				if (nodeAttribute == "Model") success = readModel();
				else if (nodeAttribute == "Light") success = readLight();
				else if (nodeAttribute == "Camera") success = readCamera();
				else if (nodeAttribute == "Geometry") success = readMesh();
				else success = readUnknownType();
				if (!success) return nullptr;
			}
		}

		// __TODO__: Parse Connections...

		return result;
  	}
}
