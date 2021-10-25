#pragma once
#include "FBXObjects.h"
#include "FBXObjectIndex.h"
#include "../../Animation.h"


namespace Jimara {
	namespace FBXHelpers {
		class FBXAnimationExtractor {
		public:
			bool Extract(const FBXObjectIndex& objectIndex, OS::Logger* logger, Callback<FBXAnimation*> onAnimationFound);

		private:
			std::vector<int64_t> m_timeBuffer;
			std::vector<float> m_valueBuffer;
			std::vector<int64_t> m_attrFlagBuffer;
			std::vector<float> m_dataBuffer;
			std::vector<size_t> m_refCountBuffer;

			Reference<FBXAnimation> ExtractLayer(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger);
			bool ExtractCurveNode(const FBXObjectIndex::NodeWithConnections& node, AnimationClip::Writer& writer, OS::Logger* logger);
			Reference<ParametricCurve<float, float>> ExtractCurve(const FBXObjectIndex::NodeWithConnections& node, float defaultValue, OS::Logger* logger);
			Reference<ParametricCurve<float, float>> CreateCurve(OS::Logger* logger)const;
		};
	}
}
