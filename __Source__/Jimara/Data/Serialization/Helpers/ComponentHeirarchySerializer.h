#pragma once
#include "../../../Environment/Scene/Scene.h"

namespace Jimara {
	struct ComponentHeirarchySerializerInput;

	/// <summary>
	/// Serializer for storing and/or loading component heirarchies (could work for levels/prefabs)
	/// </summary>
	class ComponentHeirarchySerializer : public virtual Serialization::SerializerList::From<ComponentHeirarchySerializerInput> {
	public:
		/// <summary> Information about resource loading progress </summary>
		struct ProgressInfo {
			/// <summary> Number of resources to load </summary>
			size_t numResources = 0;

			/// <summary> Number of resources already loaded </summary>
			size_t numLoaded = 0;
		};

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
		/// <param name="input"> Input and configuration </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ComponentHeirarchySerializerInput* input)const override;
	};

	/// <summary>
	/// Input for ComponentHeirarchySerializer
	/// </summary>
	struct ComponentHeirarchySerializerInput {
		/// <summary> Root of the component heirarchy </summary>
		Reference<Component> rootComponent = nullptr;

		/// <summary> 
		/// Once the serializer becomes aware of the set of resources spowned objects will need, 
		/// it'll start loading those and reporting progress through this callback 
		/// </summary>
		Callback<ComponentHeirarchySerializer::ProgressInfo> reportProgress = Callback(Unused<ComponentHeirarchySerializer::ProgressInfo>);
	};
}
