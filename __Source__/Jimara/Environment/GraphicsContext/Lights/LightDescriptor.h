#pragma once
#include "../../Core/Object.h"
#include "../../Math/Math.h"

namespace Jimara {
	class LightDescriptor : public virtual Object {
	public:
		struct LightInfo {
			uint32_t typeId;
			const void* data;
			size_t dataSize;
		};

		virtual LightInfo GetLightInfo()const = 0;

		virtual AABB GetLightBounds()const = 0;
	};
}
