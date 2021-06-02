#pragma once
#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
#pragma warning(disable: 4250)
		namespace PhysX {
			/// <summary> A simple wrapper on top of physx::PxRigidStatic </summary>
			class PhysXStaticBody : public virtual StaticBody, public virtual PhysXBody {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="scene"> Scene, this body belongs to </param>
				/// <param name="pose"> Pose matrix (only rotation and translation are allowed; scale is not supported and will result in failures) </param>
				/// <param name="enabled"> If true, the body will start-off enabled </param>
				PhysXStaticBody(PhysXScene* scene, const Matrix4& pose, bool enabled);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXStaticBody();

				/// <summary> Underlying API object </summary>
				operator physx::PxRigidStatic* ()const;

				/// <summary> Underlying API object </summary>
				physx::PxRigidStatic* operator->()const;
			};
#pragma warning(default: 4250)
		}
	}
}
