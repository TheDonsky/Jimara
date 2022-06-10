#include "ComponentHeirarchySerializer.h"
#include "../../../Core/Collections/ThreadPool.h"
#include <tuple>


namespace Jimara {
	const ComponentHeirarchySerializer* ComponentHeirarchySerializer::Instance() {
		static const Reference<const ComponentHeirarchySerializer> instance = Object::Instantiate<ComponentHeirarchySerializer>();
		return instance;
	}

	ComponentHeirarchySerializer::ComponentHeirarchySerializer(
		const std::string_view& name,
		const std::string_view& hint, 
		const std::vector<Reference<const Object>>& attributes)
		: ItemSerializer(name, hint, attributes) {}

	namespace {
		struct ResourceCollection {
			std::vector<GUID> guids;
			std::unordered_map<GUID, Reference<Resource>> guidCache;
			
			void IncludeResources(const std::vector<Reference<Resource>>& resources) {
				for (size_t i = 0; i < resources.size(); i++) {
					const Reference<Resource> resource = resources[i];
					if (resource == nullptr || (!resource->HasAsset())) return;
					GUID id = resource->GetAsset()->Guid();
					if (guidCache.find(id) == guidCache.end()) {
						guidCache.insert(std::make_pair(id, resource));
						guids.push_back(id);
					}
				}
			}

			void CollectResourceGUIDsFromSerializedObject(Serialization::SerializedObject object) {
				{
					const Serialization::ObjectReferenceSerializer* const serializer = object.As<Serialization::ObjectReferenceSerializer>();
					if (serializer != nullptr) {
						Reference<Object> item = serializer->GetObjectValue(object.TargetAddr());
						Reference<Resource> resource = dynamic_cast<Resource*>(item.operator->());
						if (resource != nullptr && resource->HasAsset()) {
							GUID id = resource->GetAsset()->Guid();
							if (guidCache.find(id) == guidCache.end()) {
								guidCache.insert(std::make_pair(id, resource));
								guids.push_back(id);
							}
						}
						return;
					}
				}
				{
					const Serialization::SerializerList* const serializer = object.As<Serialization::SerializerList>();
					if (serializer != nullptr)
						object.GetFields(Callback(&ResourceCollection::CollectResourceGUIDsFromSerializedObject, this));
				}
			}

			void CollectResourceGUIDs(
				Component* component, const ComponentSerializer::Set* serializers) {
				if (component == nullptr) return;
				{
					const ComponentSerializer* serializer = serializers->FindSerializerOf(component);
					if (serializer != nullptr)
						serializer->GetFields(Callback(&ResourceCollection::CollectResourceGUIDsFromSerializedObject, this), component);
				}
				for (size_t i = 0; i < component->ChildCount(); i++)
					CollectResourceGUIDs(component->GetChild(i), serializers);
			}

			void CollectResources(ComponentHeirarchySerializerInput* input, AssetDatabase* database) {
				if (database == nullptr) return;

				std::vector<Reference<Resource>>& resources = input->resources;
				resources.clear();
				std::vector<Reference<Asset>> assetsToLoad;
				std::vector<Reference<Asset>> assetsWithDependencies;
				for (size_t i = 0; i < guids.size(); i++) {
					const Reference<Asset> asset = database->FindAsset(guids[i]);
					if (asset == nullptr) continue;
					const Reference<Resource> resource = asset->GetLoadedResource();
					if (resource != nullptr) resources.push_back(resource);
					else (asset->HasRecursiveDependencies() ? assetsWithDependencies : assetsToLoad).push_back(asset);
				}
				
				const size_t TOTAL_COUNT = assetsToLoad.size() + assetsWithDependencies.size();
				std::atomic<size_t> countLeft = assetsToLoad.size();
				std::atomic<size_t> resourceIndex = resources.size();
				std::atomic<size_t> totalLoaded = 0;
				size_t totalReported = ~((size_t)0);
				resources.resize(resources.size() + TOTAL_COUNT);

				auto reportProgeress = [&]() -> bool {
					size_t value = totalLoaded.load();
					if (totalReported == value) return false;
					input->reportProgress(ComponentHeirarchySerializer::ProgressInfo(TOTAL_COUNT, value));
					totalReported = value;
					return true;
				};
				auto loadAsset = [&](Asset* asset) {
					Reference<Resource> resource = asset->LoadResource();
					if (resource != nullptr) resources[resourceIndex.fetch_add(1)] = resource;
					totalLoaded.fetch_add(1);
				};
				auto loadOne = [&]() -> bool {
					size_t index = assetsToLoad.size() - countLeft.fetch_sub(1);
					if (index < assetsToLoad.size()) {
						loadAsset(assetsToLoad[index]);
						return true;
					}
					else return false;
				};
				auto loadOnesWithDependencies = [&]() {
					for (size_t i = 0; i < assetsWithDependencies.size(); i++) {
						reportProgeress();
						loadAsset(assetsWithDependencies[i]);
					}
				};
				
				if (input->resourceCountPerLoadWorker <= 0 || input->resourceCountPerLoadWorker > assetsToLoad.size()) {
					loadOnesWithDependencies();
					for (size_t i = 0; i < assetsToLoad.size(); i++) {
						reportProgeress();
						loadOne();
					}
				}
				else {
					const size_t THREAD_COUNT = [&]() -> size_t {
						const size_t baseCount = (assetsToLoad.size() + input->resourceCountPerLoadWorker - 1) / input->resourceCountPerLoadWorker;
						const size_t maxHWThreads = std::thread::hardware_concurrency();
						return (baseCount < maxHWThreads) ? baseCount : maxHWThreads;
					}();
					ThreadPool pool(THREAD_COUNT);
					auto loadFn = [&](Object*) { while (loadOne()); };
					const Callback<Object*> loadCall = Callback<Object*>::FromCall(&loadFn);
					for (size_t i = 0; i < THREAD_COUNT; i++)
						pool.Schedule(loadCall, nullptr);
					loadOnesWithDependencies();
					while (true) {
						reportProgeress();
						if (!loadOne()) break;
					}
				}

				resources.resize(resourceIndex.load());
				if (TOTAL_COUNT > 0)
					reportProgeress();
			}

			class Serializer : public virtual Serialization::SerializerList::From<ResourceCollection> {
			public:
				inline Serializer() : ItemSerializer("Resources", "Resource GUIDs") {}

				inline static const Serializer* Instance() {
					static const Serializer instance;
					return &instance;
				}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ResourceCollection* target)const final override {
					{
						static const Reference<const Serialization::ItemSerializer::Of<uint32_t>> serializer = Serialization::ValueSerializer<uint32_t>::Create("Count", "Resource count");
						uint32_t count = static_cast<uint32_t>(target->guids.size());
						recordElement(serializer->Serialize(count));
						while (target->guids.size() < count)
							target->guids.push_back(GUID{});
						target->guids.resize(count);
					}
					{
						static const Reference<const GUID::Serializer> serializer = Object::Instantiate<GUID::Serializer>("ResourceId", "Resource GUID");
						for (size_t i = 0; i < target->guids.size(); i++)
							recordElement(serializer->Serialize(target->guids[i]));
					}
				}
			};
		};


		struct SerializerAndParentId {
			const ComponentSerializer* serializer;
			Reference<Component> component;

			inline SerializerAndParentId(const ComponentSerializer* ser = nullptr, Component* target = nullptr)
				: serializer(ser), component(target) {}
		};


		// Tree structure serializer
		class ChildCollectionSerializer : public virtual Serialization::SerializerList::From<Component> {
		private:
			mutable uint32_t m_parentComponentIndex = 0;
			mutable size_t m_childIndex = 0;
			mutable std::string m_typeName;

		public:
			const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			mutable std::vector<SerializerAndParentId> objects;
			mutable std::unordered_map<Component*, uint32_t> objectIndex;
			const ComponentHeirarchySerializerInput* const hierarchyInput;

			inline ChildCollectionSerializer(const ComponentHeirarchySerializerInput* input)
				: ItemSerializer("Node", "Component Heirarchy node")
				, hierarchyInput(input) {}

			inline void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
				// Serialize Type name and optionally (re)create the target:
				{
					const ComponentSerializer* serializer = serializers->FindSerializerOf(target);
					if (serializer == nullptr) 
						serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
					assert(serializer != nullptr);

					m_typeName = serializer->TargetComponentType().Name();
					{
						static const Reference<const Serialization::ItemSerializer::Of<std::string>> typeNameSerializer =
							Serialization::ValueSerializer<std::string_view>::For<std::string>("Type", "Type name of the component",
								[](std::string* text) -> std::string_view { return *text; },
								[](const std::string_view& value, std::string* text) { (*text) = value; });
						recordElement(typeNameSerializer->Serialize(&m_typeName));
						if (m_typeName.empty())
							m_typeName = TypeId::Of<Component>().Name();
					}
					
					if (m_typeName != serializer->TargetComponentType().Name() || target == nullptr) {
						Reference<Component> parentComponent =
							(target != nullptr) ? Reference<Component>(target->Parent()) :
							(objects.size() > m_parentComponentIndex) ? objects[m_parentComponentIndex].component : nullptr;
						if (parentComponent != nullptr) {
							serializer = serializers->FindSerializerOf(m_typeName);
							Reference<Component> newTarget = (serializer != nullptr) ? serializer->CreateComponent(parentComponent) : nullptr;
							if (newTarget == nullptr) {
								newTarget = Object::Instantiate<Component>(parentComponent, "Component");
								serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
							}
							if (target != nullptr) {
								m_childIndex = target->IndexInParent();
								while (target->ChildCount() > 0)
									target->GetChild(0)->SetParent(newTarget);
								target->Destroy();
							}
							target = newTarget;
							target->SetIndexInParent(m_childIndex);
						}
						else if (target == nullptr) return;
					}

					assert(serializer != nullptr);
					assert(target != nullptr);
					objectIndex[target] = static_cast<uint32_t>(objects.size());
					objects.push_back(SerializerAndParentId(serializer, target));
				}
				
				// Serialize child count:
				uint32_t childCount = static_cast<uint32_t>(target->ChildCount());
				{
					static const Reference<const Serialization::ItemSerializer::Of<uint32_t>> childCountSerializer =
						Serialization::ValueSerializer<uint32_t>::Create("Child Count", "Number of children of the component");
					recordElement(childCountSerializer->Serialize(childCount));
				}
				
				// Delete "extra" child objects
				{
					size_t i = target->ChildCount();
					while (i > childCount) {
						i--;
						target->GetChild(i)->Destroy();
						if (i == childCount) break;
					}
				}	

				// Serialize child info:
				uint32_t parentIndex = static_cast<uint32_t>(objects.size() - 1);
				for (uint32_t i = 0; i < childCount; i++) {
					m_parentComponentIndex = parentIndex;
					m_childIndex = i;
					recordElement(Serialize((i < target->ChildCount()) ? target->GetChild(i) : nullptr));
				}
			}
		};


		// Serializer for individual components
		class TreeComponentSerializer : public virtual Serialization::SerializerList::From<std::pair<const ChildCollectionSerializer*, size_t>> {
		private:
			inline static GUID GetGUID(Object* object, const ChildCollectionSerializer* collection) {
				{
					Component* component = dynamic_cast<Component*>(object);
					if (component != nullptr) {
						std::unordered_map<Component*, uint32_t>::const_iterator it = collection->objectIndex.find(component);
						if (it != collection->objectIndex.end()) {
							GUID result = {};
							memset(result.bytes, 0, GUID::NUM_BYTES);
							(*reinterpret_cast<uint32_t*>(result.bytes)) = (it->second + 1);
							return result;
						}
					}
				}
				{
					Resource* resource = dynamic_cast<Resource*>(object);
					if (resource != nullptr && resource->HasAsset())
						return resource->GetAsset()->Guid();
				}
				{
					Asset* asset = dynamic_cast<Asset*>(object);
					if (asset != nullptr) return asset->Guid();
				}
				return collection->hierarchyInput->getExternalObjectId(object);
			}

			inline static Reference<Object> GetReference(const GUID& guid, const TypeId& valueType, const ChildCollectionSerializer* collection) {
				{
					static_assert(GUID::NUM_BYTES % (sizeof(uint32_t)) == 0);
					const constexpr size_t numWords = (GUID::NUM_BYTES / (sizeof(uint32_t)));
					const uint32_t* words = reinterpret_cast<const uint32_t*>(guid.bytes);
					bool isComponentIndex = (((*words) > 0) && ((*words) <= collection->objects.size()));
					if (isComponentIndex)
						for (size_t i = 1; i < numWords; i++)
							if (words[i] != 0) {
								isComponentIndex = false;
								break;
							}
					if (isComponentIndex) {
						Reference<Component> component = collection->objects[(*words) - 1].component;
						if (valueType.CheckType(component) && (!component->Destroyed()))
							return component;
					}
				}
				if (collection->objects.size() > 0 && collection->objects[0].component != nullptr) {
					Reference<Asset> asset = collection->objects[0].component->Context()->AssetDB()->FindAsset(guid);
					if (asset != nullptr) {
						if (valueType.CheckType(asset)) 
							return asset;
						Reference<Resource> resource = asset->LoadResource();
						if (valueType.CheckType(resource)) 
							return resource;
					}
				}
				{
					Reference<Object> item = collection->hierarchyInput->getExternalObject(guid);
					if (valueType.CheckType(item)) return item;
				}
				return nullptr;
			}

			class ComponentFieldListSerializer 
				: public virtual Serialization::SerializerList::From<std::pair<const Serialization::SerializedObject*, const ChildCollectionSerializer*>> {
			public:
				inline ComponentFieldListSerializer() : ItemSerializer("ComponentFieldSublist") {}

				inline static const ComponentFieldListSerializer* Instance() {
					static const ComponentFieldListSerializer instance;
					return &instance;
				}

				inline void GetFields(
					const Callback<Serialization::SerializedObject>& recordElement, 
					std::pair<const Serialization::SerializedObject*, const ChildCollectionSerializer*>* target)const override {

					typedef std::pair<const ChildCollectionSerializer*, const Callback<Serialization::SerializedObject>*> RecordOverrideData;
					void(*recordOverrideFn)(const RecordOverrideData*, Serialization::SerializedObject) =
						[](const RecordOverrideData* data, Serialization::SerializedObject serializedObject) {

						const ChildCollectionSerializer* childCollectionSerializer = data->first;
						const Serialization::ObjectReferenceSerializer* const serializer = serializedObject.As<Serialization::ObjectReferenceSerializer>();
						if (serializer != nullptr) {
							const Reference<Object> currentObject = serializer->GetObjectValue(serializedObject.TargetAddr());
							const GUID initialGUID = GetGUID(currentObject, childCollectionSerializer);
							GUID guid = initialGUID;
							{
								static const Reference<const GUID::Serializer> guidSerializer =
									Object::Instantiate<GUID::Serializer>("ReferenceId", "Object, referenced by the component");
								(*data->second)(guidSerializer->Serialize(guid));
							}
							if (guid != initialGUID) {
								Reference<Object> newObject = GetReference(guid, serializer->ReferencedValueType(), childCollectionSerializer);
								serializer->SetObjectValue(newObject, serializedObject.TargetAddr());
							}
						}
						else {
							const Serialization::SerializerList* listSerializer = serializedObject.As<Serialization::SerializerList>();
							if (listSerializer != nullptr) {
								std::pair<const Serialization::SerializedObject*, const ChildCollectionSerializer*> args(&serializedObject, childCollectionSerializer);
								(*data->second)(Instance()->Serialize(args));
							}
							else (*data->second)(serializedObject);
						}
					};
					RecordOverrideData data(target->second, &recordElement);
					target->first->GetFields(Callback<Serialization::SerializedObject>(recordOverrideFn, &data));
				}
			};

		public:
			inline TreeComponentSerializer() : ItemSerializer("Component") {}

			inline static const TreeComponentSerializer* Instance() {
				static const TreeComponentSerializer instance;
				return &instance;
			}

			inline void GetFields(const Callback<Serialization::SerializedObject>& recordElement, std::pair<const ChildCollectionSerializer*, size_t>* target)const override {
				const ChildCollectionSerializer* childCollectionSerializer = target->first;
				SerializerAndParentId object = childCollectionSerializer->objects[target->second];
				const Serialization::SerializedObject serializedObject = object.serializer->Serialize(object.component);
				std::pair<const Serialization::SerializedObject*, const ChildCollectionSerializer*> args(&serializedObject, childCollectionSerializer);
				ComponentFieldListSerializer::Instance()->GetFields(recordElement, &args);
			}
		};

		Reference<Scene::LogicContext> GetContext(ComponentHeirarchySerializerInput* input) {
			return input == nullptr ? nullptr : (input->rootComponent != nullptr ? Reference<Scene::LogicContext>(input->rootComponent->Context()) : input->context);
		};

		template<typename CallType>
		inline static void ExecuteWithUpdateLock(const CallType& call, ComponentHeirarchySerializerInput* input, Scene::LogicContext* context) {
			Reference<Scene::LogicContext> curContext = GetContext(input);
			if (curContext != context) {
				context->Log()->Error("ComponentHeirarchySerializer::GetFields - Context changed mid-serialization!");
				if (curContext == nullptr) {
					curContext = context;
					input->context = curContext;
				}
			}
			if (input->useUpdateQueue) {
				std::pair<const CallType*, Semaphore> args;
				args.first = &call;
				void(*callFn)(std::pair<const CallType*, Semaphore>*, Object*) = [](std::pair<const CallType*, Semaphore>* args, Object* ctx) {
					{
						std::unique_lock<std::recursive_mutex> lock(dynamic_cast<Scene::LogicContext*>(ctx)->UpdateLock());
						(*args->first)();
					}
					args->second.post();
				};
				curContext->ExecuteAfterUpdate(Callback<Object*>(callFn, &args), curContext);
				args.second.wait();
			}
			else {
				std::unique_lock<std::recursive_mutex> lock(curContext->UpdateLock());
				call();
			}
		}
	}

	void ComponentHeirarchySerializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, ComponentHeirarchySerializerInput* input)const {
		const Reference<Scene::LogicContext> context = GetContext(input);
		
		ResourceCollection resources;
		{
			// Include pre-configured resources:
			resources.IncludeResources(input->resources);
		};

		if (input->rootComponent != nullptr && context != nullptr)
			ExecuteWithUpdateLock([&]() {
			if (input->rootComponent == nullptr) return;
			// Collect resource GUIDs
			const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			resources.CollectResourceGUIDs(input->rootComponent, serializers);
				}, input, context);

		{
			// Collect resources:
			recordElement(ResourceCollection::Serializer::Instance()->Serialize(resources));
			Reference<AssetDatabase> database = (context == nullptr) ? input->assetDatabase : Reference<AssetDatabase>(context->AssetDB());
			resources.CollectResources(input, database);
		}

		if (context == nullptr) return;

		ExecuteWithUpdateLock([&]() {
			input->onResourcesLoaded();

			if (input->rootComponent == nullptr && input->context != nullptr)
				input->rootComponent = Object::Instantiate<Component>(input->context->RootObject());

			if (input->rootComponent != nullptr) {
				// Collect all objects & their serializers:
				ChildCollectionSerializer childCollectionSerializer(input);
				recordElement(childCollectionSerializer.Serialize(input->rootComponent));

				// Collect serialized data per object:
				for (size_t i = 0; i < childCollectionSerializer.objects.size(); i++) {
					std::pair<const ChildCollectionSerializer*, size_t> args(&childCollectionSerializer, i);
					recordElement(TreeComponentSerializer::Instance()->Serialize(args));
				}
				// Let the system know about the results:
				input->rootComponent = childCollectionSerializer.objects[0].component;
			}

			input->onSerializationFinished();
			}, input, context);
	}
}
