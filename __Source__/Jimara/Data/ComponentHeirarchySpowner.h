#pragma once
#include "../Environment/Scene/Scene.h"

namespace Jimara {
	class ComponentHeirarchySpowner : public virtual Resource {
	public:
		virtual std::vector<Reference<Component>> CreateHeirarchy(Component* parent) = 0;
	};

	template<> inline void TypeIdDetails::GetParentTypesOf<ComponentHeirarchySpowner>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }
}
