#pragma once
#include "Rigidbody.h"
#include "../Interfaces/PhysicsUpdaters.h"


namespace Jimara {
	class Collider : public virtual Component, public virtual PrePhysicsSynchUpdater {
	public:
		virtual void PrePhysicsSynch() override;

	protected:
		void ColliderDirty();

		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) = 0;

	private:
		Reference<Rigidbody> m_rigidbody;
		Reference<Physics::PhysicsBody> m_body;
		Reference<Physics::PhysicsCollider> m_collider;
		Matrix4 m_lastPose;
		Vector3 m_lastScale;
		std::atomic<bool> m_dirty = true;
	};
}

