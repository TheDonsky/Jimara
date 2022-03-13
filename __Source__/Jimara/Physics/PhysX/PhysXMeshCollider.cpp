#include "PhysXMeshCollider.h"
#include "../../Math/Helpers.h"
#include "../../Core/Collections/ObjectCache.h"

namespace Jimara {
	namespace Physics {
		namespace PhysX {
			Reference<PhysXMeshCollider> PhysXMeshCollider::Create(PhysXBody* body, const MeshShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active) {
				PhysXInstance* instance = dynamic_cast<PhysXInstance*>(body->Scene()->APIInstance());
				
				if (geometry.mesh == nullptr) {
					instance->Log()->Error("PhysXMeshCollider::Create - Mesh can not be nullptr!");
					return nullptr;
				}

				Reference<PhysXMaterial> apiMaterial = dynamic_cast<PhysXMaterial*>(material);
				if (apiMaterial == nullptr) apiMaterial = PhysXMaterial::Default(instance);
				if (apiMaterial == nullptr) {
					instance->Log()->Fatal("PhysXMeshCollider::Create - Failed to resolve the material!");
					return nullptr;
				}

				// To make sure we don't have accidental fuck ups...
				TriMesh::Reader reader(geometry.mesh);

				const Reference<CollisionMeshAsset> asset = CollisionMeshAsset::GetFor(geometry.mesh, instance);
				Reference<PhysXCollisionMesh> mesh = asset->Load();
				if (mesh == nullptr) {
					instance->Log()->Error("PhysXMeshCollider::Create - Failed get physics mesh!");
					return nullptr;
				}

				PhysXReference<physx::PxTriangleMesh> physXMesh = mesh->PhysXMesh();
				if (physXMesh == nullptr) {
					instance->Log()->Fatal("PhysXMeshCollider::Create - Failed get physX mesh!");
					return nullptr;
				}

				physx::PxShape* shape = (*instance)->createShape(physx::PxTriangleMeshGeometry(physXMesh, physx::PxMeshScale(Translate(geometry.scale))), *(*apiMaterial), true);
				if (shape == nullptr) {
					instance->Log()->Error("PhysXMeshCollider::Create - Failed to create shape!");
					return nullptr;
				}
				Reference<PhysXMeshCollider> collider = new PhysXMeshCollider(body, shape, apiMaterial, listener, active, mesh, geometry, physXMesh);
				collider->ReleaseRef();
				return collider;
			}

			void PhysXMeshCollider::Update(const MeshShape& newShape) {
				if (newShape.mesh == nullptr) {
					Body()->Scene()->APIInstance()->Log()->Error("PhysXMeshCollider::Update - Mesh can not be nullptr!");
					return;
				}
				std::unique_lock<std::mutex> lock(m_lock);
				TriMesh::Reader reader(newShape.mesh);

				if (m_shapeObject->Mesh() != newShape.mesh) {
					const Reference<CollisionMeshAsset> asset = CollisionMeshAsset::GetFor(newShape.mesh, Body()->Scene()->APIInstance());
					Reference<PhysXCollisionMesh> mesh = asset->Load();
					if (mesh == nullptr) {
						Body()->Scene()->APIInstance()->Log()->Fatal("PhysXMeshCollider::Update - Failed get physics mesh!");
						return;
					}

					PhysXReference<physx::PxTriangleMesh> physXMesh = mesh->PhysXMesh();
					if (physXMesh == nullptr) {
						Body()->Scene()->APIInstance()->Log()->Fatal("PhysXMeshCollider::Update - Failed get physX mesh!");
						return;
					}

					SetShapeObject(mesh);

					m_triangleMesh = physXMesh;
				}
				else if (m_scale == newShape.scale) return;
				m_scale = newShape.scale;
				PhysXScene::WriteLock sceneLock(Body()->Scene());
				Shape()->setGeometry(physx::PxTriangleMeshGeometry(m_triangleMesh, physx::PxMeshScale(Translate(m_scale))));
			}

			PhysXMeshCollider::PhysXMeshCollider(
				PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active,
				PhysXCollisionMesh* shapeObject, const MeshShape& mesh, physx::PxTriangleMesh* physMesh)
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), SingleMaterialPhysXCollider(body, shape, material, listener, active)
				, m_triangleMesh(physMesh), m_scale(mesh.scale) {
				SetShapeObject(shapeObject);
			}

			PhysXMeshCollider::~PhysXMeshCollider() {
				std::unique_lock<std::mutex> lock(m_lock);
				SetShapeObject(nullptr);
			}

			void PhysXMeshCollider::SetShapeObject(PhysXCollisionMesh* shapeObject) {
				if (m_shapeObject == shapeObject) return;
				Callback onDirty(&PhysXMeshCollider::ShapeDirty, this);
				if (m_shapeObject != nullptr) m_shapeObject->OnDirty() -= onDirty;
				m_shapeObject = shapeObject;
				if (m_shapeObject != nullptr) m_shapeObject->OnDirty() += onDirty;
			}

			void PhysXMeshCollider::ShapeDirty(PhysXCollisionMesh* shapeObject) {
				std::unique_lock<std::mutex> lock(m_lock);
				PhysXReference<physx::PxTriangleMesh> physXMesh = shapeObject->PhysXMesh();
				if (physXMesh == nullptr) {
					Body()->Scene()->APIInstance()->Log()->Fatal("PhysXMeshCollider::ShapeDirty - Failed get physX mesh!");
					return;
				}
				m_triangleMesh = physXMesh;
				PhysXScene::WriteLock sceneLock(Body()->Scene());
				Shape()->setGeometry(physx::PxTriangleMeshGeometry(m_triangleMesh, physx::PxMeshScale(Translate(m_scale))));
			}
		}
	}
}
