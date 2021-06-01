#pragma once
#include "Collider.h"


namespace Jimara {
	class BoxCollider : public virtual Collider {
	public:
		BoxCollider(Component* parent, const std::string_view& name = "Box", const Vector3& size = Vector3(1.0f));

		Vector3 Size()const;

		void SetSize(const Vector3& size);

	protected:
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) override;

	private:
		Vector3 m_size;
	};
}
