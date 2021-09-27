#include "FBXData.h"
#include <stddef.h>
#include <unordered_map>
#include <map>
#include <stdint.h>
#include <algorithm>


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
		template<typename Type>
		struct VectorElementExtractor {};
		template<>
		struct VectorElementExtractor<Vector3> {
			inline static constexpr size_t VectorDimms() { return 3; }
			inline static Vector3 ExtractF(const FBXContent::Property& prop, size_t startIndex) {
				return Vector3(prop.Float32Elem(startIndex), prop.Float32Elem(startIndex + 1), prop.Float32Elem(startIndex + 2));
			}
			inline static Vector3 ExtractD(const FBXContent::Property& prop, size_t startIndex) {
				return Vector3(static_cast<float>(prop.Float64Elem(startIndex)), static_cast<float>(prop.Float64Elem(startIndex + 1)), static_cast<float>(prop.Float64Elem(startIndex + 2)));
			}
		};
		struct Vector3ExtractorXZY {
			inline static Vector3 ExtractF(const FBXContent::Property& prop, size_t startIndex) {
				return Vector3(prop.Float32Elem(startIndex), prop.Float32Elem(startIndex + 2), prop.Float32Elem(startIndex + 1));
			}
			inline static Vector3 ExtractD(const FBXContent::Property& prop, size_t startIndex) {
				return Vector3(static_cast<float>(prop.Float64Elem(startIndex)), static_cast<float>(prop.Float64Elem(startIndex + 2)), static_cast<float>(prop.Float64Elem(startIndex + 1)));
			}
		};
		template<>
		struct VectorElementExtractor<Vector2> {
			inline static constexpr size_t VectorDimms() { return 2; }
			inline static Vector2 ExtractF(const FBXContent::Property& prop, size_t startIndex) {
				return Vector2(prop.Float32Elem(startIndex), prop.Float32Elem(startIndex + 1));
			}
			inline static Vector2 ExtractD(const FBXContent::Property& prop, size_t startIndex) {
				return Vector2(static_cast<float>(prop.Float64Elem(startIndex)), static_cast<float>(prop.Float64Elem(startIndex + 1)));
			}
		};
		struct Vector2ExtractorUV {
			inline static Vector2 ExtractF(const FBXContent::Property& prop, size_t startIndex) {
				return Vector2(prop.Float32Elem(startIndex), 1.0f - prop.Float32Elem(startIndex + 1));
			}
			inline static Vector2 ExtractD(const FBXContent::Property& prop, size_t startIndex) {
				return Vector2(static_cast<float>(prop.Float64Elem(startIndex)), 1.0f - static_cast<float>(prop.Float64Elem(startIndex + 1)));
			}
		};

		template<typename VectorType, typename ExtractorType = VectorElementExtractor<VectorType>>
		inline static bool FillVectorBuffer(const FBXContent::Property& prop, const std::string_view& propertyName, std::vector<VectorType>& buffer, OS::Logger* logger) {
			if (prop.Type() != FBXContent::PropertyType::FLOAT_32_ARR && prop.Type() != FBXContent::PropertyType::FLOAT_64_ARR)
				return Error(logger, false, "FBXData::Extract::FillVectorBuffer - property<", propertyName, "> is not a floating point array!");
			else if ((prop.Count() % VectorElementExtractor<VectorType>::VectorDimms()) != 0)
				return Error(logger, false, "FBXData::Extract::FillVectorBuffer - ",
					"property<", propertyName, "> element count is not a multiple of ", VectorElementExtractor<VectorType>::VectorDimms(), "!");
			else if (prop.Type() == FBXContent::PropertyType::FLOAT_32_ARR) {
				for (size_t i = 0; i < prop.Count(); i += VectorElementExtractor<VectorType>::VectorDimms())
					buffer.push_back(ExtractorType::ExtractF(prop, i));
			}
			else if (prop.Type() == FBXContent::PropertyType::FLOAT_64_ARR) {
				for (size_t i = 0; i < prop.Count(); i += VectorElementExtractor<VectorType>::VectorDimms())
					buffer.push_back(ExtractorType::ExtractD(prop, i));
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
					return FillVectorBuffer<Vector3, Vector3ExtractorXZY>(verticesNode->NodeProperty(0), "FBXData::Objects::Geometry::Vertices", m_nodeVertices, logger);
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
					if (logger != nullptr)
						logger->Warning("FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", layerElementName, " layer was set 'ByEdge'; not sure if the interpretation is correct...");
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

			inline bool ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* normalsNode = FindChildNode(layerElement, "Normals", 0);
				if (normalsNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractNormals - Normals node missing!");
				else if (normalsNode->PropertyCount() >= 1)
					if (!FillVectorBuffer<Vector3, Vector3ExtractorXZY>(normalsNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementNormal::Normals", m_normals, logger)) return false;
				return ExtractLayerIndexInformation<NORMAL_ID_OFFSET>(layerElement, m_normals.size(), "Normals", "NormalsIndex", logger);
			}

			inline bool ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* smoothingNode = FindChildNode(layerElement, "Smoothing", 0);
				if (smoothingNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Smoothing node missing!");
				else if (smoothingNode->PropertyCount() >= 1)
					if (!FillBoolBuffer(smoothingNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementSmoothing::Smoothing", m_smooth, logger)) return false;
				return ExtractLayerIndexInformation<SMOOTH_ID_OFFSET>(layerElement, m_smooth.size(), "Smoothing", "SmoothingIndex", logger);
			}

			inline bool ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) {
					m_uvs = { Vector2(0.0f) };
					return true;
				}
				const FBXContent::Node* uvNode = FindChildNode(layerElement, "UV", 0);
				if (uvNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractUVs - UV node missing!");
				else if (uvNode->PropertyCount() >= 1)
					if (!FillVectorBuffer<Vector2, Vector2ExtractorUV>(uvNode->NodeProperty(0), "FBXData::Objects::Geometry::LayerElementUV::UV", m_uvs, logger)) return false;
				if (!ExtractLayerIndexInformation<UV_ID_OFFSET>(layerElement, m_uvs.size(), "UV", "UVIndex", logger)) return false;
				else if (m_uvs.size() <= 0) m_uvs.push_back(Vector2(0.0f));
				return true;
			}

#undef NORMAL_ID_OFFSET
#undef SMOOTH_ID_OFFSET
#undef UV_ID_OFFSET

			inline bool FixNormals(OS::Logger* logger) {
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
				if (desiredNormalCount > UINT32_MAX) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Too many normals!");
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

			inline Reference<PolyMesh> CreateMesh(const std::string_view& name) {
				std::map<VNUIndex, uint32_t> vertexIndexMap;
				Reference<PolyMesh> mesh = Object::Instantiate<PolyMesh>(name);
				PolyMesh::Writer writer(mesh);
				for (size_t faceId = 0; faceId < m_faceEnds.size(); faceId++) {
					const size_t faceEnd = m_faceEnds[faceId];
					if (faceEnd <= 0 || (faceId > 0 && m_faceEnds[faceId] <= m_faceEnds[faceId - 1])) continue;
					writer.Faces().push_back(PolygonFace());
					PolygonFace& face = writer.Faces().back();
					for (size_t i = m_indices[faceEnd - 1].nextIndexOnPoly; i < faceEnd; i++) {
						const VNUIndex& index = m_indices[i];
						uint32_t vertexIndex;
						{
							std::map<VNUIndex, uint32_t>::const_iterator it = vertexIndexMap.find(index);
							if (it == vertexIndexMap.end()) {
								vertexIndex = static_cast<uint32_t>(writer.Verts().size());
								vertexIndexMap[index] = vertexIndex;
								writer.Verts().push_back(MeshVertex(m_nodeVertices[index.vertexId], m_normals[index.normalId], m_uvs[index.uvId]));
							}
							else vertexIndex = it->second;
						}
						face.Push(vertexIndex);
					}
				}
				return mesh;
			}

		public:
			inline Reference<PolyMesh> ExtractData(const FBXContent::Node* objectNode, const std::string_view& name, OS::Logger* logger) {
				auto error = [&](auto... message) ->Reference<PolyMesh> { return Error<Reference<PolyMesh>>(logger, nullptr, message...); };
				Clear();
				if (!ExtractVertices(objectNode, logger)) return nullptr;
				else if (!ExtractFaces(objectNode, logger)) return nullptr;
				else if (!ExtractEdges(objectNode, logger)) return nullptr;
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
				if (!ExtractNormals(normalLayerElement.first, logger)) return nullptr;
				else if (!ExtractSmoothing(smoothingLayerElement.first, logger)) return nullptr;
				else if (!ExtractUVs(uvLayerElement.first, logger)) return nullptr;
				else if (!FixNormals(logger)) return nullptr;
				else return CreateMesh(name);
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
					// __TODO__: Warning needed, but this check has to change a bit for animation curves...
					//warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> not formatted correctly(name=", nodeAttribute, "); Object entry will be ignored...");
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
					if (((std::string_view)subClassProperty) != "Mesh") {
						warning("FBXData::Extract::readMesh - subClassProperty<'", ((std::string_view)subClassProperty).data(), "'> is nor 'Mesh'!; Ignoring the node...");
						return true;
					}
					const Reference<PolyMesh> mesh = meshExtractor.ExtractData(objectNode, nameClass, logger);
					if (mesh == nullptr) return false;
					result->m_meshIndex[objectUid] = static_cast<int64_t>(result->m_meshes.size());
					result->m_meshes.push_back(FBXMesh{ objectUid, mesh });
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

	size_t FBXData::MeshCount()const { return m_meshes.size(); }

	const FBXData::FBXMesh& FBXData::GetMesh(size_t index)const { return m_meshes[index]; }
}
