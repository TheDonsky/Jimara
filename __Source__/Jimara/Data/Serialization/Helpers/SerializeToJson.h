#pragma once
#include "../ItemSerializers.h"
#include "../../../OS/Logging/Logger.h"
#pragma warning(disable: 26819)
#include "nlohmann/json.hpp"
#pragma warning(default: 26819)


namespace Jimara {
	namespace Serialization {
		nlohmann::json SerializeToJson(const SerializedObject& object, OS::Logger* logger, bool& error,
			const Function<nlohmann::json, const SerializedObject&, bool&>& serializerObjectPtr);
	}
}
