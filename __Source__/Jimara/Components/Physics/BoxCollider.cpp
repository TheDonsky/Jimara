#include "BoxCollider.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	BoxCollider::BoxCollider(Component* parent, const std::string_view& name, const Vector3& size, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_size(size) {}

	namespace {
		class BoxColliderSerializer : public virtual ComponentSerializer::Of<BoxCollider> {
		public:
			inline BoxColliderSerializer()
				: ItemSerializer("Jimara/Physics/BoxCollider", "Box Collider component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, BoxCollider* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				JIMARA_SERIALIZE_FIELDS(target, recordElement, {
					JIMARA_SERIALIZE_FIELD_GET_SET(Size, SetSize, "Size", "Collider size");
					});
			}

			inline static const ComponentSerializer* Instance() {
				static const BoxColliderSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<BoxCollider>(const Callback<const Object*>& report) { report(BoxColliderSerializer::Instance()); }

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
		const Physics::BoxShape shape(Vector3(std::abs(m_size.x * scale.x), std::abs(m_size.y * scale.y), std::abs(m_size.z * scale.z)));
		Physics::PhysicsBoxCollider* box = dynamic_cast<Physics::PhysicsBoxCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			box->SetMaterial(m_material);
			return box;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
