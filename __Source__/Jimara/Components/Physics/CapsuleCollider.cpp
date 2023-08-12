#include "CapsuleCollider.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	CapsuleCollider::CapsuleCollider(Component* parent, const std::string_view& name, float radius, float height, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_capsule(radius, height) {}

	template<> void TypeIdDetails::GetTypeAttributesOf<CapsuleCollider>(const Callback<const Object*>& report) { 
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<CapsuleCollider>(
			"Capsule Collider", "Jimara/Physics/CapsuleCollider", "Capsule-shaped collider");
		report(factory);
	}

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

	void CapsuleCollider::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Capsule radius");
			JIMARA_SERIALIZE_FIELD_GET_SET(Height, SetHeight, "Height", "Capsule height");
			{
				static const auto serializer = Serialization::Uint32Serializer::For<CapsuleCollider>(
					"Alignment", "Capsule orientationt",
					[](CapsuleCollider* target) { return static_cast<uint32_t>(target->Alignment()); },
					[](const uint32_t& value, CapsuleCollider* target) {
						target->SetAlignment(static_cast<Physics::CapsuleShape::Alignment>(value));
					}, { Object::Instantiate<Serialization::Uint32EnumAttribute>(std::vector<Serialization::Uint32EnumAttribute::Choice>({
							Serialization::Uint32EnumAttribute::Choice("X", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::X)),
							Serialization::Uint32EnumAttribute::Choice("Y", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::Y)),
							Serialization::Uint32EnumAttribute::Choice("Z", static_cast<uint32_t>(Physics::CapsuleShape::Alignment::Z))
						}), false) });
				recordElement(serializer->Serialize(this));
			}
			JIMARA_SERIALIZE_FIELD_GET_SET(Material, SetMaterial, "Material", "Physics material");
		};
		
	}

	Reference<Physics::PhysicsCollider> CapsuleCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		Physics::CapsuleShape shape(
			std::abs(m_capsule.radius) * Math::Max(std::abs(scale.x), Math::Max(std::abs(scale.y), std::abs(scale.z))),
			std::abs(m_capsule.height * (
				(m_capsule.alignment == Physics::CapsuleShape::Alignment::X) ? scale.x : 
				(m_capsule.alignment == Physics::CapsuleShape::Alignment::Y) ? scale.y : scale.z)),
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
