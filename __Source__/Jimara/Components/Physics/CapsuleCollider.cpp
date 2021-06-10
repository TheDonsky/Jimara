#include "CapsuleCollider.h"


namespace Jimara {
	CapsuleCollider::CapsuleCollider(Component* parent, const std::string_view& name, float radius, float height, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_capsule(radius, height) {}

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
