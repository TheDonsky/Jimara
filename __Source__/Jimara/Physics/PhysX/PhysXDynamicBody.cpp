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


			PhysXDynamicBody::operator physx::PxRigidDynamic* ()const {
				return (physx::PxRigidDynamic*)(operator physx::PxRigidActor * ());
			}

			physx::PxRigidDynamic* PhysXDynamicBody::operator->()const {
				return (physx::PxRigidDynamic*)(operator physx::PxRigidActor * ());
			}
		}
	}
}
