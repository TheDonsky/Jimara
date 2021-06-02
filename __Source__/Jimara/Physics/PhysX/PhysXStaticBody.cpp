#include "PhysXStaticBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXStaticBody::PhysXStaticBody(PhysXScene* scene, const Matrix4& pose, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidStatic(physx::PxTransform(Translate(pose))), enabled) {}

			PhysXStaticBody::~PhysXStaticBody() {}

			PhysXStaticBody::operator physx::PxRigidStatic* ()const {
				return dynamic_cast<physx::PxRigidStatic*>((physx::PxActor*)(*this));
			}

			physx::PxRigidStatic* PhysXStaticBody::operator->()const {
				return dynamic_cast<physx::PxRigidStatic*>((physx::PxActor*)(*this));
			}
		}
	}
}
