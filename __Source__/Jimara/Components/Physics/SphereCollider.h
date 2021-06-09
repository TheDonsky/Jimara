#pragma once
#include "Collider.h"


namespace Jimara {
	class SphereCollider : public Collider {
	public:
		SphereCollider(Component* parent, const std::string_view& name = "Sphere", float radius = 0.5f, Physics::PhysicsMaterial* material = nullptr);

		float Radius()const;

		void SetRadius(float value);

		Physics::PhysicsMaterial* Material()const;

		void SetMaterial(Physics::PhysicsMaterial* material);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		Reference<Physics::PhysicsMaterial> m_material;
		std::atomic<float> m_radius;
	};
}
