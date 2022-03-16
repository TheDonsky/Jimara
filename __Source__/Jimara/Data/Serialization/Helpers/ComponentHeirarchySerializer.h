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
		/// <summary> 
		/// Root of the component heirarchy 
		/// Notes:
		///		0. If rootComponent is nullptr, context is required;
		///		1. If rootComponent is nullptr, the serializer will create an empty Component 
		///		as a child of the root object and serialize it instead (useful for loading);
		///		2. Serialization has two steps: Resource collection and Component serialization; 
		///		after Resource collection, onResourcesLoaded is invoked while holding the update lock; 
		///		Modifying rootComponent from inside that callback is permitted, since Component serialization does not depend on the previous step.
		///		(Use case for this could be, for example, when loading a prefab asynchronously: You could leave rootComponent null, 
		///		then create a placeholder after the resources are loaded to make spowning fully atomic);
		///		3. If the serializer decides to recreate the root component because of the type mismatch, this variable will be updated.
		/// </summary>
		Reference<Component> rootComponent = nullptr;

		/// <summary> 
		/// If the root component is initially nullptr, the user has to provide the context 
		/// Note: If, after invoking onResourcesLoaded, both rootComponent and context are set to nullptr, serialization will exit prematurely 
		///		(You may find this useful when, for example, preloading resources without a need to instantiate anything just yet)
		/// </summary>
		Reference<Scene::LogicContext> context = nullptr;

		/// <summary>
		/// If both rootComponent and context are initially nullptr, that "means" that there is no need to actually serialize data and all you're after is getting the serialized resource set.
		/// In that case, this field can be used to load suff.
		/// Note: Ignored if any one of the rootComponent or context are provided.
		/// </summary>
		Reference<AssetDatabase> assetDatabase = nullptr;

		/// <summary>
		/// List of all resources the serializer has to load to create a fresh instance of the subtree
		/// Notes: 
		///		0. This does not have to be filled; the serializer will fill it and give a result to the user;
		///		1. In case the user provides some resources as an input, they will be included alongside the ones
		///		organically generated from the rootComponent; However, the order can change and ones with no assets will be excluded.
		/// </summary>
		std::vector<Reference<Resource>> resources;

		/// <summary> 
		/// Once the serializer becomes aware of the set of resources spowned objects will need, 
		/// it'll start loading those and reporting progress through this callback 
		/// </summary>
		Callback<ComponentHeirarchySerializer::ProgressInfo> reportProgress = Callback(Unused<ComponentHeirarchySerializer::ProgressInfo>);

		/// <summary>
		/// Invoked after Resource collection step
		/// Note: Feel free to initialize rootComponent inside this function if you wish to ignore resources or are using the 
		///		serializer only for deserialization.
		/// </summary>
		Callback<> onResourcesLoaded = Callback(Unused<>);

		/// <summary>
		/// Invoked after serialization is done
		/// Note: Only benefit of ever using this one is that it's invoked while the serializer is still holding the update lock and, 
		///		therefore, it's useful for guaranteeing mutual exclusion.
		/// </summary>
		Callback<> onSerializationFinished = Callback(Unused<>);

		/// <summary>
		/// If true, serializer will schedule several steps on ExecuteAfterUpdate() queue and wait for them.
		/// Notes:
		///		0. Calling this from main update thread, naturally, will result in a deadlock; same will happen if Update loop is not running;
		///		1. Even if this flag is not set, any thread can use the serializer; this is simply a tool for avoiding hitching;
		///		2. This is likely only useful for asynchronous instantiation and switching using this flag will guarantee that the serialization 
		///		will take at least two frames... (only useful when instantiation is not urgent)
		/// </summary>
		bool useUpdateQueue = false;
	};
}
