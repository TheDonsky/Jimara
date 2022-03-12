#include "Rigidbody.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


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
				
				static const Reference<const FieldSerializer> colorSerializer = Serialization::FloatSerializer::For<Rigidbody>(
					"Mass", "Rigidbody mass",
					[](Rigidbody* target) { return target->Mass(); },
					[](const float& value, Rigidbody* target) { target->SetMass(value); });
				recordElement(colorSerializer->Serialize(target));

				static const Reference<const FieldSerializer> kinematicSerializer = Serialization::BoolSerializer::For<Rigidbody>(
					"Kinematic", "True, if the rigidbody should be kinematic",
					[](Rigidbody* target) { return target->IsKinematic(); },
					[](const bool& value, Rigidbody* target) { target->SetKinematic(value); });
				recordElement(kinematicSerializer->Serialize(target));

				static const Reference<const FieldSerializer> lockFlagsSerializer = Serialization::Uint32Serializer::For<Rigidbody>(
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
				recordElement(lockFlagsSerializer->Serialize(target));

				static const Reference<const FieldSerializer> velocitySerializer = Serialization::Vector3Serializer::For<Rigidbody>(
					"Velocity", "Current/Initial velocity of the Rigidbody",
					[](Rigidbody* target) -> Vector3 { return target->Velocity(); },
					[](const Vector3& value, Rigidbody* target) { target->SetVelocity(value); });
				recordElement(velocitySerializer->Serialize(target));
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

	Vector3 Rigidbody::Velocity()const { ACCESS_BODY_PROPERTY({ return body->Velocity(); }, { return Vector3(0.0f); }); }

	void Rigidbody::SetVelocity(const Vector3& velocity) { 
		if (m_kinematic) return;
		ACCESS_BODY_PROPERTY({ body->SetVelocity(velocity); }, {}); 
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
