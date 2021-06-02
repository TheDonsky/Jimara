#include "PhysXDynamicBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXDynamicBody::PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidDynamic(physx::PxTransform(Translate(transform))), enabled) {}

			PhysXDynamicBody::~PhysXDynamicBody() {}

			PhysXDynamicBody::operator physx::PxRigidDynamic* ()const {
				return dynamic_cast<physx::PxRigidDynamic*>((physx::PxActor*)(*this));
			}

			physx::PxRigidDynamic* PhysXDynamicBody::operator->()const {
				return dynamic_cast<physx::PxRigidDynamic*>((physx::PxActor*)(*this));
			}
		}
	}
}
