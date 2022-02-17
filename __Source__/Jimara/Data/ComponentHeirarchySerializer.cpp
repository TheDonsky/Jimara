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
		private:
			mutable size_t m_parentComponentIndex = 0;
			mutable size_t m_childIndex = 0;

		public:
			const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			mutable std::vector<SerializerAndParentId> objects;

			inline ChildCollectionSerializer() : ItemSerializer("Node", "Component Heirarchy node") {}

			inline void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const override {
				// Serialize Type name and optionally (re)create the target:
				{
					const ComponentSerializer* serializer = serializers->FindSerializerOf(target);
					if (serializer == nullptr) 
						serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
					assert(serializer != nullptr);

					std::string_view typeName = serializer->TargetComponentType().Name();
					{
						static const Reference<const Serialization::ItemSerializer::Of<std::string_view>> typeNameSerializer =
							Serialization::ValueSerializer<std::string_view>::Create("Type", "Type name of the component");
						recordElement(typeNameSerializer->Serialize(typeName));
						if (typeName.empty())
							typeName = TypeId::Of<Component>().Name();
					}
					
					if (typeName != serializer->TargetComponentType().Name()) {
						Reference<Component> parentComponent =
							(target != nullptr) ? Reference<Component>(target->Parent()) :
							(objects.size() > m_parentComponentIndex) ? objects[m_parentComponentIndex].component : nullptr;
						if (parentComponent != nullptr) {
							serializer = serializers->FindSerializerOf(typeName);
							Reference<Component> newTarget = (serializer == nullptr) ? serializer->CreateComponent(parentComponent) : nullptr;
							if (newTarget == nullptr) {
								newTarget = Object::Instantiate<Component>("Component", parentComponent);
								serializer = TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>();
							}
							if (target != nullptr) {
								while (target->ChildCount() > 0)
									target->GetChild(0)->SetParent(newTarget);
								target->Destroy();
							}
							target = newTarget;
						}
						else if (target == nullptr) return;
					}

					assert(serializer != nullptr);
					assert(target != nullptr);
					objects.push_back(SerializerAndParentId(serializer, target, m_parentComponentIndex));
				}
				
				uint32_t childCount = static_cast<uint32_t>(target->ChildCount());
				{
					static const Reference<const Serialization::ItemSerializer::Of<uint32_t>> childCountSerializer =
						Serialization::ValueSerializer<uint32_t>::Create("Child Count", "Number of children of the component");
					recordElement(childCountSerializer->Serialize(childCount));
				}
				
				uint32_t parentIndex = static_cast<uint32_t>(objects.size() - 1);
				for (uint32_t i = 0; i < childCount; i++) {
					m_parentComponentIndex = parentIndex;
					m_childIndex = i;
					recordElement(Serialize((i < target->ChildCount()) ? target->GetChild(i) : nullptr));
				}
			}
		};
	}

	void ComponentHeirarchySerializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Component* target)const {
		// 0. Collect all objects & their serializers:
		ChildCollectionSerializer childCollectionSerializer;
		childCollectionSerializer.GetFields(recordElement, target);

		// 1. Collect serialized data per object:
		for (size_t i = 0; i < childCollectionSerializer.objects.size(); i++) {
			SerializerAndParentId object = childCollectionSerializer.objects[i];
			recordElement(object.serializer->Serialize(object.component));
			// TODO: Implement this crap! (override recordElement to translate Component and Resource/Asset references into indices and GUIDs)
		}
	}
}
