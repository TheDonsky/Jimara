#pragma once
#include "Collider.h"


namespace Jimara {
	class SphereCollider : public Collider {
	public:
		SphereCollider(Component* parent, const std::string_view& name = "Sphere", float radius = 0.5f);

		float Radius()const;

		void SetRadius(float value);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		std::atomic<float> m_radius;
	};
}
