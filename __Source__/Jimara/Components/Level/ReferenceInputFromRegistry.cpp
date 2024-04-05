#include "ReferenceInputFromRegistry.h"


namespace Jimara {
	template<> void TypeIdDetails::GetParentTypesOf<ComponentFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<ReferenceInputFromRegistry<Component>>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<ComponentFromRegistry>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<RegistryEntry>(
			"Component From Registry", "Jimara/Level/ComponentFromRegistry", "Component reference input from Registry");
		report(factory);
	}
}
