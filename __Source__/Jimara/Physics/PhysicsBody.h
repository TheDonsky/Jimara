#pragma once
#include "PhysicsCollider.h"

namespace Jimara {
	namespace Physics {
		/// <summary>
		/// A collection of colliders and triggers, that can be a part of physics simulation
		/// </summary>
		class PhysicsBody : public virtual Object {
		public:
			/// <summary> If true, the body is currently an active part of the scene </summary>
			virtual bool Active()const = 0;

			/// <summary>
			/// Activates/deactivates the body
			/// </summary>
			/// <param name="active"> If true, the body will appear in the next simulation </param>
			virtual void SetActive(bool active) = 0;

			/// <summary> Position and rotation of the body within the scene </summary>
			virtual Matrix4 GetPose()const = 0;

			/// <summary>
			/// Repositions the body
			/// </summary>
			/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
			virtual void SetPose(const Matrix4& transform) = 0;

			/// <summary>
			/// Adds a box collider
			/// </summary>
			/// <param name="box"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsBoxCollider> AddCollider(const BoxShape& box, PhysicsMaterial* material, bool enabled = true) = 0;

			/// <summary>
			/// Adds a sphere collider
			/// </summary>
			/// <param name="sphere"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsSphereCollider> AddCollider(const SphereShape& sphere, PhysicsMaterial* material, bool enabled = true) = 0;

			/// <summary>
			/// Adds a capsule collider
			/// </summary>
			/// <param name="capsule"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsCapsuleCollider> AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, bool enabled = true) = 0;
		};

		/// <summary>
		/// Dynamic body (rigidbody/body that is effected by physics/whatever...)
		/// </summary>
		class DynamicBody : public virtual PhysicsBody {
		public:
			/// <summary> Mass of the body </summary>
			virtual float Mass()const = 0;

			/// <summary>
			/// Updates the mass of the body
			/// </summary>
			/// <param name="mass"> New mass </param>
			virtual void SetMass(float mass) = 0;

			/// <summary> If true, physics simulation will not effect the object's movement </summary>
			virtual bool IsKinematic()const = 0;

			/// <summary>
			/// Sets kinematic flag
			/// </summary>
			/// <param name="kinematic"> If true, the object will be made kinematic and vice versa </param>
			virtual void SetKinematic(bool kinematic) = 0;

			/// <summary> Movement speed vector </summary>
			virtual Vector3 Velocity()const = 0;

			/// <summary>
			/// Sets movement speed
			/// </summary>
			/// <param name="velocity"> New speed </param>
			virtual void SetVelocity(const Vector3& velocity) = 0;
		};

		/// <summary>
		/// Static body (Obstacle/trigger/simple collider collection)
		/// </summary>
		class StaticBody : public virtual PhysicsBody {
		public:
		};
	}
}
