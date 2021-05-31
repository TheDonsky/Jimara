#include "PhysXBody.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXBody::PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor)
				: m_scene(scene), m_actor(actor), m_active(false) {}

			PhysXBody::~PhysXBody() {
				SetActive(false);
				if (m_actor != nullptr) {
					m_actor->release();
					m_actor = nullptr;
				}
			}

			bool PhysXBody::Active()const { return m_active; }

			void PhysXBody::SetActive(bool active) {
				if (m_active == active) return;
				m_active = active;
				if (m_actor == nullptr) return;
				else if (m_active) (*m_scene)->addActor(*m_actor);
				else (*m_scene)->removeActor(*m_actor);
			}

			Matrix4 PhysXBody::GetPose()const { return Translate(physx::PxMat44(m_actor->getGlobalPose())); }

			void PhysXBody::SetPose(const Matrix4& transform) { m_actor->setGlobalPose(physx::PxTransform(Translate(transform))); }

			PhysXScene* PhysXBody::Scene()const { return m_scene; }

			PhysXBody::operator physx::PxRigidActor* ()const { return m_actor; }
		}
	}
}
