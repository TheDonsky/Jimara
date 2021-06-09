#pragma once
#include "Rigidbody.h"


namespace Jimara {
	/// <summary>
	/// Base class for all component collider types, wrapping round Physics collider objects and making them a proper part of the scene
	/// </summary>
	class Collider : public virtual Component, public virtual PrePhysicsSynchUpdater {
	public:
		/// <summary> Constructor </summary>
		Collider();

		/// <summary> Virtual destructor </summary>
		virtual ~Collider();

		/// <summary> True, if the collider is trigger </summary>
		bool IsTrigger()const;

		/// <summary>
		/// Sets trigger flag
		/// </summary>
		/// <param name="trigger"> If true, the collider will be made a trigger </param>
		void SetTrigger(bool trigger);

		/// <summary> Invoked before physics synch point [Part of the Update cycle; do not invoke by hand] </summary>
		virtual void PrePhysicsSynch()override;

	protected:
		/// <summary> 
		/// Derived classes can use this method to notify the collider, that the underlying Physics::PhysicsCollider is no longer valid and 
		/// should be refreshed using GetPhysicsCollider() before it gets the chance to ruin the simulation
		/// </summary>
		void ColliderDirty();

		/// <summary>
		/// Derived classes should use this method to create and alter the underlying Physics::PhysicsCollider objects to their liking.
		/// Note: Keeping the reference to the toolbox colliders created within this method is not adviced, unless you know exactly how stuff works.
		/// </summary>
		/// <param name="old"> Collider from the previous GetPhysicsCollider() call or nullptr if it did not exist/got invalidated </param>
		/// <param name="body"> Physics body, the collider should be tied to </param>
		/// <param name="scale"> Collider scale, based on transform </param>
		/// <returns> Physics::PhysicsCollider to be used by the collider </returns>
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale) = 0;

	private:
		// Attached rigidbody
		Reference<Rigidbody> m_rigidbody;

		// Body, the underlying collider is tied to
		Reference<Physics::PhysicsBody> m_body;

		// Underlying collider
		Reference<Physics::PhysicsCollider> m_collider;

		// Last pose of the collider
		Matrix4 m_lastPose = Math::Identity();

		// Last scale
		Vector3 m_lastScale = Vector3(1.0f);

		// If true, the underlying collider will be made a trigger
		std::atomic<bool> m_isTrigger = false;

		// If true, GetPhysicsCollider will have to be called before the next physics synch point
		std::atomic<bool> m_dirty = true;

		// If true, OnDestroyed has already been invoked and colliders should not stay allocated
		std::atomic<bool> m_dead = false;

		// Clears objects when OnDestroyed gets invoked
		void ClearWhenDestroyed(Component*);
	};


	/// <summary>
	/// Collider, that uses a single physics material
	/// </summary>
	class SingleMaterialCollider : public virtual Collider {
	public:
		/// <summary> Physics material, used by the collider (nullptr means the default material) </summary>
		virtual Physics::PhysicsMaterial* Material()const = 0;

		/// <summary>
		/// Updates physics material used by the collider (nullptr will result in some default material)
		/// </summary>
		/// <param name="material"> Material to use </param>
		virtual void SetMaterial(Physics::PhysicsMaterial* material) = 0;
	};
}
