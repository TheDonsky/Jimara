#include "MeshCollider.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	MeshCollider::MeshCollider(Component* parent, const std::string_view& name, TriMesh* mesh, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material) {
		SetMesh(mesh);
	}

	TriMesh* MeshCollider::Mesh()const { return (m_mesh == nullptr) ? (TriMesh*)nullptr : m_mesh->Mesh(); }

	void MeshCollider::SetMesh(TriMesh* mesh) {
		Reference<Asset::Of<Physics::CollisionMesh>> collisionMeshAsset = 
			mesh == nullptr ? nullptr : Physics::CollisionMesh::GetAsset(mesh, Context()->Physics()->APIInstance());
		Reference<Physics::CollisionMesh> collisionMesh = (collisionMeshAsset == nullptr) ? nullptr : collisionMeshAsset->Load();
		SetCollisionMesh(collisionMesh);
	}

	Physics::CollisionMesh* MeshCollider::CollisionMesh()const { return m_mesh; }

	void MeshCollider::SetCollisionMesh(Physics::CollisionMesh* mesh) {
		if (m_mesh == mesh) return;
		m_mesh = mesh;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* MeshCollider::Material()const { return m_material; }

	void MeshCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> MeshCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::MeshShape shape(m_mesh, scale);
		if (shape.mesh == nullptr) return nullptr;
		Physics::PhysicsMeshCollider* mesh = dynamic_cast<Physics::PhysicsMeshCollider*>(old);
		if (mesh != nullptr) {
			mesh->Update(shape);
			mesh->SetMaterial(m_material);
			return mesh;
		}
		else {
			Physics::StaticBody* staticBody = dynamic_cast<Physics::StaticBody*>(body);
			if (staticBody == nullptr) {
				Context()->Log()->Warning("MeshCollider::GetPhysicsCollider - Mesh colliders can only be attached to static bodies!");
				return nullptr;
			}
			else return staticBody->AddCollider(shape, m_material, listener, true);
		}
	}

	namespace {
		class MeshColliderSerializer : public ComponentSerializer::Of<MeshCollider> {
		public:
			inline MeshColliderSerializer()
				: ItemSerializer("Jimara/Physics/MeshCollder", "Mesh Collider component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, MeshCollider* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				JIMARA_SERIALIZE_FIELDS(target, recordElement, {
					JIMARA_SERIALIZE_FIELD_GET_SET(CollisionMesh, SetCollisionMesh, "Mesh", "Collision Mesh");
					});
			}

			inline static const ComponentSerializer* Instance() {
				static const MeshColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<MeshCollider>(const Callback<const Object*>& report) { report(MeshColliderSerializer::Instance()); }
}

