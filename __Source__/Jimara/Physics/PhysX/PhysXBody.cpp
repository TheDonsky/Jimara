#include "PhysXBody.h"
#include "PhysXMaterial.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			namespace {
				class PhysXCollider : public virtual SingleMaterialCollider {
				private:
					const Reference<PhysXBody> m_body;
					const PhysXReference<physx::PxShape> m_shape;
					Reference<PhysXMaterial> m_material;
					std::atomic<bool> m_active = false;

					class ContactInformation : public virtual ContactInfo {
					private:
						PhysicsCollider* const m_collider;
						PhysicsCollider* const m_otherCollider;
						const ContactType m_type;
						const PhysicsCollider::ContactPoint* const m_points;
						const size_t m_pointCount;

					public:
						inline ContactInformation(PhysicsCollider* self, PhysicsCollider* other, ContactType type, const PhysicsCollider::ContactPoint* points, size_t pointCount)
							: m_collider(self), m_otherCollider(other), m_type(type), m_points(points), m_pointCount(pointCount) {}

						inline virtual PhysicsCollider* Collider()const override { return m_collider; }
						inline virtual PhysicsCollider* OtherCollider()const override { return m_otherCollider; }
						inline virtual ContactType EventType()const override { return m_type; }
						inline virtual size_t ContactPointCount()const override { return m_pointCount; }
						inline virtual PhysicsCollider::ContactPoint ContactPoint(size_t index)const override { return m_points[index]; }
					};

					struct EventListener : public virtual PhysXScene::ContactEventListener {
						PhysXCollider* owner = nullptr;

						inline virtual void OnContact(
							physx::PxShape* shape, physx::PxShape* otherShape, PhysicsCollider::ContactType type
							, const PhysicsCollider::ContactPoint* points, size_t pointCount) override {
							if (owner == nullptr) return;
							if (owner->m_shape != shape || otherShape == nullptr) return;
							PhysXScene::ContactEventListener* listener = (PhysXScene::ContactEventListener*)otherShape->userData;
							if (listener == nullptr) return;
							EventListener* eventListener = dynamic_cast<EventListener*>(listener);
							if (eventListener == nullptr) return;
							PhysXCollider* otherCollider = eventListener->owner;
							if (otherCollider == nullptr) return;
							owner->NotifyContact(ContactInformation(owner, otherCollider, type, points, pointCount));
						}
					} m_eventListener;

				protected:
					inline PhysXCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active)
						: PhysicsCollider(listener), m_body(body), m_shape(shape), m_material(material) {
						if (m_shape == nullptr) {
							m_body->Scene()->APIInstance()->Log()->Fatal("PhysXCollider - null Shape!");
							return;
						}
						m_eventListener.owner = this;
						PhysXScene::ContactEventListener* userData = &m_eventListener;
						m_shape->userData = userData;
						m_shape->release();
						SetActive(active);
					}

				public:
					inline virtual ~PhysXCollider() { 
						m_shape->userData = nullptr;
						SetActive(false); 
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

					virtual PhysicsMaterial* Material()const override { return m_material; }

					virtual void SetMaterial(PhysicsMaterial* material) {
						Reference<PhysXMaterial> materialToSet = dynamic_cast<PhysXMaterial*>(material);
						if (materialToSet == nullptr) {
							materialToSet = PhysXMaterial::Default(dynamic_cast<PhysXInstance*>(m_body->Scene()->APIInstance()));
							if (materialToSet == nullptr) {
								m_body->Scene()->APIInstance()->Log()->Fatal("PhysXCollider::SetMaterial - Failed get default material!");
								return;
							}
						}
						if (materialToSet == m_material) return;
						physx::PxMaterial* apiMaterial = (*materialToSet);
						m_shape->setMaterials(&apiMaterial, 1);
						m_material = materialToSet;
					}

					inline physx::PxShape* Shape()const { return m_shape; }

					template<typename ColliderType, typename ShapeType>
					inline static Reference<ColliderType> Create(PhysXBody* body, const ShapeType& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
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
						return Object::Instantiate<ColliderType>(body, (*instance)->createShape(ColliderType::Geometry(geometry), *(*apiMaterial), true), apiMaterial, listener, enabled, geometry);
					}
				};

#pragma warning(disable: 4250)
				class PhysXBoxCollider : public virtual PhysXCollider, public virtual PhysicsBoxCollider {
				public:
					inline PhysXBoxCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, const BoxShape&)
						: PhysicsCollider(listener), PhysXCollider(body, shape, material, listener, active) {}

					inline static physx::PxBoxGeometry Geometry(const BoxShape& shape) {
						return physx::PxBoxGeometry(shape.size.x * 0.5f, shape.size.y * 0.5f, shape.size.z * 0.5f);
					}

					inline virtual void Update(const BoxShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
					}
				};

				class PhysXSphereCollider : public virtual PhysXCollider, public virtual PhysicsSphereCollider {
				public:
					inline PhysXSphereCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, const SphereShape&)
						: PhysicsCollider(listener), PhysXCollider(body, shape, material, listener, active) {}

					inline static physx::PxSphereGeometry Geometry(const SphereShape& shape) {
						return physx::PxSphereGeometry(shape.radius);
					}

					inline virtual void Update(const SphereShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
					}
				};

				class PhysXCapusuleCollider : public virtual PhysXCollider, public virtual PhysicsCapsuleCollider {
				private:
					typedef std::pair<Matrix4, Matrix4> Wrangler;
					inline static Wrangler Wrangle(CapsuleShape::Alignment alignment) {
						static const Wrangler* FN = []() {
							static Wrangler fn[3];
							fn[static_cast<uint8_t>(CapsuleShape::Alignment::X)] = Wrangler(Math::Identity(), Math::Identity());
							fn[static_cast<uint8_t>(CapsuleShape::Alignment::Y)] = Wrangler(
								Matrix4(Vector4(0.0f, 1.0, 0.0f, 0.0f), Vector4(-1.0f, 0.0, 0.0f, 0.0f), Vector4(0.0f, 0.0, 1.0f, 0.0f), Vector4(0.0f, 0.0, 0.0f, 1.0f)),
								Matrix4(Vector4(0.0f, -1.0, 0.0f, 0.0f), Vector4(1.0f, 0.0, 0.0f, 0.0f), Vector4(0.0f, 0.0, 1.0f, 0.0f), Vector4(0.0f, 0.0, 0.0f, 1.0f)));
							fn[static_cast<uint8_t>(CapsuleShape::Alignment::Z)] = Wrangler(
								Matrix4(Vector4(0.0f, 0.0, -1.0f, 0.0f), Vector4(0.0f, 1.0, 0.0f, 0.0f), Vector4(1.0f, 0.0, 0.0f, 0.0f), Vector4(0.0f, 0.0, 0.0f, 1.0f)),
								Matrix4(Vector4(0.0f, 0.0, 1.0f, 0.0f), Vector4(0.0f, 1.0, 0.0f, 0.0f), Vector4(-1.0f, 0.0, 0.0f, 0.0f), Vector4(0.0f, 0.0, 0.0f, 1.0f)));
							for (size_t i = 0; i < 3; i++) {
								Wrangler w = fn[i];
								assert((w.first * w.second) == Math::Identity());
							}
							return fn;
						}();
						if (static_cast<uint8_t>(alignment) >= 3) alignment = CapsuleShape::Alignment::Y;
						return FN[static_cast<uint8_t>(alignment)];
					}
					Wrangler m_wrangle = Wrangle(CapsuleShape::Alignment::X);

					void SetAlignment(CapsuleShape::Alignment alignment) {
						Wrangler newFn = Wrangle(alignment);
						Matrix4 raw = Translate(Shape()->getLocalPose());
						Matrix4 unwrangled = raw * m_wrangle.second;
						Matrix4 rewrangled = unwrangled * newFn.first;
						Shape()->setLocalPose(physx::PxTransform(Translate(rewrangled)));
						m_wrangle = newFn;
					}

				public:
					inline PhysXCapusuleCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, const CapsuleShape& capsule)
						: PhysicsCollider(listener), PhysXCollider(body, shape, material, listener, active) {
						SetAlignment(capsule.alignment);
					}

					inline static physx::PxCapsuleGeometry Geometry(const CapsuleShape& shape) {
						return physx::PxCapsuleGeometry(shape.radius, shape.height * 0.5f);
					}

					inline virtual void Update(const CapsuleShape& newShape) override {
						Shape()->setGeometry(Geometry(newShape));
						SetAlignment(newShape.alignment);
					}

					inline virtual Matrix4 GetLocalPose()const override { 
						return Translate(physx::PxMat44(Shape()->getLocalPose())) * m_wrangle.second;
					}

					inline virtual void SetLocalPose(const Matrix4& transform) override { 
						Shape()->setLocalPose(physx::PxTransform(Translate(transform * m_wrangle.first)));
					}
				};
#pragma warning(default: 4250)
			}

			PhysXBody::PhysXBody(PhysXScene* scene, physx::PxRigidActor* actor, bool enabled)
				: m_scene(scene), m_actor(actor), m_active(false) {
				if (m_actor == nullptr) {
					m_scene->APIInstance()->Log()->Fatal("PhysXBody::PhysXBody - null Actor pointer!");
					return;
				}
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
				else if (m_active) (*m_scene)->addActor(*m_actor);
				else (*m_scene)->removeActor(*m_actor);
			}

			Matrix4 PhysXBody::GetPose()const { return Translate(physx::PxMat44(m_actor->getGlobalPose())); }

			void PhysXBody::SetPose(const Matrix4& transform) { m_actor->setGlobalPose(physx::PxTransform(Translate(transform))); }

			Reference<PhysicsBoxCollider> PhysXBody::AddCollider(const BoxShape& box, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXCollider::Create<PhysXBoxCollider>(this, box, material, listener, enabled);
			}

			Reference<PhysicsSphereCollider> PhysXBody::AddCollider(const SphereShape& sphere, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXCollider::Create<PhysXSphereCollider>(this, sphere, material, listener, enabled);
			}

			Reference<PhysicsCapsuleCollider> PhysXBody::AddCollider(const CapsuleShape& capsule, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool enabled) {
				return PhysXCollider::Create<PhysXCapusuleCollider>(this, capsule, material, listener, enabled);
			}

			PhysXScene* PhysXBody::Scene()const { return m_scene; }

			PhysXBody::operator physx::PxRigidActor* ()const { return m_actor; }
		}
	}
}
