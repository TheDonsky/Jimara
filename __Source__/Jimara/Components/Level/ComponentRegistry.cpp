#include "ComponentRegistry.h"

namespace Jimara {
	template<> void TypeIdDetails::GetParentTypesOf<ComponentRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<Registry>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<ComponentRegistry>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ComponentRegistry> serializer("Jimara/Scene/Registry", "Registry Component");
		report(&serializer);
	}
}
