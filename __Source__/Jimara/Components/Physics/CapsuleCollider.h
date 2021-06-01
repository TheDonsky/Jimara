#pragma once
#include "Collider.h"


namespace Jimara {
	class CapsuleCollider : public Collider {
	public:
		CapsuleCollider(Component* parent, const std::string_view& name = "Capsule", float radius = 0.5f, float height = 1.0f);

		float Radius()const;

		void SetRadius(float value);

		float Height()const;

		void SetHeight(float value);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		Physics::CapsuleShape m_capsule;
	};
}
