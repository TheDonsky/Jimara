#include "Rigidbody.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace {
		inline static Matrix4 GetPose(const Transform* transform) {
			if (transform == nullptr) return Math::Identity();
			Matrix4 pose = transform->WorldRotationMatrix();
			pose[3] = Vector4(transform->WorldPosition(), 1.0f);
			return pose;
		}

		inline static const constexpr uint32_t RIGIDBODY_DIRTY_FLAG_VELOCITY = (1u << 0u);
		inline static const constexpr uint32_t RIGIDBODY_DIRTY_FLAG_ANGULAR_VELOCITY = (1u << 1u);
	}

	Rigidbody::Rigidbody(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	Rigidbody::~Rigidbody() {
		OnComponentDestroyed();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Rigidbody>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Rigidbody>(
			"Rigidbody", "Jimara/Physics/Rigidbody", "Body, effected by physics simulationt");
		report(factory);
	}

#define ACCESS_BODY_PROPERTY(if_not_null, if_null) Physics::DynamicBody* body = GetBody(); if (body != nullptr) if_not_null else if_null

	float Rigidbody::Mass()const { ACCESS_BODY_PROPERTY({ return body->Mass(); }, { return 0.0f; }); }

	void Rigidbody::SetMass(float mass) { ACCESS_BODY_PROPERTY({ body->SetMass(mass); }, {}); }

	bool Rigidbody::IsKinematic()const { return m_kinematic; }

	void Rigidbody::SetKinematic(bool kinematic) { 
		if (m_kinematic != kinematic) { 
			m_kinematic = kinematic;
			ACCESS_BODY_PROPERTY({ body->SetKinematic(kinematic); }, {}); 
		} 
	}

	bool Rigidbody::CCDEnabled()const { return m_ccdEnabled; }

	void Rigidbody::EnableCCD(bool enable) {
		if (m_ccdEnabled != enable) {
			m_ccdEnabled = enable;
			ACCESS_BODY_PROPERTY({ body->EnableCCD(m_ccdEnabled); }, {});
		}
	}

	Vector3 Rigidbody::Velocity()const { return m_velocity; }

	void Rigidbody::SetVelocity(const Vector3& velocity) { 
		if (m_kinematic) return;
		if (m_velocity == velocity) return;
		m_velocity = velocity;
		m_dirtyFlags |= RIGIDBODY_DIRTY_FLAG_VELOCITY;
	}

	void Rigidbody::AddForce(const Vector3& force) {
		if (m_kinematic) return;
		ACCESS_BODY_PROPERTY({ body->AddForce(force); }, {});
	}

	void Rigidbody::AddVelocity(const Vector3& deltaVelocity) {
		if (m_kinematic) return;
		if (Math::Dot(deltaVelocity, deltaVelocity) <= std::numeric_limits<float>::epsilon()) return;
		m_velocity += deltaVelocity;
		m_dirtyFlags |= RIGIDBODY_DIRTY_FLAG_VELOCITY;
	}

	Vector3 Rigidbody::AngularVelocity()const { return m_angularVelocity; }

	void Rigidbody::SetAngularVelocity(const Vector3& velocity) {
		if (m_kinematic) return;
		if (m_angularVelocity == velocity) return;
		m_angularVelocity = velocity;
		m_dirtyFlags |= RIGIDBODY_DIRTY_FLAG_ANGULAR_VELOCITY;
	}

	Physics::DynamicBody::LockFlagMask Rigidbody::GetLockFlags()const { ACCESS_BODY_PROPERTY({ return body->GetLockFlags(); }, { return 0; }); }

	void Rigidbody::SetLockFlags(Physics::DynamicBody::LockFlagMask mask) { ACCESS_BODY_PROPERTY({ body->SetLockFlags(mask); }, {}); }

	void Rigidbody::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Mass, SetMass, "Mass", "Rigidbody mass");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsKinematic, SetKinematic, "Kinematic", "True, if the rigidbody should be kinematic");
			JIMARA_SERIALIZE_FIELD_GET_SET(CCDEnabled, EnableCCD, "Enable CCD", "Enables Continuous collision detection");
			JIMARA_SERIALIZE_FIELD_GET_SET(GetLockFlags, SetLockFlags,
				"Lock", "Lock per axis rotation and or movement simulation", Physics::DynamicBody::LockFlagMaskEnumAttribute());
			JIMARA_SERIALIZE_FIELD_GET_SET(Velocity, SetVelocity, "Velocity", "Current/Initial velocity of the Rigidbody");
			JIMARA_SERIALIZE_FIELD_GET_SET(AngularVelocity, SetAngularVelocity, "Angular Velocity", "Current/Initial angular velocity of the Rigidbody");
		};
	}

	void Rigidbody::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Mass
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Mass", "Rigidbody mass");
			report(Serialization::SerializedCallback::Create<float>::From("SetMass", Callback<float>(&Rigidbody::SetMass, this), serializer));
		}

		// Kinematic flag
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create("Kinematic", "True, if the rigidbody should be made kinematic");
			report(Serialization::SerializedCallback::Create<bool>::From("SetKinematic", Callback<bool>(&Rigidbody::SetKinematic, this), serializer));
		}

		// CCD Enabled flag
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create("Enabled", "Enables/disables Continuous collision detection");
			report(Serialization::SerializedCallback::Create<bool>::From("Enable CCD", Callback<bool>(&Rigidbody::EnableCCD, this), serializer));
		}

		// Lock flags
		{
			static const auto serializer = Serialization::DefaultSerializer<Physics::DynamicBody::LockFlagMask>::Create(
				"Lock", "Lock per axis rotation and or movement simulation", 
				std::vector<Reference<const Object>> { Physics::DynamicBody::LockFlagMaskEnumAttribute() });
			report(Serialization::SerializedCallback::Create<Physics::DynamicBody::LockFlagMask>::From(
				"SetLockFlags", Callback<Physics::DynamicBody::LockFlagMask>(&Rigidbody::SetLockFlags, this), serializer));
		}

		// Velocity
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create("Velocity", "Velocity for the Rigidbody");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetVelocity", Callback<const Vector3&>(&Rigidbody::SetVelocity, this), serializer));
		}

		// Angular Velocity
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create("Angular Velocity", "Angular velocity for the Rigidbody");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetAngularVelocity", Callback<const Vector3&>(&Rigidbody::SetAngularVelocity, this), serializer));
		}

		// Add force
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create("Force", "Force to add");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"AddForce", Callback<const Vector3&>(&Rigidbody::AddForce, this), serializer));
		}

		// Add velocity
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create("Delta", "velocity to add");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"AddVelocity", Callback<const Vector3&>(&Rigidbody::AddVelocity, this), serializer));
		}
	}

#undef ACCESS_BODY_PROPERTY

	void Rigidbody::PrePhysicsSynch() {
		Physics::DynamicBody* body = GetBody();
		if (body == nullptr) return;
		Matrix4 curPose = GetPose(GetTransfrom());
		if (m_lastPose != curPose) {
			body->SetPose(curPose);
			m_lastPose = curPose;
		}
		if ((m_dirtyFlags & RIGIDBODY_DIRTY_FLAG_VELOCITY) != 0u) {
			m_unappliedVelocity = (m_velocity - m_lastVelocity);
			body->AddVelocity(m_unappliedVelocity);
			m_lastVelocity = m_velocity;
		}
		if ((m_dirtyFlags & RIGIDBODY_DIRTY_FLAG_ANGULAR_VELOCITY) != 0u) {
			body->SetAngularVelocity(m_angularVelocity);
		}
		m_dirtyFlags = 0u;
	}

	void Rigidbody::PostPhysicsSynch() {
		if (m_dynamicBody == nullptr) return;
		
		// Update velocity:
		{
			const Vector3 deltaVelocity = (m_velocity - m_lastVelocity);
			m_lastVelocity = m_dynamicBody->Velocity() + m_unappliedVelocity;
			m_unappliedVelocity = Vector3(0.0f);
			if ((m_dirtyFlags & RIGIDBODY_DIRTY_FLAG_VELOCITY) != 0u)
				m_velocity = (m_lastVelocity + deltaVelocity);
			else m_velocity = m_lastVelocity;
		}

		// Update angular velocity:
		{
			if ((m_dirtyFlags & RIGIDBODY_DIRTY_FLAG_ANGULAR_VELOCITY) != 0u) {
				m_dynamicBody->SetAngularVelocity(m_angularVelocity);
				m_dirtyFlags &= (~RIGIDBODY_DIRTY_FLAG_ANGULAR_VELOCITY);
			}
			else m_angularVelocity = m_dynamicBody->AngularVelocity();
		}

		m_dirtyFlags = 0u;

		Transform* transform = GetTransfrom();
		if (transform == nullptr) {
			m_lastPose = m_dynamicBody->GetPose();
			return;
		}
		Matrix4 pose = m_dynamicBody->GetPose();
		transform->SetWorldPosition(pose[3]);
		pose[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		transform->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(pose));
		m_lastPose = GetPose(transform);
	}

	void Rigidbody::OnComponentEnabled() {
		Physics::DynamicBody* body = GetBody();
		if (body != nullptr)
			body->SetActive(ActiveInHierarchy());
	}

	void Rigidbody::OnComponentDisabled() {
		Physics::DynamicBody* body = GetBody();
		if (body != nullptr)
			body->SetActive(ActiveInHierarchy());
	}

	void Rigidbody::OnComponentDestroyed() {
		m_dynamicBody = nullptr;
	}

	Physics::DynamicBody* Rigidbody::GetBody()const {
		if (Destroyed()) return nullptr;
		else if (m_dynamicBody == nullptr) {
			m_lastPose = GetPose(GetTransfrom());
			m_dynamicBody = Context()->Physics()->AddRigidBody(m_lastPose, ActiveInHierarchy());
		}
		return m_dynamicBody;
	}
}
