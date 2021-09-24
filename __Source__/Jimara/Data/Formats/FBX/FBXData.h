#pragma once
#include "FBXContent.h"
#include <unordered_map>


namespace Jimara {
	class FBXData : public virtual Object {
	public:
		static Reference<FBXData> Extract(const FBXContent* sourceContent, OS::Logger* logger);

	private:
	};
}
