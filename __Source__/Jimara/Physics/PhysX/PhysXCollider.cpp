#include "PhysXCollider.h"


namespace Jimara {
	namespace Physics {
		namespace PhysX {
			PhysXCollider::~PhysXCollider() { Destroyed(); }

			bool PhysXCollider::Active()const { return m_active; }

			void PhysXCollider::SetActive(bool active) {
				if (m_active == active) return;
				m_active = active;
				if (m_shape == nullptr) return;
				else if (m_active) ((physx::PxRigidActor*)(*m_body))->attachShape(*m_shape);
				else ((physx::PxRigidActor*)(*m_body))->detachShape(*m_shape);
			}

			Matrix4 PhysXCollider::GetLocalPose()const { return Translate(physx::PxMat44(m_shape->getLocalPose())); }

			void PhysXCollider::SetLocalPose(const Matrix4& transform) { m_shape->setLocalPose(physx::PxTransform(Translate(transform))); }

			bool PhysXCollider::IsTrigger()const { return (GetFilterFlags(m_filterData) & static_cast<FilterFlags>(FilterFlag::IS_TRIGGER)) != 0; }

			void PhysXCollider::SetTrigger(bool trigger) {
				if (trigger) m_filterData.word3 |= static_cast<FilterFlags>(FilterFlag::IS_TRIGGER);
				else m_filterData.word3 &= ~static_cast<FilterFlags>(FilterFlag::IS_TRIGGER);
				m_shape->setSimulationFilterData(m_filterData);
			}

			PhysicsCollider::Layer PhysXCollider::GetLayer()const {
				return GetLayer(m_filterData);
			}

			void PhysXCollider::SetLayer(Layer layer) {
				m_filterData.word0 = layer;
				m_shape->setSimulationFilterData(m_filterData);
			}

			PhysXBody* PhysXCollider::Body()const { return m_body; }

			physx::PxShape* PhysXCollider::Shape()const { return m_shape; }

			namespace {
				class ContactInformation : public virtual PhysicsCollider::ContactInfo {
				private:
					PhysicsCollider* const m_collider;
					PhysicsCollider* const m_otherCollider;
					const PhysicsCollider::ContactType m_type;
					const PhysicsCollider::ContactPoint* const m_points;
					const size_t m_pointCount;

				public:
					inline ContactInformation(
						PhysicsCollider* self, PhysicsCollider* other, PhysicsCollider::ContactType type, const PhysicsCollider::ContactPoint* points, size_t pointCount)
						: m_collider(self), m_otherCollider(other), m_type(type), m_points(points), m_pointCount(pointCount) {}

					inline virtual PhysicsCollider* Collider()const override { return m_collider; }
					inline virtual PhysicsCollider* OtherCollider()const override { return m_otherCollider; }
					inline virtual PhysicsCollider::ContactType EventType()const override { return m_type; }
					inline virtual size_t ContactPointCount()const override { return m_pointCount; }
					inline virtual PhysicsCollider::ContactPoint ContactPoint(size_t index)const override { return m_points[index]; }
				};
			}

			void PhysXCollider::UserData::OnContact(
				physx::PxShape* shape, physx::PxShape* otherShape, PhysicsCollider::ContactType type
				, const PhysicsCollider::ContactPoint* points, size_t pointCount) {
				if (m_owner == nullptr) return;
				if (m_owner->m_shape != shape || otherShape == nullptr) return;
				UserData* listener = (UserData*)otherShape->userData;
				if (listener == nullptr) return;
				PhysXCollider* otherCollider = listener->m_owner;
				if (otherCollider == nullptr) return;
				m_owner->NotifyContact(ContactInformation(m_owner, otherCollider, type, points, pointCount));
			}

			PhysicsCollider* PhysXCollider::UserData::Collider()const { return m_owner; }

			PhysXCollider::PhysXCollider(PhysXBody* body, physx::PxShape* shape, PhysicsCollider::EventListener* listener, bool active)
				: PhysicsCollider(listener), m_body(body), m_shape(shape) {
				if (m_shape == nullptr) {
					m_body->Scene()->APIInstance()->Log()->Fatal("PhysXCollider - null Shape!");
					return;
				}
				m_userData.m_owner = this;
				m_shape->userData = &m_userData;
				m_shape->release();
				m_shape->setSimulationFilterData(m_filterData);
				SetActive(active);
			}

			void PhysXCollider::Destroyed() {
				m_shape->userData = nullptr;
				SetActive(false);
			}


			namespace {
				inline static Reference<PhysXMaterial> GetMaterial(PhysicsMaterial* material, PhysXInstance* instance) {
					Reference<PhysXMaterial> materialToUse = dynamic_cast<PhysXMaterial*>(material);
					if (materialToUse == nullptr) {
						materialToUse = PhysXMaterial::Default(instance);
						if (materialToUse == nullptr) {
							instance->Log()->Error("PhysXCollider::GetMaterial - Failed get default material!");
							return nullptr;
						}
					}
					return materialToUse;
				}
			}


			PhysicsMaterial* SingleMaterialPhysXCollider::Material()const { return m_material; }

			void SingleMaterialPhysXCollider::SetMaterial(PhysicsMaterial* material) {
				Reference<PhysXMaterial> materialToSet = GetMaterial(material, dynamic_cast<PhysXInstance*>(Body()->Scene()->APIInstance()));
				if (materialToSet == nullptr) return;
				if (materialToSet == m_material) return;
				physx::PxMaterial* apiMaterial = (*materialToSet);
				Shape()->setMaterials(&apiMaterial, 1);
				m_material = materialToSet;
			}

			SingleMaterialPhysXCollider::SingleMaterialPhysXCollider(
				PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active) 
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), m_material(material) {}




			Reference<PhysXBoxCollider> PhysXBoxCollider::Create(
				PhysXBody* body, const BoxShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active) {
				PhysXInstance* instance = dynamic_cast<PhysXInstance*>(body->Scene()->APIInstance());
				Reference<PhysXMaterial> apiMaterial = GetMaterial(material, instance);
				if (apiMaterial == nullptr) return nullptr;
				physx::PxShape* shape = (*instance)->createShape(Geometry(geometry), *(*apiMaterial), true);
				if (shape == nullptr) {
					instance->Log()->Error("PhysXBoxCollider::Create - Failed to create shape!");
					return nullptr;
				}
				Reference<PhysXBoxCollider> collider = new PhysXBoxCollider(body, shape, apiMaterial, listener, active);
				collider->ReleaseRef();
				return collider;
			}

			PhysXBoxCollider::~PhysXBoxCollider() { Destroyed(); }

			void PhysXBoxCollider::Update(const BoxShape& newShape) {
				Shape()->setGeometry(Geometry(newShape));
			}

			PhysXBoxCollider::PhysXBoxCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active)
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), SingleMaterialPhysXCollider(body, shape, material, listener, active) {}

			physx::PxBoxGeometry PhysXBoxCollider::Geometry(const BoxShape& shape) {
				return physx::PxBoxGeometry(shape.size.x * 0.5f, shape.size.y * 0.5f, shape.size.z * 0.5f);
			}


			Reference<PhysXSphereCollider> PhysXSphereCollider::Create(
				PhysXBody* body, const SphereShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active) {
				PhysXInstance* instance = dynamic_cast<PhysXInstance*>(body->Scene()->APIInstance());
				Reference<PhysXMaterial> apiMaterial = GetMaterial(material, instance);
				if (apiMaterial == nullptr) return nullptr;
				physx::PxShape* shape = (*instance)->createShape(Geometry(geometry), *(*apiMaterial), true);
				if (shape == nullptr) {
					instance->Log()->Error("PhysXSphereCollider::Create - Failed to create shape!");
					return nullptr;
				}
				Reference<PhysXSphereCollider> collider = new PhysXSphereCollider(body, shape, apiMaterial, listener, active);
				collider->ReleaseRef();
				return collider;
			}

			PhysXSphereCollider::~PhysXSphereCollider() { Destroyed(); }

			void PhysXSphereCollider::Update(const SphereShape& newShape) {
				Shape()->setGeometry(Geometry(newShape));
			}

			PhysXSphereCollider::PhysXSphereCollider(PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active)
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), SingleMaterialPhysXCollider(body, shape, material, listener, active) {}

			physx::PxSphereGeometry PhysXSphereCollider::Geometry(const SphereShape& shape) {
				return physx::PxSphereGeometry(shape.radius);
			}



			Reference<PhysXCapusuleCollider> PhysXCapusuleCollider::Create(
				PhysXBody* body, const CapsuleShape& geometry, PhysicsMaterial* material, PhysicsCollider::EventListener* listener, bool active) {
				PhysXInstance* instance = dynamic_cast<PhysXInstance*>(body->Scene()->APIInstance());
				Reference<PhysXMaterial> apiMaterial = GetMaterial(material, instance);
				if (apiMaterial == nullptr) return nullptr;
				physx::PxShape* shape = (*instance)->createShape(Geometry(geometry), *(*apiMaterial), true);
				if (shape == nullptr) {
					instance->Log()->Error("PhysXCapusuleCollider::Create - Failed to create shape!");
					return nullptr;
				}
				Reference<PhysXCapusuleCollider> collider = new PhysXCapusuleCollider(body, shape, apiMaterial, listener, active, geometry);
				collider->ReleaseRef();
				return collider;
			}

			PhysXCapusuleCollider::~PhysXCapusuleCollider() { Destroyed(); }

			void PhysXCapusuleCollider::Update(const CapsuleShape& newShape) {
				Shape()->setGeometry(Geometry(newShape));
				SetAlignment(newShape.alignment);
			}

			Matrix4 PhysXCapusuleCollider::GetLocalPose()const {
				return Translate(physx::PxMat44(Shape()->getLocalPose())) * m_wrangle.second;
			}

			void PhysXCapusuleCollider::SetLocalPose(const Matrix4& transform) {
				Shape()->setLocalPose(physx::PxTransform(Translate(transform * m_wrangle.first)));
			}

			PhysXCapusuleCollider::PhysXCapusuleCollider(
				PhysXBody* body, physx::PxShape* shape, PhysXMaterial* material, PhysicsCollider::EventListener* listener, bool active, const CapsuleShape& capsule)
				: PhysicsCollider(listener), PhysXCollider(body, shape, listener, active), SingleMaterialPhysXCollider(body, shape, material, listener, active) {
				SetAlignment(capsule.alignment);
			}

			physx::PxCapsuleGeometry PhysXCapusuleCollider::Geometry(const CapsuleShape& shape) {
				return physx::PxCapsuleGeometry(shape.radius, shape.height * 0.5f);
			}

			void PhysXCapusuleCollider::SetAlignment(CapsuleShape::Alignment alignment) {
				Wrangler newFn = Wrangle(alignment);
				Matrix4 raw = Translate(Shape()->getLocalPose());
				Matrix4 unwrangled = raw * m_wrangle.second;
				Matrix4 rewrangled = unwrangled * newFn.first;
				Shape()->setLocalPose(physx::PxTransform(Translate(rewrangled)));
				m_wrangle = newFn;
			}

			PhysXCapusuleCollider::Wrangler PhysXCapusuleCollider::Wrangle(CapsuleShape::Alignment alignment) {
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
		}
	}
}
