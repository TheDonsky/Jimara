#pragma once
#include "../Environment/Scene/Scene.h"

namespace Jimara {
	class ComponentHeirarchySerializer : public virtual Serialization::SerializerList::From<Component> {
	public:
		static const ComponentHeirarchySerializer* Instance();

		ComponentHeirarchySerializer(
			const std::string_view& name = "ComponentHeirarchySerializer",
			const std::string_view& hint = "Serializer for a component heirarchy (scenes/prefabs and alike)",
			const std::vector<Reference<const Object>>& attributes = {});

		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override;
	};
}
