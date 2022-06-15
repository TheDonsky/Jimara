#pragma once
#include "../../../Environment/Scene/Scene.h"

namespace Jimara {
	struct ComponentHeirarchySerializerInput;

	/// <summary>
	/// Serializer for storing and/or loading component heirarchies (could work for levels/prefabs)
	/// </summary>
	class JIMARA_API ComponentHeirarchySerializer : public virtual Serialization::SerializerList::From<ComponentHeirarchySerializerInput> {
	public:
		/// <summary> Information about resource loading progress </summary>
		typedef Asset::LoadInfo ProgressInfo;

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
		/// <para />Notes:
		///		<para /> 0. If rootComponent is nullptr, context is required;
		///		<para /> 1. If rootComponent is nullptr, the serializer will create an empty Component 
		///		as a child of the root object and serialize it instead (useful for loading);
		///		<para /> 2. Serialization has two steps: Resource collection and Component serialization; 
		///		after Resource collection, onResourcesLoaded is invoked while holding the update lock; 
		///		Modifying rootComponent from inside that callback is permitted, since Component serialization does not depend on the previous step.
		///		(Use case for this could be, for example, when loading a prefab asynchronously: You could leave rootComponent null, 
		///		then create a placeholder after the resources are loaded to make spowning fully atomic);
		///		<para /> 3. If the serializer decides to recreate the root component because of the type mismatch, this variable will be updated.
		/// </summary>
		Reference<Component> rootComponent = nullptr;

		/// <summary> 
		/// If the root component is initially nullptr, the user has to provide the context 
		/// <para /> Note: If, after invoking onResourcesLoaded, both rootComponent and context are set to nullptr, serialization will exit prematurely 
		///		(You may find this useful when, for example, preloading resources without a need to instantiate anything just yet)
		/// </summary>
		Reference<Scene::LogicContext> context = nullptr;

		/// <summary>
		/// If both rootComponent and context are initially nullptr, that "means" that there is no need to actually serialize data and all you're after is getting the serialized resource set.
		/// In that case, this field can be used to load suff.
		/// <para /> Note: Ignored if any one of the rootComponent or context are provided.
		/// </summary>
		Reference<AssetDatabase> assetDatabase = nullptr;

		/// <summary>
		/// List of all resources the serializer has to load to create a fresh instance of the subtree
		/// <para /> Notes: 
		///		<para /> 0. This does not have to be filled; the serializer will fill it and give a result to the user;
		///		<para /> 1. In case the user provides some resources as an input, they will be included alongside the ones
		///		organically generated from the rootComponent; However, the order can change and ones with no assets will be excluded.
		/// </summary>
		std::vector<Reference<Resource>> resources;

		/// <summary> 
		/// Once the serializer becomes aware of the set of resources spowned objects will need, 
		/// it'll start loading those and reporting progress through this callback 
		/// </summary>
		Callback<ComponentHeirarchySerializer::ProgressInfo> reportProgress = Callback(Unused<ComponentHeirarchySerializer::ProgressInfo>);

		/// <summary>
		/// If the count of resources that need to be preloaded and can be loaded with an external thread exceeds this number,
		/// some amount of worker threads will be allocated to load data in parallel. If this is set to zero, no worker threads will be allocated, no matter what; 
		/// otherwise, up to a point, worker threads will be added per this amount of work.
		/// </summary>
		size_t resourceCountPerLoadWorker = 8;

		/// <summary>
		/// By default, the serializer is only aware of the object references that point to other components within the serialized hierarchy,
		/// as well as the resources/assets from the asset database.
		/// <para/> If the user wants to store/load any pointers outside that domain, getExternalObjectId and getExternalObject can be set.
		/// <para/> Note, that hierarchical and database references will still be examined first, these functions will only be reffered to if those fail or the target object does not qualify.
		/// </summary>
		Function<GUID, Object*> getExternalObjectId = Function<GUID, Object*>([](Object*) { return GUID::Null(); });

		/// <summary>
		/// By default, the serializer is only aware of the object references that point to other components within the serialized hierarchy,
		/// as well as the resources/assets from the asset database.
		/// <para/> If the user wants to store/load any pointers outside that domain, getExternalObjectId and getExternalObject can be set.
		/// <para/> Note, that hierarchical and database references will still be examined first, these functions will only be reffered to if those fail or the target object does not qualify.
		/// </summary>
		Function<Reference<Object>, const GUID&> getExternalObject = Function<Reference<Object>, const GUID&>([](const GUID&) { return Reference<Object>(); });

		/// <summary>
		/// Invoked after Resource collection step
		/// <para /> Note: Feel free to initialize rootComponent inside this function if you wish to ignore resources or are using the 
		///		serializer only for deserialization.
		/// </summary>
		Callback<> onResourcesLoaded = Callback(Unused<>);

		/// <summary>
		/// Invoked after serialization is done
		/// <para /> Note: Only benefit of ever using this one is that it's invoked while the serializer is still holding the update lock and, 
		///		therefore, it's useful for guaranteeing mutual exclusion.
		/// </summary>
		Callback<> onSerializationFinished = Callback(Unused<>);

		/// <summary>
		/// If true, serializer will schedule several steps on ExecuteAfterUpdate() queue and wait for them.
		/// <para /> Notes:
		///		<para /> 0. Calling this from main update thread, naturally, will result in a deadlock; same will happen if Update loop is not running;
		///		<para /> 1. Even if this flag is not set, any thread can use the serializer; this is simply a tool for avoiding hitching;
		///		<para /> 2. This is likely only useful for asynchronous instantiation and switching using this flag will guarantee that the serialization 
		///		will take at least two frames... (only useful when instantiation is not urgent)
		/// </summary>
		bool useUpdateQueue = false;
	};
}
