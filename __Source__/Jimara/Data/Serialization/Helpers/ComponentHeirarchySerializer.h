#pragma once
#include "../../../Environment/Scene/Scene.h"

namespace Jimara {
	/// <summary>
	/// Serializer for storing and/or loading component heirarchies (could work for levels/prefabs)
	/// </summary>
	class ComponentHeirarchySerializer : public virtual Serialization::SerializerList::From<Component> {
	public:
		/// <summary> Singleton instance of a ComponentHeirarchySerializer (feel free to create more, but this one's always there for you) </summary>
		static const ComponentHeirarchySerializer* Instance();

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Serializer Name </param>
		/// <param name="hint"> Serializer hint </param>
		/// <param name="attributes"> Serializer attributes </param>
		ComponentHeirarchySerializer(
			const std::string_view& name = "ComponentHeirarchySerializer",
			const std::string_view& hint = "Serializer for a component heirarchy (scenes/prefabs and alike)",
			const std::vector<Reference<const Object>>& attributes = {});

		/// <summary>
		/// Serializes entire component subtree
		/// </summary>
		/// <param name="recordElement"> Each field and component data will be reported through this callback </param>
		/// <param name="target"> Serialized subtree root component </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override;
	};
}
