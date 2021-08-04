#include "BoxCollider.h"


namespace Jimara {
	BoxCollider::BoxCollider(Component* parent, const std::string_view& name, const Vector3& size, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_size(size) {}

	namespace {
		class BoxColliderSerializer : public virtual ComponentSerializer {
		public:
			inline BoxColliderSerializer()
				: ItemSerializer("BoxCollider", "Box Collider component"), ComponentSerializer("Jimara/Physics/BoxCollider") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override {
				return Object::Instantiate<BoxCollider>(parent, "Box Collider");
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				BoxCollider* target = dynamic_cast<BoxCollider*>((Component*)targetAddr);
				target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

				static const Reference<const Serialization::Vector3Serializer> colorSerializer = Serialization::Vector3Serializer::Create(
					"Size", "Collider size",
					Function<Vector3, void*>([](void* targetAddr) { return dynamic_cast<BoxCollider*>((Component*)targetAddr)->Size(); }),
					Callback<const Vector3&, void*>([](const Vector3& value, void* targetAddr) { dynamic_cast<BoxCollider*>((Component*)targetAddr)->SetSize(value); }));
				recordElement(Serialization::SerializedObject(colorSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const BoxColliderSerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry BOX_COLLIDER_SERIALIZER;
	}

	Reference<const ComponentSerializer> BoxCollider::GetSerializer()const {
		return BoxColliderSerializer::Instance();
	}

	JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(BoxCollider, { BOX_COLLIDER_SERIALIZER = BoxColliderSerializer::Instance(); }, { BOX_COLLIDER_SERIALIZER = nullptr; });

	Vector3 BoxCollider::Size()const { return m_size; }

	void BoxCollider::SetSize(const Vector3& size) {
		if (m_size == size) return;
		m_size = size;
		ColliderDirty();
	}

	Physics::PhysicsMaterial* BoxCollider::Material()const { return m_material; }

	void BoxCollider::SetMaterial(Physics::PhysicsMaterial* material) {
		if (m_material == material) return;
		m_material = material;
		ColliderDirty();
	}

	Reference<Physics::PhysicsCollider> BoxCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::BoxShape shape(m_size * scale);
		Physics::PhysicsBoxCollider* box = dynamic_cast<Physics::PhysicsBoxCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			box->SetMaterial(m_material);
			return box;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
