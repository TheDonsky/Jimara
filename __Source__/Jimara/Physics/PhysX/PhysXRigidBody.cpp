#include "PhysXRigidBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXRigidBody::PhysXRigidBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform))), enabled) {}

			PhysXRigidBody::~PhysXRigidBody() {}

			PhysXRigidBody::operator physx::PxRigidDynamic* ()const {
				return dynamic_cast<physx::PxRigidDynamic*>((physx::PxActor*)(*this));
			}

			physx::PxRigidDynamic* PhysXRigidBody::operator->()const {
				return dynamic_cast<physx::PxRigidDynamic*>((physx::PxActor*)(*this));
			}
		}
	}
}
