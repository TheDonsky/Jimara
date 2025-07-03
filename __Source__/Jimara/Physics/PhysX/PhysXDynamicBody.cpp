#include "PhysXDynamicBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXDynamicBody::PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform)))->is<physx::PxRigidActor>(), enabled) {}

			PhysXDynamicBody::~PhysXDynamicBody() {}

			float PhysXDynamicBody::Mass()const { PhysXScene::ReadLock lock(Scene()); return operator->()->getMass(); }

			void PhysXDynamicBody::SetMass(float mass) { PhysXScene::WriteLock lock(Scene()); operator->()->setMass(max(mass, 0.0f)); }

			bool PhysXDynamicBody::IsKinematic()const { PhysXScene::ReadLock lock(Scene()); return ((uint32_t)operator->()->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) != 0u; }

			void PhysXDynamicBody::SetKinematic(bool kinematic) { 
				PhysXScene::WriteLock lock(Scene());
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, (!kinematic) && m_ccdEnabled.load());
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, kinematic);
			}

			bool PhysXDynamicBody::CCDEnabled()const { return m_ccdEnabled; }

			void PhysXDynamicBody::EnableCCD(bool enable) {
				PhysXScene::WriteLock lock(Scene());
				m_ccdEnabled = enable;
				const bool isKinematic = ((uint32_t)operator->()->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) != 0u;
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, (!isKinematic) && m_ccdEnabled.load());
			}

			Vector3 PhysXDynamicBody::Velocity()const { 
				PhysXScene::ReadLock lock(Scene()); 
				return Translate(operator->()->getLinearVelocity()); 
			}

			void PhysXDynamicBody::SetVelocity(const Vector3& velocity) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->setLinearVelocity(Translate(velocity)); 
			}

			void PhysXDynamicBody::AddForce(const Vector3& force) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->addForce(Translate(force)); 
			}

			void PhysXDynamicBody::AddVelocity(const Vector3& deltaVelocity) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->addForce(Translate(deltaVelocity), physx::PxForceMode::eVELOCITY_CHANGE); 
			}

			Vector3 PhysXDynamicBody::AngularVelocity()const { 
				PhysXScene::ReadLock lock(Scene());
				const physx::PxVec3 velocity = operator->()->getAngularVelocity();
				return Vector3(Math::Degrees(velocity.x), Math::Degrees(velocity.y), Math::Degrees(velocity.z));
			}

			void PhysXDynamicBody::SetAngularVelocity(const Vector3& velocity) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->setAngularVelocity(physx::PxVec3(Math::Radians(velocity.x), Math::Radians(velocity.y), Math::Radians(velocity.z)));
			}

			void PhysXDynamicBody::AddTorque(const Vector3& torque) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->addTorque(physx::PxVec3(Math::Radians(torque.x), Math::Radians(torque.y), Math::Radians(torque.z)));
			}

			void PhysXDynamicBody::AddAngularVelocity(const Vector3& deltaAngularVelocity) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->addTorque(
					physx::PxVec3(Math::Radians(deltaAngularVelocity.x), Math::Radians(deltaAngularVelocity.y), Math::Radians(deltaAngularVelocity.z)),
					physx::PxForceMode::eVELOCITY_CHANGE); 
			}

			void PhysXDynamicBody::MoveKinematic(const Matrix4& transform) { 
				PhysXScene::WriteLock lock(Scene()); 
				operator->()->setKinematicTarget(physx::PxTransform(Translate(transform))); 
			}

			// Let's make sure the enumerations match (will make logic somewhat simpler)
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::MOVEMENT_X);
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::MOVEMENT_Y);
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::MOVEMENT_Z);
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::ROTATION_X);
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::ROTATION_Y);
			static_assert(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z == (physx::PxRigidDynamicLockFlag::Enum)DynamicBody::LockFlag::ROTATION_Z);

			DynamicBody::LockFlagMask PhysXDynamicBody::GetLockFlags()const {
				PhysXScene::ReadLock lock(Scene());
				physx::PxRigidDynamicLockFlags flags = operator->()->getRigidDynamicLockFlags();
				return (LockFlagMask)flags;
			}

			void PhysXDynamicBody::SetLockFlags(LockFlagMask mask) {
				PhysXScene::WriteLock lock(Scene());
				operator->()->setRigidDynamicLockFlags((physx::PxRigidDynamicLockFlags)mask);
			}

			void PhysXDynamicBody::SetPose(const Matrix4& transform) {
				physx::PxRigidDynamic* dynamic = (*this);
				const physx::PxTransform pxTransform(Translate(transform));
				PhysXScene::WriteLock lock(Scene());
				if (((uint32_t)dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) == 0) {
					dynamic->setGlobalPose(pxTransform, true);
				}
				else dynamic->setKinematicTarget(pxTransform);
			}


			PhysXDynamicBody::operator physx::PxRigidDynamic* ()const {
				return (operator physx::PxRigidActor * ())->is<physx::PxRigidDynamic>();
			}

			physx::PxRigidDynamic* PhysXDynamicBody::operator->()const {
				return (operator physx::PxRigidActor * ())->is<physx::PxRigidDynamic>();
			}
		}
	}
}
