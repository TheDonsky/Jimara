#pragma once
#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
#pragma warning(disable: 4250)
			class PhysXRigidBody : public virtual RigidBody, public virtual PhysXBody {
			public:
				PhysXRigidBody(PhysXScene* scene, const Matrix4& transform, bool enabled);

				virtual ~PhysXRigidBody();

				operator physx::PxRigidDynamic* ()const;

				physx::PxRigidDynamic* operator->()const;

			private:
			};
#pragma warning(default: 4250)
		}
	}
}
