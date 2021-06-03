#pragma once
#include "Rigidbody.h"


namespace Jimara {
	class Collider : public virtual Component, public virtual PrePhysicsSynchUpdater {
	public:
		Collider();

		virtual ~Collider();

		/// <summary> True, if the collider is trigger </summary>
		bool IsTrigger()const;

		/// <summary>
		/// Sets trigger flag
		/// </summary>
		/// <param name="trigger"> If true, the collider will be made a trigger </param>
		void SetTrigger(bool trigger);

		virtual void PrePhysicsSynch()override;

	protected:
		void ColliderDirty();

		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) = 0;

	private:
		Reference<Rigidbody> m_rigidbody;
		Reference<Physics::PhysicsBody> m_body;
		Reference<Physics::PhysicsCollider> m_collider;
		Matrix4 m_lastPose = Math::Identity();
		Vector3 m_lastScale = Vector3(1.0f);
		std::atomic<bool> m_isTrigger = false;
		std::atomic<bool> m_dirty = true;
		std::atomic<bool> m_dead = false;

		void ClearWhenDestroyed(Component*);
	};
}

