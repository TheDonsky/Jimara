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
			uint32_t parentIndex;

			inline SerializerAndParentId(const ComponentSerializer* ser = nullptr, Component* target = nullptr, uint32_t id = 0)
				: serializer(ser), component(target), parentIndex(id) {}
		};

		class ChildCollectionSerializer : public virtual Serialization::SerializerList::From<Component> {
		public:
			const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			mutable std::vector<SerializerAndParentId> objects;

			inline ChildCollectionSerializer() : ItemSerializer("Children", "Children of a Component") {}

			inline void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
				if (target == nullptr) return;
				uint32_t childCount = static_cast<uint32_t>(target->ChildCount());
				{
					static const Reference<const Serialization::ItemSerializer::Of<uint32_t>> childCountSerializer =
						Serialization::ValueSerializer<uint32_t>::Create("Child Count", "Number of children of the component");
					recordElement(childCountSerializer->Serialize(childCount));
				}
				uint32_t parentIndex = static_cast<uint32_t>(objects.size());
				for (uint32_t i = 0; i < childCount; i++) {
					SerializerAndParentId object;
					object.component = (i < target->ChildCount()) ? target->GetChild(i) : nullptr;
					object.serializer = serializers->FindSerializerOf(object.component);
					object.parentIndex = parentIndex;
					const std::string_view originalTypeName = (object.serializer == nullptr) ? "" : object.serializer->TargetComponentType().Name();
					std::string_view typeName = originalTypeName;
					{
						static const Reference<const Serialization::ItemSerializer::Of<std::string_view>> typeNameSerializer =
							Serialization::ValueSerializer<std::string_view>::Create("Child Type", "Type name of a child component");
						recordElement(typeNameSerializer->Serialize(typeName));
					}
					if (typeName != originalTypeName) {
						// TODO: Delete old child and replace with a new one...
					}
					if (object.component != nullptr) {
						objects.push_back(object);
						recordElement(Serialize(object.component));
					}
				}
			}
		};
	}

	void ComponentHeirarchySerializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const {
		// TODO: Implement this crap!

		// 0. Collect all objects & their serializers...
		ChildCollectionSerializer childCollectionSerializer;
		childCollectionSerializer.GetFields(recordElement, target);

		// 1. Collect serialized data per object...
	}
}