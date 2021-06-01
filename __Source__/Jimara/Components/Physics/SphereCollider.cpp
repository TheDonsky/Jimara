#include "SphereCollider.h"


namespace Jimara {
	SphereCollider::SphereCollider(Component* parent, const std::string_view& name, float radius)
		: Component(parent, name), m_radius(radius) {}

	float SphereCollider::Radius()const { return m_radius; }

	void SphereCollider::SetRadius(float value) {
		if (m_radius == value) return;
		m_radius = value;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> SphereCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) {
		const Physics::SphereShape shape(m_radius * max(scale.x, max(scale.y, scale.z)));
		Physics::PhysicsSphereCollider* box = dynamic_cast<Physics::PhysicsSphereCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			return box;
		}
		else return body->AddCollider(shape, nullptr, true);
	}
}
