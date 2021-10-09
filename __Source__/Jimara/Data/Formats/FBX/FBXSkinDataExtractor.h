#pragma once
#include "FBXObjectIndex.h"


namespace Jimara {
	namespace FBXHelpers {
		class FBXSkinDataExtractor {
		public:
			struct BoneWeight {
				uint32_t vertex = 0;
				float weight = 0.0f;
			};

			struct BoneInfo {
			public:
				FBXUid TransformId()const;

				const Matrix4& ReferencePose()const;

				size_t WeightCount()const;

				const BoneWeight& Weight(size_t weightId)const;

			private:
				const FBXSkinDataExtractor* m_owner = nullptr;
				size_t m_baseWeightId = 0;
				size_t m_weightCount = 0;
				FBXUid m_boneTransformId = 0;
				Matrix4 m_boneReferencePose = Math::Identity();

				friend class FBXSkinDataExtractor;
			};

			static bool IsSkin(const FBXObjectIndex::NodeWithConnections* node);

			bool Extract(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger);

			FBXUid RootBoneId()const;

			uint32_t BoneCount()const;

			const BoneInfo& Bone(size_t index)const;

		private:
			FBXUid m_rootBoneId = 0;
			std::vector<BoneInfo> m_boneInfo;
			std::vector<BoneWeight> m_boneWeights;

			std::vector<uint32_t> m_indexBuffer;
			std::vector<float> m_floatBuffer;

			void Clear();
			bool ExtractBone(const FBXObjectIndex::NodeWithConnections* boneNode, OS::Logger* logger);
		};
	}
}
