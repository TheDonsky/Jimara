#pragma once
#include "FBXObjects.h"
#include "FBXObjectIndex.h"
#include "../../Animation.h"


namespace Jimara {
	namespace FBXHelpers {
		/// <summary>
		/// Utility for extracting Animation data from FBX Content
		/// </summary>
		class FBXAnimationExtractor {
		public:
			/// <summary> Transform, with it's euler angle mode </summary>
			typedef std::pair<const FBXNode*, AnimationClip::Vector3Track::EvaluationMode> TransformInfo;

			/// <summary>
			/// Extracts all animations from the file
			/// </summary>
			/// <param name="objectIndex"> FBX Object index </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="rootScale"> Root object scale for value correction </param>
			/// <param name="rootAxisWrangle"> Root axis wrangle for position correction </param>
			/// <param name="getNodeById"> Function, requesting information about an FBX node by UID </param>
			/// <param name="getNodeParent"> Function for getting parent nodes </param>
			/// <param name="onAnimationFound"> Invoked, whenever the extractor finds and successfully parses an animation clip </param>
			/// <returns> True if no fatal error occures </returns>
			bool Extract(const FBXObjectIndex& objectIndex, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<const FBXNode*, const FBXNode*> getNodeParent
				, Callback<FBXAnimation*> onAnimationFound);

		private:
			// Buffer for FBX timestamps
			std::vector<int64_t> m_timeBuffer;

			// Buffer for curve values
			std::vector<float> m_valueBuffer;

			// Buffer for curve flags
			std::vector<int64_t> m_attrFlagBuffer;

			// Buffer for KeyAttrDataFloat
			std::vector<float> m_dataBuffer;

			// Buffer for KeyAttrRefCount
			std::vector<size_t> m_refCountBuffer;

			// Buffer for temporary storage of bezier nodes when correcting for time origin and what not
			std::vector<std::pair<float, BezierNode<float>>> m_bezierNodeBuffer;

			// Here lie basic extraction tests:

			// Once we found an 'AnimationLayer' node, we extract animation from it:
			Reference<FBXAnimation> ExtractLayer(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<const FBXNode*, const FBXNode*> getNodeParent);

			// 'AnimationLayer' has 'AnimationCurveNode' as children and this one tends to those:
			bool ExtractCurveNode(const FBXObjectIndex::NodeWithConnections& node, AnimationClip::Writer& writer, OS::Logger* logger, float rootScale, const Matrix4& rootAxisWrangle
				, Function<TransformInfo, FBXUid> getNodeById
				, Function<const FBXNode*, const FBXNode*> getNodeParent
				, float& minFrameTime);

			// 'AnimationCurveNode' consists of ''AnimationCurve' nodes and here we analyze those:
			bool ExtractCurve(const FBXObjectIndex::NodeWithConnections& node, float defaultValue, TimelineCurve<float, BezierNode<float>>& curve, OS::Logger* logger
				, float& minFrameTime);

			// ExtractCurve extracts data, filling the buffers, but this one actually makes sense of those buffers:
			bool FillCurve(OS::Logger* logger, TimelineCurve<float, BezierNode<float>>& curve)const;

			// Once we have all the curves, we find the 'Start' frame for the animation and shift all the curvess by that amount:
			void FixAnimationTimes(AnimationClip::Writer& writer, float minFrameTime);
		};
	}
}
