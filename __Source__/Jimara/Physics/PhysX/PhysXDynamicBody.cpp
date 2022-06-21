#include "PhysXDynamicBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXDynamicBody::PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform)))->is<physx::PxRigidActor>(), enabled) {
				PhysXScene::WriteLock lock(Scene());
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, ((uint32_t)operator->()->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) == 0u);
			}

			PhysXDynamicBody::~PhysXDynamicBody() {}

			float PhysXDynamicBody::Mass()const { PhysXScene::ReadLock lock(Scene()); return operator->()->getMass(); }

			void PhysXDynamicBody::SetMass(float mass) { PhysXScene::WriteLock lock(Scene()); operator->()->setMass(max(mass, 0.0f)); }

			bool PhysXDynamicBody::IsKinematic()const { PhysXScene::ReadLock lock(Scene()); return ((uint32_t)operator->()->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) != 0u; }

			void PhysXDynamicBody::SetKinematic(bool kinematic) { 
				PhysXScene::WriteLock lock(Scene());
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, !kinematic);
				operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, kinematic);
			}

			Vector3 PhysXDynamicBody::Velocity()const { PhysXScene::ReadLock lock(Scene()); return Translate(operator->()->getLinearVelocity()); }

			void PhysXDynamicBody::SetVelocity(const Vector3& velocity) { PhysXScene::WriteLock lock(Scene()); operator->()->setLinearVelocity(Translate(velocity)); }

			Vector3 PhysXDynamicBody::AngularVelocity()const { PhysXScene::ReadLock lock(Scene()); return Translate(operator->()->getAngularVelocity()); }

			void PhysXDynamicBody::SetAngularVelocity(const Vector3& velocity) { PhysXScene::WriteLock lock(Scene()); operator->()->setAngularVelocity(Translate(velocity)); }

			void PhysXDynamicBody::MoveKinematic(const Matrix4& transform) { PhysXScene::WriteLock lock(Scene()); operator->()->setKinematicTarget(physx::PxTransform(Translate(transform))); }

			namespace {
				static const uint8_t LOCK_MASK_PAIR_COUNT = 6;

				static const std::pair<physx::PxRigidDynamicLockFlag::Enum, DynamicBody::LockFlag> LOCK_MASK_PAIRS[LOCK_MASK_PAIR_COUNT] = {
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_X, DynamicBody::LockFlag::MOVEMENT_X),
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, DynamicBody::LockFlag::MOVEMENT_Y),
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, DynamicBody::LockFlag::MOVEMENT_Z),
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, DynamicBody::LockFlag::ROTATION_X),
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, DynamicBody::LockFlag::ROTATION_Y),
					std::make_pair(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, DynamicBody::LockFlag::ROTATION_Z)
				};
			}

			DynamicBody::LockFlagMask PhysXDynamicBody::GetLockFlags()const {
				PhysXScene::ReadLock lock(Scene());
				physx::PxRigidDynamicLockFlags flags = operator->()->getRigidDynamicLockFlags();
				LockFlagMask mask = 0;
				for (size_t i = 0; i < LOCK_MASK_PAIR_COUNT; i++) {
					const std::pair<physx::PxRigidDynamicLockFlag::Enum, DynamicBody::LockFlag>& pair = LOCK_MASK_PAIRS[i];
					if (((physx::PxU8)flags & pair.first) != 0) mask |= LockFlags(pair.second);
				}
				return mask;
			}

			void PhysXDynamicBody::SetLockFlags(LockFlagMask mask) {
				physx::PxRigidDynamicLockFlags flags(0);
				for (size_t i = 0; i < LOCK_MASK_PAIR_COUNT; i++) {
					const std::pair<physx::PxRigidDynamicLockFlag::Enum, DynamicBody::LockFlag>& pair = LOCK_MASK_PAIRS[i];
					if ((LockFlags(pair.second) & mask) != 0) flags |= pair.first;
				}
				PhysXScene::WriteLock lock(Scene());
				operator->()->setRigidDynamicLockFlags(flags);
			}

			void PhysXDynamicBody::SetPose(const Matrix4& transform) {
				PhysXBody::SetPose(transform);
				physx::PxRigidDynamic* dynamic = (*this);
				PhysXScene::WriteLock lock(Scene());
				if (((uint32_t)dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) == 0) dynamic->wakeUp();
				else dynamic->setKinematicTarget(dynamic->getGlobalPose());
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
