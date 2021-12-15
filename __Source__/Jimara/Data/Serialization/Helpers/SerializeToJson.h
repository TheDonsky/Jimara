#pragma once
#include "../ItemSerializers.h"
#include "../../../OS/Logging/Logger.h"
#pragma warning(disable: 26819)
#pragma warning(disable: 26495)
#pragma warning(disable: 28020)
#include "nlohmann/json.hpp"
#pragma warning(default: 26819)
#pragma warning(default: 28020)
#pragma warning(default: 26495)


namespace Jimara {
	namespace Serialization {
		nlohmann::json SerializeToJson(const SerializedObject& object, OS::Logger* logger, bool& error,
			const Function<nlohmann::json, const SerializedObject&, bool&>& serializerObjectPtr);
	}
}
