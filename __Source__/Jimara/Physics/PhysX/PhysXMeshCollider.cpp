#include "PhysXMeshCollider.h"
#include "../../Math/Helpers.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				struct InstanceAndMesh {
					Reference<PhysXInstance> instance;
					Reference<const TriMesh> mesh;

					inline InstanceAndMesh(PhysXInstance* i = nullptr, const TriMesh* m = nullptr) : instance(i), mesh(m) {}

					inline bool operator<(const InstanceAndMesh& other)const {
						if (instance < other.instance) return true;
						else if (instance > other.instance) return false;
						else return mesh < other.mesh;
					}

					inline bool operator==(const InstanceAndMesh& other)const { return instance == other.instance && mesh == other.mesh; }
				};
			}
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Physics::PhysX::InstanceAndMesh> {
		inline size_t operator()(const Jimara::Physics::PhysX::InstanceAndMesh& v)const {
			return Jimara::MergeHashes(std::hash<Jimara::Physics::PhysX::PhysXInstance*>()(v.instance), std::hash<const Jimara::TriMesh*>()(v.mesh));
		}
	};
}

namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				class PhysXMeshGeometry : public virtual ObjectCache<InstanceAndMesh>::StoredObject {
				private:
					const Reference<PhysXInstance> m_instance;
					const Reference<const TriMesh> m_mesh;
					PhysXReference<physx::PxTriangleMesh> m_pxMesh;
					EventInstance<Object*> m_onDirty;

					inline void OnMeshDirty(Mesh<MeshVertex, TriangleFace>*) {
						m_pxMesh = nullptr;
						m_onDirty(this);
					}

				public:
					inline PhysXMeshGeometry(PhysXInstance* instance, const TriMesh* mesh) : m_instance(instance), m_mesh(mesh) {
						m_mesh->OnDirty() += Callback(&PhysXMeshGeometry::OnMeshDirty, this);
						OnMeshDirty(nullptr);
					}

					inline virtual ~PhysXMeshGeometry() {
						m_mesh->OnDirty() -= Callback(&PhysXMeshGeometry::OnMeshDirty, this);
					}

					inline Event<Object*>& OnDirty() { return m_onDirty; }

					inline static PhysXReference<physx::PxTriangleMesh> CreatePhysXMesh(PhysXInstance* instance, const TriMesh* mesh) {
						TriMesh::Reader reader(mesh);

						physx::PxTriangleMeshDesc meshDesc;
						{
							static thread_local std::vector<physx::PxVec3> points;
							{
								if (points.size() < reader.VertCount()) points.resize(reader.VertCount());
								for (size_t i = 0; i < reader.VertCount(); i++) points[i] = Translate(reader.Vert(i).position);
							}
							meshDesc.points.count = static_cast<physx::PxU32>(reader.VertCount());
							meshDesc.points.stride = sizeof(physx::PxVec3);
							meshDesc.points.data = points.data();
						}
						{
							static thread_local std::vector<physx::PxU32> triangles;
							{
								size_t triBufferSize = reader.FaceCount() * 3;
								if (triangles.size() < triBufferSize) triangles.resize(triBufferSize);
								size_t i = 0;
								for (size_t a = 0; a < triBufferSize; a += 3) {
									const TriangleFace& face = reader.Face(i);
									triangles[a] = face.a;
									triangles[a + 1] = face.b;
									triangles[a + 2] = face.c;
									i++;
								}
							}
							meshDesc.triangles.count = static_cast<uint32_t>(reader.FaceCount());
							meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
							meshDesc.triangles.data = triangles.data();
						}
#ifdef _DEBUG
						if (!instance->Cooking()->validateTriangleMesh(meshDesc)) {
							instance->Log()->Error("PhysXMeshCollider - Invalid mesh!");
							return nullptr;
						}
#endif

						PhysXReference<physx::PxTriangleMesh> physXMesh = instance->Cooking()->createTriangleMesh(meshDesc, (*instance)->getPhysicsInsertionCallback());
						if (physXMesh == nullptr) instance->Log()->Error("PhysXMeshCollider - Failed to create physx::PxTriangleMesh!");
						else physXMesh->release();
						return physXMesh;
					}

					inline PhysXReference<physx::PxTriangleMesh> PhysXMesh() {
						PhysXReference<physx::PxTriangleMesh> mesh = m_pxMesh;
						if (mesh == nullptr) {
							mesh = CreatePhysXMesh(m_instance, m_mesh);
							m_pxMesh = mesh;
						}
						return mesh;
					}

					inline const TriMesh* Mesh()const { return m_mesh; }
				};

				class GeometryCache : public virtual ObjectCache<InstanceAndMesh> {
				public:
					inline static Reference<PhysXMeshGeometry> GetMesh(PhysXInstance* instance, const TriMesh* mesh) {
						static GeometryCache cache;
						if (mesh == nullptr || instance == nullptr) return nullptr;
						return cache.GetCachedOrCreate(InstanceAndMesh(instance, mesh), false
							, [&]()->Reference<PhysXMeshGeometry> { return Object::Instantiate<PhysXMeshGeometry>(instance, mesh); });
					}
				};
			}

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

				Reference<PhysXMeshGeometry> mesh = GeometryCache::GetMesh(instance, geometry.mesh);
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
				return nullptr;
			}

			void PhysXMeshCollider::Update(const MeshShape& newShape) {
				if (newShape.mesh == nullptr) {
					Body()->Scene()->APIInstance()->Log()->Error("PhysXMeshCollider::Update - Mesh can not be nullptr!");
					return;
				}
				std::unique_lock<std::mutex> lock(m_lock);
				TriMesh::Reader reader(newShape.mesh);

				PhysXMeshGeometry* geometry = dynamic_cast<PhysXMeshGeometry*>(m_shapeObject.operator->());

				if (geometry->Mesh() != newShape.mesh) {
					Reference<PhysXMeshGeometry> mesh = GeometryCache::GetMesh(dynamic_cast<PhysXInstance*>(Body()->Scene()->APIInstance()), newShape.mesh);
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
					m_scale = newShape.scale;
				}
				else if (m_scale == newShape.scale) return;
				Shape()->setGeometry(physx::PxTriangleMeshGeometry(m_triangleMesh, physx::PxMeshScale(Translate(m_scale))));
			}

			PhysXMeshCollider::PhysXMeshCollider(
				PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active,
				Object* shapeObject, const MeshShape& mesh, physx::PxTriangleMesh* physMesh)
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), SingleMaterialPhysXCollider(body, shape, material, listener, active)
				, m_triangleMesh(physMesh), m_scale(mesh.scale) {
				SetShapeObject(shapeObject);
			}

			PhysXMeshCollider::~PhysXMeshCollider() {
				std::unique_lock<std::mutex> lock(m_lock);
				SetShapeObject(nullptr);
			}

			void PhysXMeshCollider::SetShapeObject(Object* shapeObject) {
				PhysXMeshGeometry* oldGeometry = dynamic_cast<PhysXMeshGeometry*>(m_shapeObject.operator->());
				PhysXMeshGeometry* newGeometry = dynamic_cast<PhysXMeshGeometry*>(shapeObject);
				if (oldGeometry == newGeometry) return;
				Callback onDirty(&PhysXMeshCollider::ShapeDirty, this);
				if (oldGeometry != nullptr) oldGeometry->OnDirty() -= onDirty;
				m_shapeObject = shapeObject;
				if (newGeometry != nullptr) newGeometry->OnDirty() += onDirty;
			}

			void PhysXMeshCollider::ShapeDirty(Object* shapeObject) {
				std::unique_lock<std::mutex> lock(m_lock);
				PhysXMeshGeometry* geometry = dynamic_cast<PhysXMeshGeometry*>(shapeObject);
				PhysXReference<physx::PxTriangleMesh> physXMesh = geometry->PhysXMesh();
				if (physXMesh == nullptr) {
					Body()->Scene()->APIInstance()->Log()->Fatal("PhysXMeshCollider::ShapeDirty - Failed get physX mesh!");
					return;
				}
				m_triangleMesh = physXMesh;
				Shape()->setGeometry(physx::PxTriangleMeshGeometry(m_triangleMesh, physx::PxMeshScale(Translate(m_scale))));
			}
		}
	}
}
