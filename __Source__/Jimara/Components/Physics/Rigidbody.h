#pragma once
#include "../Transform.h"


namespace Jimara {
	class Collider;

	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Rigidbody);

	/// <summary>
	/// Body, effected by physics simulation
	/// </summary>
	class JIMARA_API Rigidbody : public virtual Scene::PhysicsContext::PrePhysicsSynchUpdatingComponent, public virtual Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent {
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

		/// <summary>
		/// True, if CCD(PhysX term; means 'continuous collision detection') is enabled; Depending on the backend, 
		/// this may or may not apply and refer to different things, but, in general, enabling will serve the purpose 
		/// of reducing the probability of a dynamic body "phasing through" colliders due to high velocity and what not.
		/// </summary>
		bool CCDEnabled()const;

		/// <summary>
		/// Enables/Disables continuous collision detection (refer to CCDEnabled() for more details)
		/// </summary>
		/// <param name="enable"> If true, CCD will be enabled </param>
		void EnableCCD(bool enable);

		/// <summary> Movement speed vector </summary>
		Vector3 Velocity()const;

		/// <summary>
		/// Sets movement speed
		/// </summary>
		/// <param name="velocity"> New speed </param>
		void SetVelocity(const Vector3& velocity);

		/// <summary>
		/// Applies force on the body
		/// </summary>
		/// <param name="force"> Force to add </param>
		void AddForce(const Vector3& force);

		/// <summary>
		/// Applies force that will increase velocity by given amount
		/// </summary>
		/// <param name="deltaVelocity"> velocity to add </param>
		void AddVelocity(const Vector3& deltaVelocity);

		/// <summary> Rotation speed </summary>
		Vector3 AngularVelocity()const;

		/// <summary>
		/// Sets rotation speed
		/// </summary>
		/// <param name="velocity"> New rotation speed </param>
		void SetAngularVelocity(const Vector3& velocity);

		/// <summary> Retrieves currently applied lock flags </summary>
		Physics::DynamicBody::LockFlagMask GetLockFlags()const;

		/// <summary>
		/// Applies constraints, based on given bitmask
		/// </summary>
		/// <param name="mask"> Constraint bitmask </param>
		void SetLockFlags(Physics::DynamicBody::LockFlagMask mask);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;


	protected:
		/// <summary> Invoked before physics synch point [Part of the Update cycle; do not invoke by hand] </summary>
		virtual void PrePhysicsSynch()override;

		/// <summary> Invoked after physics synch point [Part of the Update cycle; do not invoke by hand] </summary>
		virtual void PostPhysicsSynch()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, whenever the component gets destroyed </summary>
		virtual void OnComponentDestroyed()override;

	private:
		// Underlying physics body
		mutable Reference<Physics::DynamicBody> m_dynamicBody;

		// Last pose of the body
		mutable Matrix4 m_lastPose = Math::Identity();

		// True, if the component is kinematic
		bool m_kinematic = false;

		// CCD enabled/disabled
		bool m_ccdEnabled = false;

		// "Last known" properties and dirty flags
		Vector3 m_lastVelocity = Vector3(0.0f);
		Vector3 m_velocity = Vector3(0.0f);
		Vector3 m_unappliedVelocity = Vector3(0.0f);
		Vector3 m_angularVelocity = Vector3(0.0f);
		uint32_t m_dirtyFlags = 0u;

		// Retrieves the body (if destroyed, this will be nullptr)
		Physics::DynamicBody* GetBody()const;

		// Collider has to access a few fields... (friend classes are suboptimal, I know, but whatever...)
		friend class Collider;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Rigidbody>(const Callback<TypeId>& report) {
		report(TypeId::Of<Scene::PhysicsContext::PrePhysicsSynchUpdatingComponent>());
		report(TypeId::Of<Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<Rigidbody>(const Callback<const Object*>& report);
}
