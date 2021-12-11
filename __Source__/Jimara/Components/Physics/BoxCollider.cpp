#include "BoxCollider.h"


namespace Jimara {
	BoxCollider::BoxCollider(Component* parent, const std::string_view& name, const Vector3& size, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_size(size) {}

	namespace {
		class BoxColliderSerializer : public virtual ComponentSerializer::Of<BoxCollider> {
		public:
			inline BoxColliderSerializer()
				: ItemSerializer("Jimara/Physics/BoxCollider", "Box Collider component") {}

			inline virtual void SerializeTarget(const Callback<Serialization::SerializedObject>& recordElement, BoxCollider* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->SerializeComponent(recordElement, target);

				static const Reference<const Serialization::Vector3Serializer> colorSerializer = Serialization::Vector3Serializer::For<BoxCollider>(
					"Size", "Collider size",
					[](BoxCollider* target) { return target->Size(); },
					[](const Vector3& value, BoxCollider* target) { target->SetSize(value); });
				recordElement(Serialization::SerializedObject(colorSerializer, target));
			}

			inline static const ComponentSerializer* Instance() {
				static const BoxColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<BoxCollider>(const Callback<const Object*>& report) { report(BoxColliderSerializer::Instance()); }

	Vector3 BoxCollider::Size()const { return m_size; }

	void BoxCollider::SetSize(const Vector3& size) {
		if (m_size == size) return;
		m_size = size;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* BoxCollider::Material()const { return m_material; }

	void BoxCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> BoxCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::BoxShape shape(m_size * scale);
		Physics::PhysicsBoxCollider* box = dynamic_cast<Physics::PhysicsBoxCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			box->SetMaterial(m_material);
			return box;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
