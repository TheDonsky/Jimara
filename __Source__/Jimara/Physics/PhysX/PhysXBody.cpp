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

					inline virtual ~PhysXCollider() { SetActive(false); }

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

					inline virtual bool IsTrigger()const override { return ((physx::PxU8)m_shape->getFlags() & physx::PxShapeFlag::eTRIGGER_SHAPE) != 0; }

					inline virtual void SetTrigger(bool trigger) override {
						physx::PxShapeFlags flags = m_shape->getFlags();
						if (trigger) {
							flags |= physx::PxShapeFlag::eTRIGGER_SHAPE;
							flags &= ~(physx::PxShapeFlag::eSIMULATION_SHAPE);
						}
						else {
							flags &= ~physx::PxShapeFlag::eTRIGGER_SHAPE;
							flags |= physx::PxShapeFlag::eSIMULATION_SHAPE;
						}
						m_shape->setFlags(flags);
					}

					inline physx::PxShape* Shape()const { return m_shape; }

					template<typename ColliderType, typename ShapeType>
					inline static Reference<ColliderType> Create(PhysXBody* body, const ShapeType& geometry, PhysicsMaterial* material, bool enabled) {
						PhysXInstance* instance = dynamic_cast<PhysXInstance*>(body->Scene()->APIInstance());
						if (instance == nullptr) {
							body->Scene()->APIInstance()->Log()->Error("PhysXCollider::Create - Invalid API instance!");
							return nullptr;
						}
						Reference<PhysXMaterial> apiMaterial = dynamic_cast<PhysXMaterial*>(material);
						if (apiMaterial == nullptr) apiMaterial = PhysXMaterial::Default(instance);
						if (apiMaterial == nullptr) {
							body->Scene()->APIInstance()->Log()->Error("PhysXCollider::Create - Material not provided or of an incorrect type and default material could not be retrieved!");
							return nullptr;
						}
						return Object::Instantiate<ColliderType>(body, (*instance)->createShape(ColliderType::Geometry(geometry), *(*apiMaterial), true), enabled);
					}
				};

#pragma warning(disable: 4250)
				class PhysXBoxCollider : public virtual PhysXCollider, public virtual PhysicsBoxCollider {
				public:
					inline PhysXBoxCollider(PhysXBody* body, physx::PxShape* shape, bool active) : PhysXCollider(body, shape, active) {}

					inline static physx::PxBoxGeometry Geometry(const BoxShape& shape) {
						return physx::PxBoxGeometry(shape.size.x * 0.5f, shape.size.y * 0.5f, shape.size.z * 0.5f);
					}

					inline virtual void Update(const BoxShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
					}
				};

				class PhysXSphereCollider : public virtual PhysXCollider, public virtual PhysicsSphereCollider {
				public:
					inline PhysXSphereCollider(PhysXBody* body, physx::PxShape* shape, bool active) : PhysXCollider(body, shape, active) {}

					inline static physx::PxSphereGeometry Geometry(const SphereShape& shape) {
						return physx::PxSphereGeometry(shape.radius);
					}

					inline virtual void Update(const SphereShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
					}
				};

				class PhysXCapusuleCollider : public virtual PhysXCollider, public virtual PhysicsCapsuleCollider {
				public:
					inline PhysXCapusuleCollider(PhysXBody* body, physx::PxShape* shape, bool active) : PhysXCollider(body, shape, active) {}

					inline static physx::PxCapsuleGeometry Geometry(const CapsuleShape& shape) {
						return physx::PxCapsuleGeometry(shape.radius, shape.height * 0.5f);
					}

					inline virtual void Update(const CapsuleShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
					}
				};
#pragma warning(default: 4250)
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

			Reference<PhysicsBoxCollider> PhysXBody::AddCollider(const BoxShape& box, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create<PhysXBoxCollider>(this, box, material, enabled);
			}

			Reference<PhysicsSphereCollider> PhysXBody::AddCollider(const SphereShape& sphere, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create<PhysXSphereCollider>(this, sphere, material, enabled);
			}

			Reference<PhysicsCapsuleCollider> PhysXBody::AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, bool enabled) {
				return PhysXCollider::Create<PhysXCapusuleCollider>(this, capsule, material, enabled);
			}

			PhysXScene* PhysXBody::Scene()const { return m_scene; }

			PhysXBody::operator physx::PxRigidActor* ()const { return m_actor; }
		}
	}
}
