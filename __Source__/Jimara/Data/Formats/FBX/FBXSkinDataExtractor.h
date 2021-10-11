#pragma once
#include "FBXObjectIndex.h"


namespace Jimara {
	namespace FBXHelpers {
		/// <summary>
		/// Utility for extracting mesh skinning data from an FBX file.
		/// Note: Objects of this class are meant to be used for optimal memory management
		/// </summary>
		class FBXSkinDataExtractor {
		public:
			/// <summary>
			/// Vertex id with weight
			/// </summary>
			struct BoneWeight {
				/// <summary> Vertex ID (from geometry node, not the final result) </summary>
				uint32_t vertex = 0;

				/// <summary> Bone weight for the vertex </summary>
				float weight = 0.0f;
			};

			/// <summary>
			/// Bone binding information
			/// </summary>
			struct BoneInfo {
			public:
				/// <summary> Bone's corresponding transform node UID ("Model", "LimbNode/<or anything really>") UID </summary>
				FBXUid TransformId()const;

				/// <summary> Bone reference pose </summary>
				const Matrix4& ReferencePose()const;

				/// <summary> Number of vertex weights </summary>
				size_t WeightCount()const;

				/// <summary>
				/// Vertex weight by index
				/// </summary>
				/// <param name="weightId"> Vertex weight index [valid range is [0 - WeightCount())] </param>
				/// <returns> Vertex weight </returns>
				const BoneWeight& Weight(size_t weightId)const;

			private:
				// "Owner" FBXSkinDataExtractor
				const FBXSkinDataExtractor* m_owner = nullptr;

				// First BoneWeight index within m_owner->m_boneWeights
				size_t m_baseWeightId = 0;

				// Number of BoneWeight-s dedicated to this bone
				size_t m_weightCount = 0;

				// Bone's corresponding transform node UID
				FBXUid m_boneTransformId = 0;

				// Bone reference pose
				Matrix4 m_boneReferencePose = Math::Identity();

				// FBXSkinDataExtractor sets the internals...
				friend class FBXSkinDataExtractor;
			};


			/// <summary>
			/// Checks if given node is a "Skin" object
			/// </summary>
			/// <param name="node"> Node to check </param>
			/// <returns> True, if node is a "Deformer" with a sub-class "Skin" </returns>
			static bool IsSkin(const FBXObjectIndex::NodeWithConnections* node);

			/// <summary>
			/// Attempts to extract skinning data from the node
			/// </summary>
			/// <param name="node"> Node to analyze (shoudl be a Skin node) </param>
			/// <param name="logger"> Ligger for error/warning reporting </param>
			/// <returns> True, if the node gets analyzed successfully </returns>
			bool Extract(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger);

			/// <summary> [Optional] root bone UID (currently will never be set) </summary>
			std::optional<FBXUid> RootBoneId()const;

			/// <summary> Number of bones within the armature/skeleton/skin/whatever you call it... </summary>
			uint32_t BoneCount()const;

			/// <summary>
			/// Bone data by index
			/// </summary>
			/// <param name="index"> Bone index (valid range is [0 - BoneCount())) </param>
			/// <returns> Bone data </returns>
			const BoneInfo& Bone(size_t index)const;

		private:
			// [Optional] root bone UID (currently will never be set)
			std::optional<FBXUid> m_rootBoneId;

			// Bone infos
			std::vector<BoneInfo> m_boneInfo;

			// Bone weight buffer
			std::vector<BoneWeight> m_boneWeights;

			// Temporary buffer for vertex indices
			std::vector<uint32_t> m_indexBuffer;

			// Temporary buffer for matrices and weights
			std::vector<float> m_floatBuffer;


			// Here are "Traditional" analisis steps:

			// First, we clean all state-related buffers here
			void Clear();

			// Next, we add bones one by one
			bool ExtractBone(const FBXObjectIndex::NodeWithConnections* boneNode, OS::Logger* logger);
		};
	}
}
