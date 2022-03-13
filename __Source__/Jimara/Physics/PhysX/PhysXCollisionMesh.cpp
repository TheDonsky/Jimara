#include "PhysXCollisionMesh.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXCollisionMesh::PhysXCollisionMesh(PhysXInstance* apiInstance, TriMesh* mesh) 
				: CollisionMesh(mesh), m_apiInstance(apiInstance) {
				Mesh()->OnDirty() += Callback(&PhysXCollisionMesh::RecreatePhysXMesh, this);
				RecreatePhysXMesh(nullptr);
			}

			PhysXCollisionMesh::~PhysXCollisionMesh() {
				Mesh()->OnDirty() -= Callback(&PhysXCollisionMesh::RecreatePhysXMesh, this);
			}

			Event<const PhysXCollisionMesh*>& PhysXCollisionMesh::OnDirty()const { return m_onDirty; }

			PhysXReference<physx::PxTriangleMesh> PhysXCollisionMesh::PhysXMesh()const {
				std::unique_lock<SpinLock> lock(m_pxMeshLock);
				PhysXReference<physx::PxTriangleMesh> rv = m_pxMesh;
				return rv;
			}

			namespace {
				inline static PhysXReference<physx::PxTriangleMesh> CreatePhysXMesh(PhysXInstance* instance, const TriMesh* mesh) {
					TriMesh::Reader reader(mesh);

					physx::PxTriangleMeshDesc meshDesc;
					{
						static thread_local std::vector<physx::PxVec3> points;
						{
							if (points.size() < reader.VertCount()) points.resize(reader.VertCount());
							for (uint32_t i = 0; i < reader.VertCount(); i++) points[i] = Translate(reader.Vert(i).position);
						}
						meshDesc.points.count = static_cast<physx::PxU32>(reader.VertCount());
						meshDesc.points.stride = sizeof(physx::PxVec3);
						meshDesc.points.data = points.data();
					}
					{
						static thread_local std::vector<physx::PxU32> triangles;
						{
							uint32_t triBufferSize = reader.FaceCount() * 3;
							if (triangles.size() < triBufferSize) triangles.resize(triBufferSize);
							uint32_t i = 0;
							for (size_t a = 0; a < triBufferSize; a += 3) {
								const TriangleFace& face = reader.Face(i);
								triangles[a] = face.a;
								triangles[a + 1] = face.c;
								triangles[a + 2] = face.b;
								i++;
							}
						}
						meshDesc.triangles.count = static_cast<uint32_t>(reader.FaceCount());
						meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
						meshDesc.triangles.data = triangles.data();
					}

					PhysXReference<physx::PxTriangleMesh> physXMesh = instance->Cooking()->createTriangleMesh(meshDesc, (*instance)->getPhysicsInsertionCallback());
					if (physXMesh == nullptr) instance->Log()->Error("PhysXCollisionMesh::CreatePhysXMesh - Failed to create physx::PxTriangleMesh!");
					else physXMesh->release();
					return physXMesh;
				}
			}

			void PhysXCollisionMesh::RecreatePhysXMesh(const TriMesh*) {
				const PhysXReference<physx::PxTriangleMesh> physXMesh = CreatePhysXMesh(m_apiInstance, Mesh());
				{
					std::unique_lock<SpinLock> lock(m_pxMeshLock);
					m_pxMesh = physXMesh;
				}
				m_onDirty(this);
			}
		}
	}
}
