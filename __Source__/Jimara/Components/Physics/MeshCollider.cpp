#include "MeshCollider.h"


namespace Jimara {
	MeshCollider::MeshCollider(Component* parent, const std::string_view& name, TriMesh* mesh, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material) {
		SetMesh(mesh);
	}

	TriMesh* MeshCollider::Mesh()const { return (m_mesh == nullptr) ? (TriMesh*)nullptr : m_mesh->Mesh(); }

	void MeshCollider::SetMesh(TriMesh* mesh) {
		Reference<Physics::CollisionMeshAsset> collisionMeshAsset = 
			mesh == nullptr ? nullptr : Physics::CollisionMeshAsset::GetFor(mesh, Context()->Physics()->APIInstance());
		Reference<Physics::CollisionMesh> collisionMesh = (collisionMeshAsset == nullptr) ? nullptr : collisionMeshAsset->Load();
		if (m_mesh == collisionMesh) return;
		m_mesh = collisionMesh;
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
				{
					static const Reference<const Serialization::ItemSerializer::Of<MeshCollider>> serializer =
						Serialization::ValueSerializer<TriMesh*>::Create<MeshCollider>(
							"Mesh", "Mesh to render",
							Function<TriMesh*, MeshCollider*>([](MeshCollider* collider) -> TriMesh* { return collider->Mesh(); }),
							Callback<TriMesh* const&, MeshCollider*>([](TriMesh* const& value, MeshCollider* collider) { collider->SetMesh(value); }));
					recordElement(serializer->Serialize(target));
				}
			}

			inline static const ComponentSerializer* Instance() {
				static const MeshColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<MeshCollider>(const Callback<const Object*>& report) { report(MeshColliderSerializer::Instance()); }
}

