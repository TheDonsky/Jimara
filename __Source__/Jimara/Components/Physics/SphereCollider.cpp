#include "SphereCollider.h"


namespace Jimara {
	SphereCollider::SphereCollider(Component* parent, const std::string_view& name, float radius, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_radius(radius) {}

	namespace {
		class SphereColliderSerializer : public virtual ComponentSerializer::Of<SphereCollider> {
		public:
			inline SphereColliderSerializer()
				: ItemSerializer("Jimara/Physics/SphereCollder", "Sphere Collider component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SphereCollider* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);

				static const Reference<const FieldSerializer> colorSerializer = Serialization::FloatSerializer::For<SphereCollider>(
					"Radius", "Sphere radius",
					[](SphereCollider* target) { return target->Radius(); },
					[](const float& value, SphereCollider* target) { target->SetRadius(value); });
				recordElement(colorSerializer->Serialize(target));
			}

			inline static const ComponentSerializer* Instance() {
				static const SphereColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SphereCollider>(const Callback<const Object*>& report) { report(SphereColliderSerializer::Instance()); }

	float SphereCollider::Radius()const { return m_radius; }

	void SphereCollider::SetRadius(float value) {
		if (m_radius == value) return;
		m_radius = value;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* SphereCollider::Material()const { return m_material; }

	void SphereCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> SphereCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::SphereShape shape(std::abs(m_radius) * max(std::abs(scale.x), max(std::abs(scale.y), std::abs(scale.z))));
		Physics::PhysicsSphereCollider* sphere = dynamic_cast<Physics::PhysicsSphereCollider*>(old);
		if (sphere != nullptr) {
			sphere->Update(shape);
			sphere->SetMaterial(m_material);
			return sphere;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
