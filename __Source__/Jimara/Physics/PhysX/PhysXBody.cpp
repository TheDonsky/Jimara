#include "PhysXBody.h"
#include "PhysXCollider.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXBody::PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor, bool enabled)
				: m_scene(scene), m_actor(actor), m_active(false) {
				if (m_actor == nullptr) {
					m_scene->APIInstance()->Log()->Fatal("PhysXBody::PhysXBody - null Actor pointer!");
					return;
				}
				PhysXScene::WriteLock lock(m_scene);
				m_actor->userData = this;
				SetActive(enabled);
			}

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
				PhysXScene::WriteLock lock(m_scene);
				if (m_active) (*m_scene)->addActor(*m_actor);
				else (*m_scene)->removeActor(*m_actor);
			}

			Matrix4 PhysXBody::GetPose()const { PhysXScene::ReadLock lock(m_scene); return Translate(physx::PxMat44(m_actor->getGlobalPose())); }

			void PhysXBody::SetPose(const Matrix4& transform) { PhysXScene::WriteLock lock(m_scene); m_actor->setGlobalPose(physx::PxTransform(Translate(transform))); }

			Reference<PhysicsBoxCollider> PhysXBody::AddCollider(const BoxShape& box, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXBoxCollider::Create(this, box, material, listener, enabled);
			}

			Reference<PhysicsSphereCollider> PhysXBody::AddCollider(const SphereShape& sphere, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXSphereCollider::Create(this, sphere, material, listener, enabled);
			}

			Reference<PhysicsCapsuleCollider> PhysXBody::AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXCapusuleCollider::Create(this, capsule, material, listener, enabled);
			}

			PhysXScene* PhysXBody::Scene()const { return m_scene; }

			PhysXBody::operator physx::PxRigidActor* ()const { return m_actor; }
		}
	}
}
#pragma warning(default: 26812)
