#include "Collider.h"


namespace Jimara {
	Collider::Collider() {
		OnDestroyed() += Callback(&Collider::ClearWhenDestroyed, this);
	}

	Collider::~Collider() {
		OnDestroyed() -= Callback(&Collider::ClearWhenDestroyed, this);
		ClearWhenDestroyed(this);
	}

	void Collider::PrePhysicsSynch() {
		if (m_dead) return;

		Reference<Rigidbody> rigidbody = GetComponentInParents<Rigidbody>();
		if (m_rigidbody != rigidbody) {
			m_rigidbody = rigidbody;
			if (m_rigidbody != nullptr) m_body = m_rigidbody->GetBody();
			else m_body = nullptr;
			m_collider = nullptr;
			m_dirty = true;
		}
		Matrix4 transformation;
		Matrix4 rotation;
		Transform* transform = GetTransfrom();
		if (transform != nullptr) {
			transformation = transform->WorldMatrix();
			rotation = transform->WorldRotationMatrix();
		}
		else transformation = rotation = Math::Identity();

		Matrix4 curPose;
		Vector3 curScale = Math::LossyScale(transformation, rotation);
		auto setPose = [&](const Matrix4& trans, const Matrix4& rot) { 
			curPose = rot; 
			curPose[3] = trans[3]; 
		};
		if (m_rigidbody == nullptr) {
			setPose(transformation, rotation);
			if (m_body == nullptr) {
				m_body = Context()->Physics()->AddStaticBody(curPose);
				m_collider = nullptr;
				m_dirty = true;
			}
			else if (curPose != m_lastPose) 
				m_body->SetPose(curPose);
		}
		else {
			Transform* rigidTransform = m_rigidbody->GetTransfrom();
			if (rigidTransform != nullptr && transform != nullptr) {
				Matrix4 relativeTransformation = Math::Identity();
				Matrix4 relativeRotation = Math::Identity();
				for (Transform* trans = transform; trans != rigidTransform; trans = trans->GetComponentInParents<Transform>(false)) {
					relativeTransformation = trans->LocalMatrix() * relativeTransformation;
					relativeRotation = trans->LocalRotationMatrix() * relativeRotation;
				}
				setPose(relativeTransformation, relativeRotation);
			}
			else setPose(transformation, rotation);
		}
		if (m_lastScale != curScale) {
			m_lastScale = curScale;
			m_dirty = true;
		}

		if ((m_dirty || (m_collider == nullptr)) && (m_body != nullptr))
			m_collider = GetPhysicsCollider(m_collider, m_body, m_lastScale);
		if ((m_collider != nullptr) && (m_rigidbody != nullptr) && (m_dirty || curPose != m_lastPose))
			m_collider->SetLocalPose(curPose);
		m_lastPose = curPose;
		m_dirty = false;
	}

	void Collider::ColliderDirty() { m_dirty = true; }

	void Collider::ClearWhenDestroyed(Component*) {
		m_dead = true;
		m_rigidbody = nullptr;
		m_body = nullptr;
		m_collider = nullptr;
	}
}
