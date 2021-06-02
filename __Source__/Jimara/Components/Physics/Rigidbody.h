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

		/// <summary> Mass of the body </summary>
		float Mass()const;

		/// <summary>
		/// Updates the mass of the body
		/// </summary>
		/// <param name="mass"> New mass </param>
		void SetMass(float mass);

		/// <summary> If true, physics simulation will not effect the object's movement </summary>
		bool IsKinematic()const;

		/// <summary>
		/// Sets kinematic flag
		/// </summary>
		/// <param name="kinematic"> If true, the object will be made kinematic and vice versa </param>
		void SetKinematic(bool kinematic);

		/// <summary> Movement speed vector </summary>
		Vector3 Velocity()const;

		/// <summary>
		/// Sets movement speed
		/// </summary>
		/// <param name="velocity"> New speed </param>
		void SetVelocity(const Vector3& velocity);

		/// <summary> Retrieves currently applied lock flags </summary>
		Physics::DynamicBody::LockMask GetLockFlags()const;

		/// <summary>
		/// Applies constraints, based on given bitmask
		/// </summary>
		/// <param name="mask"> Constraint bitmask </param>
		void SetLockFlags(Physics::DynamicBody::LockMask mask);

		virtual void PrePhysicsSynch()override;

		virtual void PostPhysicsSynch()override;


	private:
		mutable Reference<Physics::DynamicBody> m_dynamicBody;
		mutable Matrix4 m_lastPose = Math::Identity();
		bool m_dead = false;

		Physics::DynamicBody* GetBody()const;

		void ClearWhenDestroyed(Component*);

		friend class Collider;
	};
}
