#include "SphereCollider.h"


namespace Jimara {
	SphereCollider::SphereCollider(Component* parent, const std::string_view& name, float radius, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_radius(radius) {}

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
		const Physics::SphereShape shape(m_radius * max(scale.x, max(scale.y, scale.z)));
		Physics::PhysicsSphereCollider* sphere = dynamic_cast<Physics::PhysicsSphereCollider*>(old);
		if (sphere != nullptr) {
			sphere->Update(shape);
			sphere->SetMaterial(m_material);
			return sphere;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
