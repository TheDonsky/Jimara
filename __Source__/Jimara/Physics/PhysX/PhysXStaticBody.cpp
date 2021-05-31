#include "PhysXStaticBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXStaticBody::PhysXStaticBody(PhysXScene* scene, const Matrix4& transform)
				: PhysXBody(scene, (*reinterpret_cast<PhysXInstance*>(scene->APIInstance()))->createRigidStatic(physx::PxTransform(Translate(transform)))) {}

			PhysXStaticBody::~PhysXStaticBody() {}

			PhysXStaticBody::operator physx::PxRigidStatic* ()const {
				return reinterpret_cast<physx::PxRigidStatic*>((physx::PxActor*)(*this));
			}
		}
	}
}
