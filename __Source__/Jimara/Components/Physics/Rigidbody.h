#pragma once
#include "../Component.h"
#include "../Transform.h"
#include "../Interfaces/PhysicsUpdaters.h"


namespace Jimara {
	class Collider;

	class Rigidbody : public virtual Component, public virtual PrePhysicsSynchUpdater, public virtual PostPhysicsSynchUpdater {
	public:
		Rigidbody(Component* parent, const std::string_view& name = "Rigidbody");

		virtual ~Rigidbody();

		virtual void PrePhysicsSynch()override;

		virtual void PostPhysicsSynch()override;


	private:
		Reference<Physics::RigidBody> m_body;
		Matrix4 m_lastPose = Math::Identity();
		bool m_dead = false;

		void ClearWhenDestroyed(Component*);

		friend class Collider;
	};
}
