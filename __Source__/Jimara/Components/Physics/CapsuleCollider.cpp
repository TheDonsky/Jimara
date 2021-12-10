#include "CapsuleCollider.h"


namespace Jimara {
	CapsuleCollider::CapsuleCollider(Component* parent, const std::string_view& name, float radius, float height, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_capsule(radius, height) {}

	namespace {
		class CapsuleColliderSerializer : public virtual ComponentSerializer {
		public:
			inline CapsuleColliderSerializer()
				: ItemSerializer("CapsuleCollder", "Capsule Collider component"), ComponentSerializer("Jimara/Physics/CapsuleCollider") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override {
				return Object::Instantiate<CapsuleCollider>(parent, "Capsule Collider");
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				CapsuleCollider* target = dynamic_cast<CapsuleCollider*>((Component*)targetAddr);
				target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

				static const Reference<const Serialization::FloatSerializer> colorSerializer = Serialization::FloatSerializer::Create(
					"Radius", "Capsule radius",
					Function<float, void*>([](void* targetAddr) { return dynamic_cast<CapsuleCollider*>((Component*)targetAddr)->Radius(); }),
					Callback<const float&, void*>([](const float& value, void* targetAddr) { dynamic_cast<CapsuleCollider*>((Component*)targetAddr)->SetRadius(value); }));
				recordElement(Serialization::SerializedObject(colorSerializer, targetAddr));

				static const Reference<const Serialization::FloatSerializer> heightSerializer = Serialization::FloatSerializer::Create(
					"Height", "Capsule height",
					Function<float, void*>([](void* targetAddr) { return dynamic_cast<CapsuleCollider*>((Component*)targetAddr)->Height(); }),
					Callback<const float&, void*>([](const float& value, void* targetAddr) { dynamic_cast<CapsuleCollider*>((Component*)targetAddr)->SetHeight(value); }));
				recordElement(Serialization::SerializedObject(heightSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const CapsuleColliderSerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry CAPSULE_COLLIDER_SERIALIZER;
	}

	Reference<const ComponentSerializer> CapsuleCollider::GetSerializer()const {
		return CapsuleColliderSerializer::Instance();
	}

	template<> void TypeIdDetails::OnRegisterType<CapsuleCollider>() { CAPSULE_COLLIDER_SERIALIZER = CapsuleColliderSerializer::Instance(); }
	template<> void TypeIdDetails::OnUnregisterType<CapsuleCollider>() { CAPSULE_COLLIDER_SERIALIZER = nullptr; }

	float CapsuleCollider::Radius()const { return m_capsule.radius; }

	void CapsuleCollider::SetRadius(float value) {
		if (m_capsule.radius == value) return;
		m_capsule.radius = value;
		ColliderDirty();
	}

	float CapsuleCollider::Height()const { return m_capsule.height; }

	void CapsuleCollider::SetHeight(float value) {
		if (m_capsule.height == value) return;
		m_capsule.height = value;
		ColliderDirty();
	}

	Physics::CapsuleShape::Alignment CapsuleCollider::Alignment()const { return m_capsule.alignment; }

	void CapsuleCollider::SetAlignment(Physics::CapsuleShape::Alignment alignment) {
		if (m_capsule.alignment == alignment) return;
		m_capsule.alignment = alignment;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* CapsuleCollider::Material()const { return m_material; }

	void CapsuleCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> CapsuleCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		Physics::CapsuleShape shape(m_capsule.radius * max(scale.x, max(scale.y, scale.z)), m_capsule.height * scale.y, m_capsule.alignment);
		Physics::PhysicsCapsuleCollider* capsule = dynamic_cast<Physics::PhysicsCapsuleCollider*>(old);
		if (capsule != nullptr) {
			capsule->Update(shape);
			capsule->SetMaterial(m_material);
			return capsule;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
