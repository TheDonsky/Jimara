#include "BoxCollider.h"


namespace Jimara {
	BoxCollider::BoxCollider(Component* parent, const std::string_view& name, const Vector3& size, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_size(size) {}

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
