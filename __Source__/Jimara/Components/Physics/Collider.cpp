#include "Collider.h"


namespace Jimara {
	namespace {
		class ColliderEventListener : public virtual Physics::PhysicsCollider::EventListener {
		private:
			struct EventCache : public virtual ObjectCache<Reference<Scene::PhysicsContext>>::StoredObject {
				const Reference<Scene::PhysicsContext> context;

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

				inline void Synch() {
					std::unique_lock<std::mutex> lock(deadRefLock);
					deadRefBackBuffer ^= 1;
					deadRefs[deadRefBackBuffer].clear();
				}

				inline EventCache(Scene::PhysicsContext* ctx) : context(ctx) {
					context->OnPhysicsSynch() += Callback(&EventCache::Synch, this);
				}

				inline virtual ~EventCache() {
					context->OnPhysicsSynch() -= Callback(&EventCache::Synch, this);
				}
			};

			class Registry : public virtual ObjectCache<Reference<Scene::PhysicsContext>> {
			public:
				inline static Reference<EventCache> GetCache(Scene::PhysicsContext* context) {
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

			inline Collider* Owner()const { return m_owner; }

		protected:
			inline virtual void OnContact(const Physics::PhysicsCollider::ContactInfo& info) override {
				ColliderEventListener* otherListener = dynamic_cast<ColliderEventListener*>(info.OtherCollider()->Listener());
				if (m_cache == nullptr || otherListener == nullptr || m_owner == nullptr || otherListener->m_owner == nullptr) return;
				static thread_local std::vector<Physics::PhysicsBoxCollider::ContactPoint> contactPoints;
				contactPoints.resize(info.ContactPointCount());
				for (size_t i = 0; i < contactPoints.size(); i++) contactPoints[i] = info.ContactPoint(i);
				m_callback(Collider::ContactInfo(m_owner, otherListener->m_owner, info.EventType(), contactPoints.data(), contactPoints.size()));
			}
		};
	}

	Collider::Collider() 
		: m_listener(Object::Instantiate<ColliderEventListener>(this, Callback(&Collider::NotifyContact, this))) {}

	Collider::~Collider() {
		dynamic_cast<ColliderEventListener*>(m_listener.operator->())->OwnerDestroyed();
		OnComponentDestroyed();
	}

	bool Collider::IsTrigger()const { return m_isTrigger; }

	void Collider::SetTrigger(bool trigger) {
		if (m_isTrigger == trigger) return;
		m_isTrigger = trigger;
		ColliderDirty();
	}

	Collider::Layer Collider::GetLayer()const { return m_layer; }

	void Collider::SetLayer(Layer layer) {
		if (m_layer == layer) return;
		m_layer = layer;
		ColliderDirty();
	}

	Event<const Collider::ContactInfo&>& Collider::OnContact() { return m_onContact; }

	Collider* Collider::GetOwner(Physics::PhysicsCollider* collider) {
		if (collider == nullptr) return nullptr;
		ColliderEventListener* listener = dynamic_cast<ColliderEventListener*>(collider->Listener());
		if (listener == nullptr) return nullptr;
		else return listener->Owner();
	}

	void Collider::PrePhysicsSynch() {
		if (Destroyed()) return;

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
		if (Math::SqrMagnitude(m_lastScale - curScale) > std::numeric_limits<float>::epsilon() * 8.0f) {
			m_lastScale = curScale;
			m_dirty = true;
		}

		if ((m_dirty || (m_collider == nullptr)) && (m_body != nullptr)) {
			m_collider = GetPhysicsCollider(m_collider, m_body, m_lastScale, m_listener);
			if (m_collider != nullptr) {
				m_collider->SetTrigger(m_isTrigger);
				m_collider->SetLayer(m_layer);
				m_collider->SetActive(ActiveInHeirarchy());
			}
		}
		if ((m_collider != nullptr) && (m_rigidbody != nullptr) && (m_dirty || curPose != m_lastPose))
			m_collider->SetLocalPose(curPose);
		m_lastPose = curPose;
		m_dirty = false;
	}

	void Collider::OnComponentEnabled() {
		if (m_collider != nullptr)
			m_collider->SetActive(ActiveInHeirarchy());
	}

	void Collider::OnComponentDisabled() {
		if (m_collider != nullptr)
			m_collider->SetActive(ActiveInHeirarchy());
	}

	void Collider::OnComponentDestroyed() {
		dynamic_cast<ColliderEventListener*>(m_listener.operator->())->OwnerDead(m_collider);
		m_rigidbody = nullptr;
		m_body = nullptr;
		m_collider = nullptr;
	}

	void Collider::ColliderDirty() { m_dirty = true; }

	void Collider::NotifyContact(const ContactInfo& info) { m_onContact(info); }
}
