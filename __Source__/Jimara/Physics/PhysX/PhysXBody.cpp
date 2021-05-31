#include "PhysXBody.h"
#include "PhysXMaterial.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				class PhysXCollider : public virtual PhysicsCollider {
				private:
					const Reference<PhysXBody> m_body;
					const PhysXReference<physx::PxShape> m_shape;
					std::atomic<bool> m_active = false;

				public:
					inline PhysXCollider(PhysXBody* body, physx::PxShape* shape, bool active)
						: m_body(body), m_shape(shape) {
						if (m_shape == nullptr) m_body->Scene()->APIInstance()->Log()->Fatal("PhysXCollider - null Shape!");
						else {
							m_shape->release();
							SetActive(active);
						}
					}

					inline virtual bool Active()const override { return m_active; }

					inline virtual void SetActive(bool active) override {
						if (m_active == active) return;
						m_active = active;
						if (m_shape == nullptr) return;
						else if (m_active) ((physx::PxRigidActor*)(*m_body))->attachShape(*m_shape);
						else ((physx::PxRigidActor*)(*m_body))->detachShape(*m_shape);
					}

					inline virtual Matrix4 GetLocalPose()const override { return Translate(physx::PxMat44(m_shape->getLocalPose())); }

					inline virtual void SetLocalPose(const Matrix4& transform) override { m_shape->setLocalPose(physx::PxTransform(Translate(transform))); }

					inline static Reference<PhysXCollider> Create(PhysXBody* body, const physx::PxGeometry& geometry, PhysicsMaterial* material, bool enabled) {
						PhysXInstance* instance = reinterpret_cast<PhysXInstance*>(body->Scene()->APIInstance());
						if (instance == nullptr) {
							body->Scene()->APIInstance()->Log()->Error("PhysXCollider::Create - Invalid API instance!");
							return nullptr;
						}
						Reference<PhysXMaterial> apiMaterial = reinterpret_cast<PhysXMaterial*>(material);
						if (apiMaterial == nullptr) apiMaterial = PhysXMaterial::Default(instance);
						if (apiMaterial == nullptr) {
							body->Scene()->APIInstance()->Log()->Error("PhysXCollider::Create - Material not provided or of an incorrect type and default material could not be retrieved!");
							return nullptr;
						}
						return Object::Instantiate<PhysXCollider>(body, (*instance)->createShape(geometry, *(*apiMaterial)), enabled);
					}
				};
			}

			PhysXBody::PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor, bool enabled)
				: m_scene(scene), m_actor(actor), m_active(false) {
				if (m_actor == nullptr) m_scene->APIInstance()->Log()->Fatal("PhysXBody::PhysXBody - null Actor pointer!");
				else SetActive(enabled);
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
				else if (m_active) (*m_scene)->addActor(*m_actor);
				else (*m_scene)->removeActor(*m_actor);
			}

			Matrix4 PhysXBody::GetPose()const { return Translate(physx::PxMat44(m_actor->getGlobalPose())); }

			void PhysXBody::SetPose(const Matrix4& transform) { m_actor->setGlobalPose(physx::PxTransform(Translate(transform))); }

			Reference<PhysicsCollider> PhysXBody::AddCollider(const BoxShape& box, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create(this, physx::PxBoxGeometry(box.size.x * 0.5f, box.size.y * 0.5f, box.size.z * 0.5f), material, enabled);
			}

			Reference<PhysicsCollider> PhysXBody::AddCollider(const SphereShape& sphere, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create(this, physx::PxSphereGeometry(sphere.radius), material, enabled);
			}

			Reference<PhysicsCollider> PhysXBody::AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create(this, physx::PxCapsuleGeometry(capsule.radius, capsule.height * 0.5f), material, enabled);
			}

			PhysXScene* PhysXBody::Scene()const { return m_scene; }

			PhysXBody::operator physx::PxRigidActor* ()const { return m_actor; }
		}
	}
}
