#include "PhysXRigidBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXRigidBody::PhysXRigidBody(PhysXScene* scene, const Matrix4& transform)
				: PhysXBody(scene, (*reinterpret_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform)))) {}

			PhysXRigidBody::~PhysXRigidBody() {}

			PhysXRigidBody::operator physx::PxRigidDynamic* ()const {
				return reinterpret_cast<physx::PxRigidDynamic*>((physx::PxActor*)(*this));
			}
		}
	}
}
