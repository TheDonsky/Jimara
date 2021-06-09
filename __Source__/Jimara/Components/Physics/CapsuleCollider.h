#pragma once
#include "Collider.h"


namespace Jimara {
	class CapsuleCollider : public Collider {
	public:
		CapsuleCollider(Component* parent, const std::string_view& name = "Capsule", float radius = 0.5f, float height = 1.0f, Physics::PhysicsMaterial* material = nullptr);

		float Radius()const;

		void SetRadius(float value);

		float Height()const;

		void SetHeight(float value);

		Physics::CapsuleShape::Alignment Alignment()const;

		void SetAlignment(Physics::CapsuleShape::Alignment alignment);

		Physics::PhysicsMaterial* Material()const;

		void SetMaterial(Physics::PhysicsMaterial* material);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		Reference<Physics::PhysicsMaterial> m_material;
		Physics::CapsuleShape m_capsule;
	};
}
