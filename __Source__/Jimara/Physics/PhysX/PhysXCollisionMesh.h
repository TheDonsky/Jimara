#pragma once
#include "PhysXInstance.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			/// <summary>
			/// CollisionMesh for PhysX backend
			/// </summary>
			class PhysXCollisionMesh : public virtual CollisionMesh {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="apiInstance"> "Owner" PhysicsInstance </param>
				/// <param name="mesh"> Triangle mesh to base CollisionMesh on </param>
				PhysXCollisionMesh(PhysXInstance* apiInstance, TriMesh* mesh);

				/// <summary> Virtual destructor </summary>
				virtual ~PhysXCollisionMesh();

				/// <summary> Invoked each time underlying PhysXMesh is regenerated (passes self as an argument) </summary>
				Event<const PhysXCollisionMesh*>& OnDirty()const;

				/// <summary> Underlying API mesh </summary>
				PhysXReference<physx::PxTriangleMesh> PhysXMesh()const;

			private:
				// "Owner" PhysicsInstance
				const Reference<PhysXInstance> m_apiInstance;

				// Lock for m_pxMesh
				mutable SpinLock m_pxMeshLock;

				// "Underlying" API mesh
				PhysXReference<physx::PxTriangleMesh> m_pxMesh;

				// Invoked each time underlying PhysXMesh is regenerated
				mutable EventInstance<const PhysXCollisionMesh*> m_onDirty;

				// Invoked each time the original triangle mesh gets dirty
				void RecreatePhysXMesh(const TriMesh*);
			};
		}
	}
}
