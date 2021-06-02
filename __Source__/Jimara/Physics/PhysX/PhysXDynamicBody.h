#pragma once
#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
#pragma warning(disable: 4250)
			/// <summary> A simple wrapper on top of physx::PxRigidDynamic </summary>
			class PhysXDynamicBody : public virtual DynamicBody, public virtual PhysXBody {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, this body belongs to </param>
				/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="enabled"> If true, the body will start-off enabled </param>
				PhysXDynamicBody(PhysXScene* scene, const Matrix4& transform, bool enabled);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXDynamicBody();

				/// <summary> Underlying API object </summary>
				operator physx::PxRigidDynamic* ()const;

				/// <summary> Underlying API object </summary>
				physx::PxRigidDynamic* operator->()const;
			};
#pragma warning(default: 4250)
		}
	}
}
