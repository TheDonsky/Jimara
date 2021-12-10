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
		: Component(parent, name) {
		OnDestroyed() += Callback(&Rigidbody::ClearWhenDestroyed, this);
	}

	Rigidbody::~Rigidbody() {
		OnDestroyed() -= Callback(&Rigidbody::ClearWhenDestroyed, this);
		ClearWhenDestroyed(this);
	}

	namespace {
		class RigidbodySerializer : public virtual ComponentSerializer {
		public:
			inline RigidbodySerializer()
				: ItemSerializer("Rigidbody", "Rigidbody component"), ComponentSerializer("Jimara/Physics/Rigidbody") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override {
				return Object::Instantiate<Rigidbody>(parent, "Rigidbody");
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				Rigidbody* target = dynamic_cast<Rigidbody*>((Component*)targetAddr);
				target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

				static const Reference<const Serialization::FloatSerializer> colorSerializer = Serialization::FloatSerializer::Create(
					"Mass", "Rigidbody mass",
					Function<float, void*>([](void* targetAddr) { return dynamic_cast<Rigidbody*>((Component*)targetAddr)->Mass(); }),
					Callback<const float&, void*>([](const float& value, void* targetAddr) { dynamic_cast<Rigidbody*>((Component*)targetAddr)->SetMass(value); }));
				recordElement(Serialization::SerializedObject(colorSerializer, targetAddr));

				static const Reference<const Serialization::BoolSerializer> kinematicSerializer = Serialization::BoolSerializer::Create(
					"Kinematic", "True, if the rigidbody should be kinematic",
					Function<bool, void*>([](void* targetAddr) { return dynamic_cast<Rigidbody*>((Component*)targetAddr)->IsKinematic(); }),
					Callback<const bool&, void*>([](const bool& value, void* targetAddr) { dynamic_cast<Rigidbody*>((Component*)targetAddr)->SetKinematic(value); }));
				recordElement(Serialization::SerializedObject(kinematicSerializer, targetAddr));

				static const Reference<const Serialization::Uint32Serializer> lockFlagsSerializer = Serialization::Uint32Serializer::Create(
					"Lock", "Lock per axis rotation and or movement simulation",
					Function<uint32_t, void*>([](void* targetAddr) { return (uint32_t)(dynamic_cast<Rigidbody*>((Component*)targetAddr)->GetLockFlags()); }),
					Callback<const uint32_t&, void*>([](const uint32_t& value, void* targetAddr) {
						dynamic_cast<Rigidbody*>((Component*)targetAddr)->SetLockFlags((Physics::DynamicBody::LockFlagMask)value); }),
						{ Object::Instantiate<Serialization::Uint32EnumAttribute>(std::vector<Serialization::Uint32EnumAttribute::Choice>({
								Serialization::Uint32EnumAttribute::Choice("MOVEMENT_X", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_X)),
								Serialization::Uint32EnumAttribute::Choice("MOVEMENT_Y", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_Y)),
								Serialization::Uint32EnumAttribute::Choice("MOVEMENT_Z", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::MOVEMENT_Z)),
								Serialization::Uint32EnumAttribute::Choice("ROTATION_X", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_X)),
								Serialization::Uint32EnumAttribute::Choice("ROTATION_Y", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_Y)),
								Serialization::Uint32EnumAttribute::Choice("ROTATION_Z", static_cast<uint32_t>(Physics::DynamicBody::LockFlag::ROTATION_Z))
							}), true) });
				recordElement(Serialization::SerializedObject(lockFlagsSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const RigidbodySerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry RIGIDBODY_SERIALIZER;
	}

	Reference<const ComponentSerializer> Rigidbody::GetSerializer()const {
		return RigidbodySerializer::Instance();
	}

	template<> void TypeIdDetails::OnRegisterType<Rigidbody>() { RIGIDBODY_SERIALIZER = RigidbodySerializer::Instance(); }
	template<> void TypeIdDetails::OnUnregisterType<Rigidbody>() { RIGIDBODY_SERIALIZER = nullptr; }

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

	Physics::DynamicBody* Rigidbody::GetBody()const {
		if (m_dead) return nullptr;
		else if (m_dynamicBody == nullptr) {
			m_lastPose = GetPose(GetTransfrom());
			m_dynamicBody = Context()->Physics()->AddRigidBody(m_lastPose);
		}
		return m_dynamicBody;
	}

	void Rigidbody::ClearWhenDestroyed(Component*) {
		m_dead = true;
		m_dynamicBody = nullptr;
	}
}
