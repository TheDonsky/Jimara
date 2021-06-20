#include "PhysXStaticBody.h"
#include "PhysXMeshCollider.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXStaticBody::PhysXStaticBody(PhysXScene* scene, const Matrix4& pose, bool enabled)
				: PhysXBody(scene, (*dynamic_cast<PhysXInstance*>(scene->APIInstance()))->createRigidStatic(physx::PxTransform(Translate(pose)))->is<physx::PxRigidActor>(), enabled) {}

			PhysXStaticBody::~PhysXStaticBody() {}

			Reference<PhysicsMeshCollider> PhysXStaticBody::AddCollider(const MeshShape& mesh, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXMeshCollider::Create(this, mesh, material, listener, enabled);
			}

			PhysXStaticBody::operator physx::PxRigidStatic* ()const {
				return (physx::PxRigidStatic*)(operator physx::PxRigidActor * ());
			}

			physx::PxRigidStatic* PhysXStaticBody::operator->()const {
				return (physx::PxRigidStatic*)(operator physx::PxRigidActor * ());
			}
		}
	}
}
