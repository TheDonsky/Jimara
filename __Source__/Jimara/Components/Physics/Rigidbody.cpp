#include "Rigidbody.h"


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

#define ACCESS_BODY_PROPERTY(if_not_null, if_null) Physics::DynamicBody* body = GetBody(); if (body != nullptr) if_not_null else if_null

	float Rigidbody::Mass()const { ACCESS_BODY_PROPERTY({ return body->Mass(); }, { return 0.0f; }); }

	void Rigidbody::SetMass(float mass) { ACCESS_BODY_PROPERTY({ body->SetMass(mass); }, {}); }

	bool Rigidbody::IsKinematic()const { ACCESS_BODY_PROPERTY({ return body->IsKinematic(); }, { return false; }); }

	void Rigidbody::SetKinematic(bool kinematic) { ACCESS_BODY_PROPERTY({ body->SetKinematic(kinematic); }, {}); }

	Vector3 Rigidbody::Velocity()const { ACCESS_BODY_PROPERTY({ return body->Velocity(); }, { return Vector3(0.0f); }); }

	void Rigidbody::SetVelocity(const Vector3& velocity) { ACCESS_BODY_PROPERTY({ body->SetVelocity(velocity); }, {}); }

	Physics::DynamicBody::LockMask Rigidbody::GetLockFlags()const { ACCESS_BODY_PROPERTY({ return body->GetLockFlags(); }, { return 0; }); }

	void Rigidbody::SetLockFlags(Physics::DynamicBody::LockMask mask) { ACCESS_BODY_PROPERTY({ body->SetLockFlags(mask); }, {}); }

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
