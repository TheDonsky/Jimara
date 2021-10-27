#pragma once
#include "FBXObjects.h"
#include "FBXObjectIndex.h"
#include "../../Animation.h"


namespace Jimara {
	namespace FBXHelpers {
		class FBXAnimationExtractor {
		public:
			typedef std::pair<const FBXNode*, AnimationClip::Vector3Track::EvaluationMode> TransformInfo;

			bool Extract(const FBXObjectIndex& objectIndex, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<TransformInfo, const FBXNode*> getNodeParent
				, Callback<FBXAnimation*> onAnimationFound);

		private:
			std::vector<int64_t> m_timeBuffer;
			std::vector<float> m_valueBuffer;
			std::vector<int64_t> m_attrFlagBuffer;
			std::vector<float> m_dataBuffer;
			std::vector<size_t> m_refCountBuffer;

			Reference<FBXAnimation> ExtractLayer(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<TransformInfo, const FBXNode*> getNodeParent);
			bool ExtractCurveNode(const FBXObjectIndex::NodeWithConnections& node, AnimationClip::Writer& writer, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<TransformInfo, const FBXNode*> getNodeParent);
			bool ExtractCurve(const FBXObjectIndex::NodeWithConnections& node, float defaultValue, TimelineCurve<float, BezierNode<float>>& curve, OS::Logger* logger);
			bool FillCurve(OS::Logger* logger, TimelineCurve<float, BezierNode<float>>& curve)const;
		};
	}
}
