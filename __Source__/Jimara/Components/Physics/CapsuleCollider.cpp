#include "CapsuleCollider.h"


namespace Jimara {
	CapsuleCollider::CapsuleCollider(Component* parent, const std::string_view& name, float radius, float height) 
		: Component(parent, name), m_capsule(radius, height) {}

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

	Reference<Physics::PhysicsCollider> CapsuleCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) {
		Physics::CapsuleShape shape(m_capsule.radius * max(scale.x, max(scale.y, scale.z)), m_capsule.height * scale.y);
		Physics::PhysicsCapsuleCollider* box = dynamic_cast<Physics::PhysicsCapsuleCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			return box;
		}
		else return body->AddCollider(shape, nullptr, true);
	}
}
