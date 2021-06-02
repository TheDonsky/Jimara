#include "Rigidbody.h"


namespace Jimara {
	namespace {
		inline static Matrix4 GetPose(Transform* transform) {
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

	Physics::DynamicBody* Rigidbody::GetBody() {
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
