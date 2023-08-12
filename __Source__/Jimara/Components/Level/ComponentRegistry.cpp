#include "ComponentRegistry.h"

namespace Jimara {
	template<> void TypeIdDetails::GetParentTypesOf<ComponentRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<Registry>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<ComponentRegistry>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<ComponentRegistry>(
			"Registry Component", "Jimara/Level/Registry", "Registry, that is also a Component");
		report(factory);
	}
}
