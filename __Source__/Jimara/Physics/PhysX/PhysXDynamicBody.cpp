#include "PhysXDynamicBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXDynamicBody::PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform))), enabled) {}

			PhysXDynamicBody::~PhysXDynamicBody() {}

			float PhysXDynamicBody::Mass()const { return operator->()->getMass(); }

			void PhysXDynamicBody::SetMass(float mass) { operator->()->setMass(max(mass, 0.0f)); }

			bool PhysXDynamicBody::IsKinematic()const { return ((uint32_t)operator->()->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC) != 0; }

			void PhysXDynamicBody::SetKinematic(bool kinematic) { operator->()->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, kinematic); }

			Vector3 PhysXDynamicBody::Velocity()const { return Translate(operator->()->getLinearVelocity()); }

			void PhysXDynamicBody::SetVelocity(const Vector3& velocity) { operator->()->setLinearVelocity(Translate(velocity)); }

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
				operator->()->setRigidDynamicLockFlags(flags);
			}


			PhysXDynamicBody::operator physx::PxRigidDynamic* ()const {
				return (physx::PxRigidDynamic*)(operator physx::PxRigidActor * ());
			}

			physx::PxRigidDynamic* PhysXDynamicBody::operator->()const {
				return (physx::PxRigidDynamic*)(operator physx::PxRigidActor * ());
			}
		}
	}
}
