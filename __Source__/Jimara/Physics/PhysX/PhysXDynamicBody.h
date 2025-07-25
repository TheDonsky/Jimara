#pragma once
#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
#pragma warning(disable: 4250)
			/// <summary> A simple wrapper on top of physx::PxRigidDynamic </summary>
			class PhysXDynamicBody : public virtual DynamicBody, public virtual PhysXBody {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, this body belongs to </param>
				/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="enabled"> If true, the body will start-off enabled </param>
				PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXDynamicBody();

				/// <summary> Mass of the body </summary>
				virtual float Mass()const override;

				/// <summary>
				/// Updates the mass of the body
				/// </summary>
				/// <param name="mass"> New mass </param>
				virtual void SetMass(float mass) override;

				/// <summary> If true, physics simulation will not effect the object's movement </summary>
				virtual bool IsKinematic()const override;

				/// <summary>
				/// Sets kinematic flag
				/// </summary>
				/// <param name="kinematic"> If true, the object will be made kinematic and vice versa </param>
				virtual void SetKinematic(bool kinematic) override;

				/// <summary>
				/// True, if CCD(PhysX term; means 'continuous collision detection') is enabled; Depending on the backend, 
				/// this may or may not apply and refer to different things, but, in general, enabling will serve the purpose 
				/// of reducing the probability of a dynamic body "phasing through" colliders due to high velocity and what not.
				/// </summary>
				virtual bool CCDEnabled()const override;

				/// <summary>
				/// Enables/Disables continuous collision detection (refer to CCDEnabled() for more details)
				/// </summary>
				/// <param name="enable"> If true, CCD will be enabled </param>
				virtual void EnableCCD(bool enable) override;

				/// <summary> Movement speed vector </summary>
				virtual Vector3 Velocity()const override;

				/// <summary>
				/// Sets movement speed
				/// </summary>
				/// <param name="velocity"> New speed </param>
				virtual void SetVelocity(const Vector3& velocity) override;

				/// <summary>
				/// Applies force on the body
				/// </summary>
				/// <param name="force"> Force to add </param>
				virtual void AddForce(const Vector3& force) override;

				/// <summary>
				/// Applies force that will increase velocity by given amount
				/// </summary>
				/// <param name="deltaVelocity"> velocity to add </param>
				virtual void AddVelocity(const Vector3& deltaVelocity) override;

				/// <summary> Rotation speed </summary>
				virtual Vector3 AngularVelocity()const override;

				/// <summary>
				/// Sets rotation speed
				/// </summary>
				/// <param name="velocity"> New rotation speed </param>
				virtual void SetAngularVelocity(const Vector3& velocity) override;

				/// <summary>
				/// Applies torque to the body
				/// </summary>
				/// <param name="torque"> Angular velocity change rate </param>
				virtual void AddTorque(const Vector3& torque) override;

				/// <summary>
				/// Adds angular velocity to the body
				/// </summary>
				/// <param name="deltaAngularVelocity"> Angular velocity change </param>
				virtual void AddAngularVelocity(const Vector3& deltaAngularVelocity) override;

				/// <summary>
				/// Moves kinematic body to given destination
				/// </summary>
				/// <param name="transform"> Destination pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				virtual void MoveKinematic(const Matrix4& transform) override;

				/// <summary> Retrieves currently applied lock flags </summary>
				virtual LockFlagMask GetLockFlags()const override;

				/// <summary>
				/// Applies constraints, based on given bitmask
				/// </summary>
				/// <param name="mask"> Constraint bitmask </param>
				virtual void SetLockFlags(LockFlagMask mask) override;

				/// <summary>
				/// Repositions the body
				/// </summary>
				/// <param name="transform"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				virtual void SetPose(const Matrix4& transform) override;

				/// <summary> Underlying API object </summary>
				operator physx::PxRigidDynamic* ()const;

				/// <summary> Underlying API object </summary>
				physx::PxRigidDynamic* operator->()const;

			private:
				// CCD enabled/disabled
				std::atomic<bool> m_ccdEnabled = false;
			};
#pragma warning(default: 4250)
		}
	}
}
