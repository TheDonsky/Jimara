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
	}

	Rigidbody::Rigidbody(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	Rigidbody::~Rigidbody() {
		OnComponentDestroyed();
	}

	namespace {
		class RigidbodySerializer : public virtual ComponentSerializer::Of<Rigidbody> {
		public:
			inline RigidbodySerializer()
				: ItemSerializer("Jimara/Physics/Rigidbody", "Rigidbody component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Rigidbody* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				JIMARA_SERIALIZE_FIELDS(target, recordElement, {
					JIMARA_SERIALIZE_FIELD_GET_SET(Mass, SetMass, "Mass", "Rigidbody mass");
					JIMARA_SERIALIZE_FIELD_GET_SET(IsKinematic, SetKinematic, "Kinematic", "True, if the rigidbody should be kinematic");
					JIMARA_SERIALIZE_FIELD_GET_SET(CCDEnabled, EnableCCD, "Enable CCD", "Enables Continuous collision detection");
					{
						static const Reference<const FieldSerializer> serializer = Serialization::Uint32Serializer::For<Rigidbody>(
							"Lock", "Lock per axis rotation and or movement simulation",
							[](Rigidbody* target) { return (uint32_t)(target->GetLockFlags()); },
							[](const uint32_t& value, Rigidbody* target) {
								target->SetLockFlags((Physics::DynamicBody::LockFlagMask)value); },
								{ Object::Instantiate<Serialization::Uint32EnumAttribute>(std::vector<Serialization::Uint32EnumAttribute::Choice>({
										Serialization::Uint32EnumAttribute::Choice("MOVEMENT_X", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_X)),
										Serialization::Uint32EnumAttribute::Choice("MOVEMENT_Y", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_Y)),
										Serialization::Uint32EnumAttribute::Choice("MOVEMENT_Z", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_Z)),
										Serialization::Uint32EnumAttribute::Choice("ROTATION_X", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_X)),
										Serialization::Uint32EnumAttribute::Choice("ROTATION_Y", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_Y)),
										Serialization::Uint32EnumAttribute::Choice("ROTATION_Z", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_Z))
									}), true) });
						recordElement(serializer->Serialize(target));
					}
					JIMARA_SERIALIZE_FIELD_GET_SET(Velocity, SetVelocity, "Velocity", "Current/Initial velocity of the Rigidbody");
					JIMARA_SERIALIZE_FIELD_GET_SET(AngularVelocity, SetAngularVelocity, "Angular Velocity", "Current/Initial angular velocity of the Rigidbody");
					});
			}

			inline static const ComponentSerializer* Instance() {
				static const RigidbodySerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Rigidbody>(const Callback<const Object*>& report) { report(RigidbodySerializer::Instance()); }

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

	Vector3 Rigidbody::Velocity()const { ACCESS_BODY_PROPERTY({ return body->Velocity(); }, { return Vector3(0.0f); }); }

	void Rigidbody::SetVelocity(const Vector3& velocity) { 
		if (m_kinematic) return;
		ACCESS_BODY_PROPERTY({ body->SetVelocity(velocity); }, {}); 
	}

	Vector3 Rigidbody::AngularVelocity()const { ACCESS_BODY_PROPERTY({ return body->AngularVelocity(); }, { return Vector3(0.0f); }); }

	void Rigidbody::SetAngularVelocity(const Vector3& velocity) {
		if (m_kinematic) return;
		ACCESS_BODY_PROPERTY({ body->SetAngularVelocity(velocity); }, {});
	}

	Physics::DynamicBody::LockFlagMask Rigidbody::GetLockFlags()const { ACCESS_BODY_PROPERTY({ return body->GetLockFlags(); }, { return 0; }); }

	void Rigidbody::SetLockFlags(Physics::DynamicBody::LockFlagMask mask) { ACCESS_BODY_PROPERTY({ body->SetLockFlags(mask); }, {}); }

#undef ACCESS_BODY_PROPERTY

	void Rigidbody::PrePhysicsSynch() {
		Physics::DynamicBody* body = GetBody();
		if (body == nullptr) return;
		Matrix4 curPose = GetPose(GetTransfrom());
		if (m_lastPose != curPose) {
			body->SetPose(curPose);
			m_lastPose = curPose;
		}
	}

	void Rigidbody::PostPhysicsSynch() {
		if (m_dynamicBody == nullptr) return;
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
			body->SetActive(ActiveInHeirarchy());
	}

	void Rigidbody::OnComponentDisabled() {
		Physics::DynamicBody* body = GetBody();
		if (body != nullptr)
			body->SetActive(ActiveInHeirarchy());
	}

	void Rigidbody::OnComponentDestroyed() {
		m_dynamicBody = nullptr;
	}

	Physics::DynamicBody* Rigidbody::GetBody()const {
		if (Destroyed()) return nullptr;
		else if (m_dynamicBody == nullptr) {
			m_lastPose = GetPose(GetTransfrom());
			m_dynamicBody = Context()->Physics()->AddRigidBody(m_lastPose, ActiveInHeirarchy());
		}
		return m_dynamicBody;
	}
}
