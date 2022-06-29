#include "SphereCollider.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	SphereCollider::SphereCollider(Component* parent, const std::string_view& name, float radius, Physics::PhysicsMaterial* material)
		: Component(parent, name), m_material(material), m_radius(radius) {}

	template<> void TypeIdDetails::GetTypeAttributesOf<SphereCollider>(const Callback<const Object*>& report) { 
		static const ComponentSerializer::Of<SphereCollider> serializer("Jimara/Physics/SphereCollder", "Sphere Collider component");
		report(&serializer);
	}

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

	void SphereCollider::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Radius, SetRadius, "Radius", "Sphere radius");
			JIMARA_SERIALIZE_FIELD_GET_SET(Material, SetMaterial, "Material", "Physics material");
		};
	}

	Reference<Physics::PhysicsCollider> SphereCollider::GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) {
		const Physics::SphereShape shape(std::abs(m_radius) * max(std::abs(scale.x), max(std::abs(scale.y), std::abs(scale.z))));
		Physics::PhysicsSphereCollider* sphere = dynamic_cast<Physics::PhysicsSphereCollider*>(old);
		if (sphere != nullptr) {
			sphere->Update(shape);
			sphere->SetMaterial(m_material);
			return sphere;
		}
		else return body->AddCollider(shape, m_material, listener, true);
	}
}
