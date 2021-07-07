#pragma once
#include "../Component.h"
#include "../Transform.h"
#include "../Interfaces/PhysicsUpdaters.h"


namespace Jimara {
	class Collider;

	/// <summary>
	/// Body, effected by physics simulation
	/// </summary>
	class Rigidbody : public virtual Component, public virtual PrePhysicsSynchUpdater, public virtual PostPhysicsSynchUpdater {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		Rigidbody(Component* parent, const std::string_view& name = "Rigidbody");

		/// <summary> Virtual destructor </summary>
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
		Physics::DynamicBody::LockFlagMask GetLockFlags()const;

		/// <summary>
		/// Applies constraints, based on given bitmask
		/// </summary>
		/// <param name="mask"> Constraint bitmask </param>
		void SetLockFlags(Physics::DynamicBody::LockFlagMask mask);


		/// <summary> Invoked before physics synch point [Part of the Update cycle; do not invoke by hand] </summary>
		virtual void PrePhysicsSynch()override;

		/// <summary> Invoked after physics synch point [Part of the Update cycle; do not invoke by hand] </summary>
		virtual void PostPhysicsSynch()override;


	private:
		// Underlying physics body
		mutable Reference<Physics::DynamicBody> m_dynamicBody;

		// Last pose of the body
		mutable Matrix4 m_lastPose = Math::Identity();

		// True, if the component is kinematic
		bool m_kinematic = false;

		// If true, OnDestroyed event has already been invoked and m_dynamicBody has to be empty
		bool m_dead = false;

		// Retrieves the body (if destroyed, this will be nullptr)
		Physics::DynamicBody* GetBody()const;

		// Invoked by OnDestroyed callback to do the cleanup
		void ClearWhenDestroyed(Component*);

		// Collider has to access a few fields... (friend classes are suboptimal, I know, but whatever...)
		friend class Collider;
	};
}
