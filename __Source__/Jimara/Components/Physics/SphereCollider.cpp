#include "SphereCollider.h"


namespace Jimara {
	SphereCollider::SphereCollider(Component* parent, const std::string_view& name, float radius, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_radius(radius) {}

	namespace {
		class SphereColliderSerializer : public virtual ComponentSerializer {
		public:
			inline SphereColliderSerializer()
				: ItemSerializer("SphereCollder", "Sphere Collider component"), ComponentSerializer("Jimara/Physics/SphereCollider") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override {
				return Object::Instantiate<SphereCollider>(parent, "Sphere Collider");
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				SphereCollider* target = dynamic_cast<SphereCollider*>((Component*)targetAddr);
				target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

				static const Reference<const Serialization::FloatSerializer> colorSerializer = Serialization::FloatSerializer::Create(
					"Radius", "Sphere radius",
					Function<float, void*>([](void* targetAddr) { return dynamic_cast<SphereCollider*>((Component*)targetAddr)->Radius(); }),
					Callback<const float&, void*>([](const float& value, void* targetAddr) { dynamic_cast<SphereCollider*>((Component*)targetAddr)->SetRadius(value); }));
				recordElement(Serialization::SerializedObject(colorSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const SphereColliderSerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry SPHERE_COLLIDER_SERIALIZER;
	}

	Reference<const ComponentSerializer> SphereCollider::GetSerializer()const {
		return SphereColliderSerializer::Instance();
	}

	template<> void TypeIdDetails::OnRegisterType<SphereCollider>() { SPHERE_COLLIDER_SERIALIZER = SphereColliderSerializer::Instance(); }
	template<> void TypeIdDetails::OnUnregisterType<SphereCollider>() { SPHERE_COLLIDER_SERIALIZER = nullptr; }

	float SphereCollider::Radius()const { return m_radius; }

	void SphereCollider::SetRadius(float value) {
		if (m_radius == value) return;
		m_radius = value;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* SphereCollider::Material()const { return m_material; }

	void SphereCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> SphereCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::SphereShape shape(m_radius * max(scale.x, max(scale.y, scale.z)));
		Physics::PhysicsSphereCollider* sphere = dynamic_cast<Physics::PhysicsSphereCollider*>(old);
		if (sphere != nullptr) {
			sphere->Update(shape);
			sphere->SetMaterial(m_material);
			return sphere;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
