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
		if (m_body == nullptr) {
			m_body = Context()->Physics()->AddRigidBody(GetPose(GetTransfrom()));
			m_lastPose = m_body->GetPose();
			return;
		}
		Matrix4 curPose = GetPose(GetTransfrom());
		if (m_lastPose != curPose) {
			m_body->SetPose(curPose);
			m_lastPose = curPose;
		}
	}

	void Rigidbody::PostPhysicsSynch() {
		if (m_body == nullptr) return;
		Transform* transform = GetTransfrom();
		if (transform == nullptr) {
			m_lastPose = m_body->GetPose();
			return;
		}
		Matrix4 pose = m_body->GetPose();
		transform->SetWorldPosition(pose[3]);
		pose[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		transform->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(pose));
		m_lastPose = GetPose(transform);
	}

	void Rigidbody::ClearWhenDestroyed(Component*) {
		m_dead = true;
		m_body = nullptr;
	}
}
