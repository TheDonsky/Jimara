#pragma once
#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
#pragma warning(disable: 4250)
		namespace PhysX {
			class PhysXStaticBody : public virtual StaticBody, public virtual PhysXBody {
			public:
				PhysXStaticBody(PhysXScene* scene, const Matrix4& transform);

				virtual ~PhysXStaticBody();

				operator physx::PxRigidStatic* ()const;


			public:
			};
#pragma warning(default: 4250)
		}
	}
}
