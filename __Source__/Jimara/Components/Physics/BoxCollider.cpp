#include "BoxCollider.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	BoxCollider::BoxCollider(Component* parent, const std::string_view& name, const Vector3& size, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_size(size) {}

	template<> void TypeIdDetails::GetTypeAttributesOf<BoxCollider>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<BoxCollider>(
			"Box Collider", "Jimara/Physics/BoxCollider", "Box-shaped collider");
		report(factory);
	}

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

	void BoxCollider::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Collider::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Size, SetSize, "Size", "Collider size");
			JIMARA_SERIALIZE_FIELD_GET_SET(Material, SetMaterial, "Material", "Physics material");
		};
	}

	void BoxCollider::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		SingleMaterialCollider::GetSerializedActions(report);

		// Size:
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create("Size", "Collider size");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From("SetSize", Callback<const Vector3&>(&BoxCollider::SetSize, this), serializer));
		}
	}

	AABB BoxCollider::GetBoundaries()const {
		const Transform* transform = GetTransform();
		if (transform == nullptr)
			return AABB(
				Vector3(std::numeric_limits<float>::quiet_NaN()),
				Vector3(std::numeric_limits<float>::quiet_NaN()));
		return transform->WorldMatrix() * AABB(-m_size * 0.5f, m_size * 0.5f);
	}

	Reference<Physics::PhysicsCollider> BoxCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		static const constexpr float minSize = std::numeric_limits<float>::epsilon() * 16.0f;
		const Physics::BoxShape shape(Vector3(
			Math::Max(std::abs(m_size.x * scale.x), minSize),
			Math::Max(std::abs(m_size.y * scale.y), minSize),
			Math::Max(std::abs(m_size.z * scale.z), minSize)));
		Physics::PhysicsBoxCollider* box = dynamic_cast<Physics::PhysicsBoxCollider*>(old);
		if (box != nullptr) {
			box->Update(shape);
			box->SetMaterial(m_material);
			return box;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
