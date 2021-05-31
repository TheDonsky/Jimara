#include "PhysXStaticBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXStaticBody::PhysXStaticBody(PhysXScene* scene, const Matrix4& transform, bool enabled)
				: PhysXBody(scene, (*reinterpret_cast<PhysXInstance*>(scene->APIInstance()))->createRigidStatic(physx::PxTransform(Translate(transform))), enabled) {}

			PhysXStaticBody::~PhysXStaticBody() {}

			PhysXStaticBody::operator physx::PxRigidStatic* ()const {
				return reinterpret_cast<physx::PxRigidStatic*>((physx::PxActor*)(*this));
			}

			physx::PxRigidStatic* PhysXStaticBody::operator->()const {
				return reinterpret_cast<physx::PxRigidStatic*>((physx::PxActor*)(*this));
			}
		}
	}
}
