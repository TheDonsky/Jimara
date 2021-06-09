#pragma once
#include "Collider.h"


namespace Jimara {
	class BoxCollider : public virtual Collider {
	public:
		BoxCollider(Component* parent, const std::string_view& name = "Box", const Vector3& size = Vector3(1.0f), Physics::PhysicsMaterial* material = nullptr);

		Vector3 Size()const;

		void SetSize(const Vector3& size);

		Physics::PhysicsMaterial* Material()const;

		void SetMaterial(Physics::PhysicsMaterial* material);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		Reference<Physics::PhysicsMaterial> m_material;
		Vector3 m_size;
	};
}
