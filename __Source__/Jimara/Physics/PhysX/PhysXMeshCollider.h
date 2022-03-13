#pragma once
#include "PhysXCollider.h"
#include "PhysXCollisionMesh.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
#pragma warning(disable: 4250)
			/// <summary>
			/// PhysX-backed mesh collider
			/// </summary>
			class PhysXMeshCollider : public virtual SingleMaterialPhysXCollider, public virtual PhysicsMeshCollider {
			public:
				/// <summary> Virtual destructor </summary>
				virtual ~PhysXMeshCollider();

				/// <summary>
				/// Creates a collider
				/// </summary>
				/// <param name="body"> Body to attach shape to </param>
				/// <param name="geometry"> Geometry descriptor </param>
				/// <param name="material"> Material to use </param>
				/// <param name="listener"> Event listener </param>
				/// <param name="active"> If true, the collider will start off enabled </param>
				/// <returns> New instance of a collider </returns>
				static Reference<PhysXMeshCollider> Create(PhysXBody* body, const MeshShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active);

				/// <summary>
				/// Alters collider shape
				/// </summary>
				/// <param name="newShape"> Updated shape </param>
				virtual void Update(const MeshShape& newShape) override;

			private:
				// Lock for internal state protection
				std::mutex m_lock;

				// Dynamic geometry handler, associated with the currently set mesh
				Reference<PhysXCollisionMesh> m_shapeObject;

				// Underlying physX mesh
				PhysXReference<physx::PxTriangleMesh> m_triangleMesh;
				
				// Current scale
				Vector3 m_scale;

				// Constructor
				PhysXMeshCollider(
					PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, 
					PhysXCollisionMesh* shapeObject, const MeshShape& mesh, physx::PxTriangleMesh* physMesh);

				// Changes m_shapeObject
				void SetShapeObject(PhysXCollisionMesh* shapeObject);

				// Whenever m_shapeObject 'understands' that m_triangleMesh is dirty, it sends this notification
				void ShapeDirty(PhysXCollisionMesh* shapeObject);
			};
#pragma warning(default: 4250)
		}
	}
}