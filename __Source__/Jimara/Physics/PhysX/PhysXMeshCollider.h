#pragma once
#include "PhysXCollider.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
#pragma warning(disable: 4250)
			class PhysXMeshCollider : public virtual SingleMaterialPhysXCollider, public virtual PhysicsMeshCollider {
			public:
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
				std::mutex m_lock;
				Reference<Object> m_shapeObject;
				PhysXReference<physx::PxTriangleMesh> m_triangleMesh;
				Vector3 m_scale;

				PhysXMeshCollider(
					PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, 
					Object* shapeObject, const MeshShape& mesh, physx::PxTriangleMesh* physMesh);

				void SetShapeObject(Object* shapeObject);

				void ShapeDirty(Object* shapeObject);
			};
#pragma warning(default: 4250)
		}
	}
}