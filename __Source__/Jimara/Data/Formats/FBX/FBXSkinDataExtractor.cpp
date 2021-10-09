#include "FBXSkinDataExtractor.h"

namespace Jimara {
	namespace FBXHelpers {
		FBXUid FBXSkinDataExtractor::BoneInfo::TransformId()const { return m_boneTransformId; }

		const Matrix4& FBXSkinDataExtractor::BoneInfo::ReferencePose()const { return m_boneReferencePose; }

		size_t FBXSkinDataExtractor::BoneInfo::WeightCount()const { return m_weightCount; }

		const FBXSkinDataExtractor::BoneWeight& FBXSkinDataExtractor::BoneInfo::Weight(size_t weightId)const { return m_owner->m_boneWeights[m_baseWeightId + weightId]; }

		namespace {
			inline static bool IsDeformer(const FBXObjectIndex::NodeWithConnections* node) {
				if (node == nullptr) return false;
				else if (node->node.NodeAttribute() != "Deformer") return false;
				else return true;
			}

			inline static FBXUid GetBoneTransform(const FBXObjectIndex::NodeWithConnections* boneNode) {
				for (size_t i = 0; i < boneNode->childConnections.Size(); i++) {
					const FBXObjectIndex::NodeWithConnections* transformNode = boneNode->childConnections[i].connection;
					if (transformNode->node.NodeAttribute() == "Model") return transformNode->node.Uid();
				}
				return 0;
			}
		}

		bool FBXSkinDataExtractor::IsSkin(const FBXObjectIndex::NodeWithConnections* node) {
			return IsDeformer(node) && (node->node.Class() == "Deformer") && (node->node.SubClass() == "Skin");
		}

		bool FBXSkinDataExtractor::Extract(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger) {
			Clear();
			if (!IsSkin(&node)) {
				if (logger != nullptr) logger->Error("FBXSkinDataExtractor::Extract - Provided node is not a 'Skin' type 'Deformer' object!");
			}
			m_rootBoneId = GetBoneTransform(&node);
			for (size_t i = 0; i < node.childConnections.Size(); i++)
				if (!ExtractBone(node.childConnections[i].connection, logger)) return false;
			return true;
		}

		FBXUid FBXSkinDataExtractor::RootBoneId()const { return m_rootBoneId; }

		uint32_t FBXSkinDataExtractor::BoneCount()const { return static_cast<uint32_t>(m_boneInfo.size()); }

		const FBXSkinDataExtractor::BoneInfo& FBXSkinDataExtractor::Bone(size_t index)const { return m_boneInfo[index]; }


		void FBXSkinDataExtractor::Clear() {
			m_rootBoneId = 0;
			m_boneInfo.clear();
			m_boneWeights.clear();
		}

		namespace {
			inline static bool IsBone(const FBXObjectIndex::NodeWithConnections* node) {
				return IsDeformer(node) && (node->node.Class() == "SubDeformer") && (node->node.SubClass() == "Cluster");
			}

			inline static bool GetMatrix(Matrix4& matrix, std::vector<float>& tmpBuffer, const char* matrixNodeName, const FBXContent::Node* boneNode, OS::Logger* logger) {
				const FBXContent::Node* matrixNode = boneNode->FindChildNodeByName(matrixNodeName);
				if (matrixNode == nullptr) {
					if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone::GetMatrix - '", matrixNodeName, "' missing!");
					return false;
				}
				else if (matrixNode->PropertyCount() <= 0) {
					if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone::GetMatrix - '", matrixNodeName, "' does not have a value!");
					return false;
				}
				else if (!matrixNode->NodeProperty(0).Fill(tmpBuffer, true)) {
					if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone::GetMatrix - '", matrixNodeName, "' is not a floating pooint array!");
					return false;
				}
				else if (tmpBuffer.size() != 16) {
					if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone::GetMatrix - '", matrixNodeName, "' contains ", tmpBuffer.size(), " elements instead of 16!");
					return false;
				}
				else {
					matrix = Matrix4(
						Vector4(tmpBuffer[0], tmpBuffer[1], -tmpBuffer[2], tmpBuffer[3]),
						Vector4(tmpBuffer[4], tmpBuffer[5], -tmpBuffer[6], tmpBuffer[7]),
						Vector4(tmpBuffer[8], tmpBuffer[9], -tmpBuffer[10], tmpBuffer[11]),
						Vector4(tmpBuffer[12], tmpBuffer[13], -tmpBuffer[14], tmpBuffer[15]));
					return true;
				}
			}
		}

		bool FBXSkinDataExtractor::ExtractBone(const FBXObjectIndex::NodeWithConnections* boneNode, OS::Logger* logger) {
			if (!IsBone(boneNode)) return true;

			// Get matrices:
			Matrix4 transform, transformLink, transformAssociateModel;
			if (!GetMatrix(transform, m_floatBuffer, "Transform", boneNode->node.Node(), logger)) return false;
			else if (!GetMatrix(transformLink, m_floatBuffer, "TransformLink", boneNode->node.Node(), logger)) return false;
			else if (!GetMatrix(transformAssociateModel, m_floatBuffer, "TransformAssociateModel", boneNode->node.Node(), logger)) return false;
			
			// Get indexes:
			{
				const FBXContent::Node* indexesNode = boneNode->node.Node()->FindChildNodeByName("Indexes");
				if (indexesNode != nullptr && indexesNode->PropertyCount() > 0) {
					if (!indexesNode->NodeProperty(0).Fill(m_indexBuffer, true)) {
						if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone - 'Indexes' has to to be a valid unsigned integer array!");
						return false;
					}
				}
				else m_indexBuffer.clear();
			}

			// Get weights:
			{
				const FBXContent::Node* weightsNode = boneNode->node.Node()->FindChildNodeByName("Weights");
				if (weightsNode != nullptr && weightsNode->PropertyCount() > 0) {
					if (!weightsNode->NodeProperty(0).Fill(m_floatBuffer, true)) {
						if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone - 'Weights' has to to be a valid floating point array!");
						return false;
					}
				}
				else m_floatBuffer.clear();
			}

			// Make sure data is not malformed:
			if (m_indexBuffer.size() != m_indexBuffer.size()) {
				if (logger != nullptr) logger->Error("FBXSkinDataExtractor::ExtractBone - 'Weights' and 'Indices' contain different number of entries!");
				return false;
			}

			// Push bone:
			{
				BoneInfo boneInfo;
				boneInfo.m_owner = this;
				boneInfo.m_baseWeightId = m_boneWeights.size();
				boneInfo.m_weightCount = m_indexBuffer.size();
				boneInfo.m_boneTransformId = GetBoneTransform(boneNode);
				boneInfo.m_boneReferencePose = Math::Inverse(transform);
				m_boneInfo.push_back(boneInfo);
			}

			// Push weights:
			for (size_t i = 0; i < m_indexBuffer.size(); i++)
				m_boneWeights.push_back(BoneWeight{ m_indexBuffer[i], m_floatBuffer[i] });
			
			return true;
		}
	}
}
