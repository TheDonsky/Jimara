#include "ComponentHeirarchySerializer.h"


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

			inline ChildCollectionSerializer() : ItemSerializer("Node", "Component Heirarchy node") {}

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
					
					if (m_typeName != serializer->TargetComponentType().Name()) {
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
				{
					GUID result = {};
					memset(result.bytes, 0, GUID::NUM_BYTES);
					return result;
				}
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
				return nullptr;
			}

		public:
			inline TreeComponentSerializer() : ItemSerializer("_Component_") {}

			inline static const TreeComponentSerializer* Instance() {
				static const TreeComponentSerializer instance;
				return &instance;
			}

			inline void GetFields(const Callback<Serialization::SerializedObject>& recordElement, std::pair<const ChildCollectionSerializer*, size_t>* target)const override {
				const ChildCollectionSerializer* childCollectionSerializer = target->first;
				SerializerAndParentId object = childCollectionSerializer->objects[target->second];

				void(*recordOverrideFn)(std::pair<const ChildCollectionSerializer*, const Callback<Serialization::SerializedObject>*>*, Serialization::SerializedObject) =
					[](std::pair<const ChildCollectionSerializer*, const Callback<Serialization::SerializedObject>*>* data, Serialization::SerializedObject serializedObject) {
					const Serialization::ObjectReferenceSerializer* const serializer = serializedObject.As<Serialization::ObjectReferenceSerializer>();
					if (serializer != nullptr) {
						const Reference<Object> currentObject = serializer->GetObjectValue(serializedObject.TargetAddr());
						const ChildCollectionSerializer* childCollectionSerializer = data->first;
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
					else (*data->second)(serializedObject);
				};

				std::pair<const ChildCollectionSerializer*, const Callback<Serialization::SerializedObject>*> data(childCollectionSerializer, &recordElement);
				object.serializer->GetFields(Callback<Serialization::SerializedObject>(recordOverrideFn, &data), object.component);
			}
		};
	}

	void ComponentHeirarchySerializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const {
		// Collect all objects & their serializers:
		ChildCollectionSerializer childCollectionSerializer;
		childCollectionSerializer.GetFields(recordElement, target);

		// Collect serialized data per object:
		for (size_t i = 0; i < childCollectionSerializer.objects.size(); i++) {
			std::pair<const ChildCollectionSerializer*, size_t> args(&childCollectionSerializer, i);
			recordElement(TreeComponentSerializer::Instance()->Serialize(args));
		}
	}
}
