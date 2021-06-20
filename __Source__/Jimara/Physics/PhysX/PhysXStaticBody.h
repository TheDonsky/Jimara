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

				/// <summary>
				/// Adds a mesh collider
				/// </summary>
				/// <param name="mesh"> Shape descriptor </param>
				/// <param name="material"> Physics material for the collider </param>
				/// <param name="listener"> Collider event listener </param>
				/// <param name="enabled"> If true, the collider will start-off enabled </param>
				/// <returns> New collider, attached to the body </returns>
				virtual Reference<PhysicsMeshCollider> AddCollider(const MeshShape& mesh, PhysicsMaterial* material, PhysicsCollider::EventListener* listener = nullptr, bool enabled = true) override;

				/// <summary> Underlying API object </summary>
				operator physx::PxRigidStatic* ()const;

				/// <summary> Underlying API object </summary>
				physx::PxRigidStatic* operator->()const;
			};
#pragma warning(default: 4250)
		}
	}
}
