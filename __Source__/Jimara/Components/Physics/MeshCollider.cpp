#include "MeshCollider.h"


namespace Jimara {
	MeshCollider::MeshCollider(Component* parent, const std::string_view& name, const TriMesh* mesh, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_mesh(mesh) {}

	const TriMesh* MeshCollider::Mesh()const { return m_mesh; }

	void MeshCollider::SetMesh(const TriMesh* mesh) {
		if (m_mesh == mesh) return;
		m_mesh = mesh;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* MeshCollider::Material()const { return m_material; }

	void MeshCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> MeshCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::MeshShape shape(m_mesh, scale);
		if (shape.mesh == nullptr) return nullptr;
		Physics::PhysicsMeshCollider* mesh = dynamic_cast<Physics::PhysicsMeshCollider*>(old);
		if (mesh != nullptr) {
			mesh->Update(shape);
			mesh->SetMaterial(m_material);
			return mesh;
		}
		else {
			Physics::StaticBody* staticBody = dynamic_cast<Physics::StaticBody*>(body);
			if (staticBody == nullptr) {
				Context()->Log()->Warning("MeshCollider::GetPhysicsCollider - Mesh colliders can only be attached to static bodies!");
				return nullptr;
			}
			else return staticBody->AddCollider(shape, m_material, listener, true);
		}
	}
}

