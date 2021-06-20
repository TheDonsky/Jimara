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
			/// <param name="listener"> Collider event listener </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsBoxCollider> AddCollider(const BoxShape& box, PhysicsMaterial* material, PhysicsCollider::EventListener* listener = nullptr, bool enabled = true) = 0;

			/// <summary>
			/// Adds a sphere collider
			/// </summary>
			/// <param name="sphere"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="listener"> Collider event listener </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsSphereCollider> AddCollider(const SphereShape& sphere, PhysicsMaterial* material, PhysicsCollider::EventListener* listener = nullptr, bool enabled = true) = 0;

			/// <summary>
			/// Adds a capsule collider
			/// </summary>
			/// <param name="capsule"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="listener"> Collider event listener </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsCapsuleCollider> AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, PhysicsCollider::EventListener* listener = nullptr, bool enabled = true) = 0;
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

			/// <summary>
			/// Moves kinematic body to given destination pose
			/// </summary>
			/// <param name="transform"> Destination pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
			virtual void MoveKinematic(const Matrix4& transform) = 0;

			/// <summary>
			/// Some aspects of the simulation can be blocked with these flags
			/// </summary>
			enum class LockFlag : uint8_t {
				/// <summary> Simulation will not effect movement across X axis </summary>
				MOVEMENT_X = (1 << 0),

				/// <summary> Simulation will not effect movement across Y axis </summary>
				MOVEMENT_Y = (1 << 1),

				/// <summary> Simulation will not effect movement across Z axis </summary>
				MOVEMENT_Z = (1 << 2),

				/// <summary> Simulation will not effect rotation around X axis </summary>
				ROTATION_X = (1 << 3),

				/// <summary> Simulation will not effect rotation around Y axis </summary>
				ROTATION_Y = (1 << 4),

				/// <summary> Simulation will not effect rotation around Z axis </summary>
				ROTATION_Z = (1 << 5)
			};

			/// <summary> Bitmask, constructed from LockFlag-s </summary>
			typedef uint8_t LockFlagMask;

			/// <summary> Casts LockFlag to LockFlagMask </summary>
			inline static LockFlagMask LockFlags(LockFlag flag) {
				return static_cast<LockFlagMask>(flag);
			}

			/// <summary>
			/// Builds LockFlagMask form a list of LockFlag-s
			/// </summary>
			/// <typeparam name="...Flags"> Some amount of LockFlag-s </typeparam>
			/// <param name="first"> First flag </param>
			/// <param name="second"> Second flag </param>
			/// <param name="...rest"> Rest of the flags </param>
			/// <returns> Lock flag mask </returns>
			template<typename... Flags>
			inline static LockFlagMask LockFlags(LockFlag first, LockFlag second, Flags... rest) {
				return LockFlagMask(first) | LockFlags(second, rest...);
			}

			/// <summary> Retrieves currently applied lock flags </summary>
			virtual LockFlagMask GetLockFlags()const = 0;

			/// <summary>
			/// Applies constraints, based on given bitmask
			/// </summary>
			/// <param name="mask"> Constraint bitmask </param>
			virtual void SetLockFlags(LockFlagMask mask) = 0;
		};

		/// <summary>
		/// Static body (Obstacle/trigger/simple collider collection)
		/// </summary>
		class StaticBody : public virtual PhysicsBody {
		public:
			/// <summary>
			/// Adds a mesh collider
			/// </summary>
			/// <param name="mesh"> Shape descriptor </param>
			/// <param name="material"> Physics material for the collider </param>
			/// <param name="listener"> Collider event listener </param>
			/// <param name="enabled"> If true, the collider will start-off enabled </param>
			/// <returns> New collider, attached to the body </returns>
			virtual Reference<PhysicsMeshCollider> AddCollider(const MeshShape& mesh, PhysicsMaterial* material, PhysicsCollider::EventListener* listener = nullptr, bool enabled = true) = 0;
		};
	}
}
