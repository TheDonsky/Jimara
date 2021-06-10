#include "Collider.h"


namespace Jimara {
	namespace {
		class ColliderEventListener : public virtual Physics::PhysicsCollider::EventListener {
		public:
			struct ContactInfo {
				Callback<const Collider::ContactInfo&>* callback = nullptr;
				Collider* collider = nullptr;
				Collider* otherCollider = nullptr;
				Physics::PhysicsBoxCollider::ContactType type = Physics::PhysicsBoxCollider::ContactType::ON_COLLISION_BEGIN;
				size_t firstContactPoint = 0;
				size_t lastContactPoint = 0;
			};

		private:
			struct EventCache : public virtual ObjectCache<Reference<PhysicsContext>>::StoredObject {
				const Reference<PhysicsContext> context;

				struct DeadReference {
					Reference<Collider> collider;
					Reference<ColliderEventListener> listener;
					Reference<Physics::PhysicsCollider> physicsCollider;

					inline DeadReference(Collider* c = nullptr, ColliderEventListener* l = nullptr, Physics::PhysicsCollider* p = nullptr)
						: collider(c), listener(l), physicsCollider(p) {}
				};

				std::mutex deadRefLock;
				std::vector<DeadReference> deadRefs[2];
				uint8_t deadRefBackBuffer = 0;

				std::mutex contactLock;
				std::vector<Physics::PhysicsBoxCollider::ContactPoint> contactPoints;
				std::vector<ContactInfo> contacts;

				inline void Synch() {
					{
						std::unique_lock<std::mutex> lock(deadRefLock);
						deadRefBackBuffer ^= 1;
						deadRefs[deadRefBackBuffer].clear();
					}
					{
						std::unique_lock<std::mutex> loc(contactLock);
						for (size_t i = 0; i < contacts.size(); i++) {
							const ContactInfo& info = contacts[i];
							(*info.callback)(Collider::ContactInfo(
								info.collider, info.otherCollider, info.type, contactPoints.data() + info.firstContactPoint, info.lastContactPoint - info.firstContactPoint));
						}
						contacts.clear();
						contactPoints.clear();
					}
				}

				inline EventCache(PhysicsContext* ctx) : context(ctx) {
					context->OnPostPhysicsSynch() += Callback(&EventCache::Synch, this);
				}

				inline virtual ~EventCache() {
					context->OnPostPhysicsSynch() -= Callback(&EventCache::Synch, this);
				}
			};

			class Registry : public virtual ObjectCache<Reference<PhysicsContext>> {
			public:
				inline static Reference<EventCache> GetCache(PhysicsContext* context) {
					static Registry registry;
					return registry.GetCachedOrCreate(context, false, [&]()->Reference<EventCache> { return Object::Instantiate<EventCache>(context); });
				}
			};
			
			Reference<EventCache> m_cache;
			Collider* m_owner;
			Callback<const Collider::ContactInfo&> m_callback;

		public:
			inline ColliderEventListener(Collider* owner, const Callback<const Collider::ContactInfo&>& callback)
				: m_cache(Registry::GetCache(owner->Context()->Physics())), m_owner(owner), m_callback(callback) {}

			inline void OwnerDead(Physics::PhysicsCollider* collider) {
				if (m_owner == nullptr || collider == nullptr) return;
				collider->SetActive(false);
				if (m_cache == nullptr) return;
				{
					std::unique_lock<std::mutex> lock(m_cache->deadRefLock);
					m_cache->deadRefs[m_cache->deadRefBackBuffer].push_back(EventCache::DeadReference(m_owner, this, collider));
				}
				m_cache = nullptr;
			}

			inline void OwnerDestroyed() { 
				m_cache = nullptr;
				m_owner = nullptr; 
			}

		protected:
			inline virtual void OnContact(const Physics::PhysicsCollider::ContactInfo& info) override {
				ColliderEventListener* otherListener = dynamic_cast<ColliderEventListener*>(info.OtherCollider()->Listener());
				if (m_cache == nullptr || otherListener == nullptr || m_owner == nullptr || otherListener->m_owner == nullptr) return;
				std::unique_lock<std::mutex> lock(m_cache->contactLock);
				ContactInfo contact;
				contact.callback = &m_callback;
				contact.collider = m_owner;
				contact.otherCollider = otherListener->m_owner;
				contact.type = info.EventType();
				contact.firstContactPoint = m_cache->contactPoints.size();
				for (size_t i = 0; i < info.ContactPointCount(); i++)
					m_cache->contactPoints.push_back(info.ContactPoint(i));
				contact.lastContactPoint = m_cache->contactPoints.size();
				m_cache->contacts.push_back(contact);
			}
		};
	}

	Collider::Collider() : m_listener(Object::Instantiate<ColliderEventListener>(this, Callback(&Collider::NotifyContact, this))) {
		OnDestroyed() += Callback(&Collider::ClearWhenDestroyed, this);
	}

	Collider::~Collider() {
		OnDestroyed() -= Callback(&Collider::ClearWhenDestroyed, this);
		dynamic_cast<ColliderEventListener*>(m_listener.operator->())->OwnerDestroyed();
		ClearWhenDestroyed(this);
	}

	bool Collider::IsTrigger()const { return m_isTrigger; }

	void Collider::SetTrigger(bool trigger) {
		if (m_isTrigger == trigger) return;
		m_isTrigger = trigger;
		ColliderDirty();
	}

	Event<const Collider::ContactInfo&>& Collider::OnContact() { return m_onContact; }

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

		if ((m_dirty || (m_collider == nullptr)) && (m_body != nullptr)) {
			m_collider = GetPhysicsCollider(m_collider, m_body, m_lastScale, m_listener);
			if (m_collider != nullptr)
				m_collider->SetTrigger(m_isTrigger);
		}
		if ((m_collider != nullptr) && (m_rigidbody != nullptr) && (m_dirty || curPose != m_lastPose))
			m_collider->SetLocalPose(curPose);
		m_lastPose = curPose;
		m_dirty = false;
	}

	void Collider::ColliderDirty() { m_dirty = true; }

	void Collider::ClearWhenDestroyed(Component*) {
		if (!m_dead) dynamic_cast<ColliderEventListener*>(m_listener.operator->())->OwnerDead(m_collider);
		m_dead = true;
		m_rigidbody = nullptr;
		m_body = nullptr;
		m_collider = nullptr;
	}

	void Collider::NotifyContact(const ContactInfo& info) { m_onContact(info); }
}
