#include "CapsuleCollider.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	CapsuleCollider::CapsuleCollider(Component* parent, const std::string_view& name, float radius, float height, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_capsule(radius, height) {}

	namespace {
		class CapsuleColliderSerializer : public ComponentSerializer::Of<CapsuleCollider> {
		public:
			inline CapsuleColliderSerializer()
				: ItemSerializer("Jimara/Physics/CapsuleCollder", "Capsule Collider component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, CapsuleCollider* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				{
					static const Reference<const FieldSerializer> serializer = Serialization::FloatSerializer::For<CapsuleCollider>(
						"Radius", "Capsule radius",
						[](CapsuleCollider* target) { return target->Radius(); },
						[](const float& value, CapsuleCollider* target) { target->SetRadius(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::FloatSerializer::For<CapsuleCollider>(
						"Height", "Capsule height",
						[](CapsuleCollider* target) { return target->Height(); },
						[](const float& value, CapsuleCollider* target) { target->SetHeight(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::Uint32Serializer::For<CapsuleCollider>(
						"Alignment", "Capsule orientationt",
						[](CapsuleCollider* target) { return static_cast<uint32_t>(target->Alignment()); },
						[](const uint32_t& value, CapsuleCollider* target) {
							target->SetAlignment(static_cast<Physics::CapsuleShape::Alignment>(value));
						}, { Object::Instantiate<Serialization::Uint32EnumAttribute>(std::vector<Serialization::Uint32EnumAttribute::Choice>({
								Serialization::Uint32EnumAttribute::Choice("X", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::X)),
								Serialization::Uint32EnumAttribute::Choice("Y", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::Y)),
								Serialization::Uint32EnumAttribute::Choice("Z", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::Z))
							}), false) });
					recordElement(serializer->Serialize(target));
				}
			}

			inline static const ComponentSerializer* Instance() {
				static const CapsuleColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<CapsuleCollider>(const Callback<const Object*>& report) { report(CapsuleColliderSerializer::Instance()); }

	float CapsuleCollider::Radius()const { return m_capsule.radius; }

	void CapsuleCollider::SetRadius(float value) {
		if (m_capsule.radius == value) return;
		m_capsule.radius = value;
		ColliderDirty();
	}

	float CapsuleCollider::Height()const { return m_capsule.height; }

	void CapsuleCollider::SetHeight(float value) {
		value = Math::Max(0.0f, value);
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
		Physics::CapsuleShape shape(
			std::abs(m_capsule.radius) * max(std::abs(scale.x), max(std::abs(scale.y), std::abs(scale.z))), 
			std::abs(m_capsule.height * scale.y), 
			m_capsule.alignment);
		Physics::PhysicsCapsuleCollider* capsule = dynamic_cast<Physics::PhysicsCapsuleCollider*>(old);
		if (capsule != nullptr) {
			capsule->Update(shape);
			capsule->SetMaterial(m_material);
			return capsule;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
