#pragma once
#include "FBXObjects.h"
#include "FBXObjectIndex.h"
#include "../../Animation.h"


namespace Jimara {
	namespace FBXHelpers {
		class FBXAnimationExtractor {
		public:
			static bool IsAnimation(const FBXObjectIndex::NodeWithConnections* node);

			Reference<AnimationClip> Extract(const FBXObjectIndex::NodeWithConnections* node, OS::Logger* logger);

		private:

		};
	}
}
