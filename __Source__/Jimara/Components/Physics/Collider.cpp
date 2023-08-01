#include "Collider.h"
#include "../../Environment/LogicSimulation/SimulationThreadBlock.h"


namespace Jimara {
	struct Collider::Helpers {
		struct ColliderComponentState {
			Matrix4 transformation = {};
			Matrix4 rotation = {};
			Matrix4 curPose = {};
			Vector3 curScale = {};
		};

		inline static ColliderComponentState UpdateComponentState(Collider* self) {
			ColliderComponentState state;
			Rigidbody* rigidbody = self->GetComponentInParents<Rigidbody>();
			if (self->m_rigidbody != rigidbody) {
				self->m_rigidbody = rigidbody;
				self->m_body = nullptr;
				self->m_collider = nullptr;
				self->m_dirty = true;
			}

			Transform* transform = self->GetTransfrom();
			if (transform != nullptr) {
				state.transformation = transform->WorldMatrix();
				state.rotation = transform->WorldRotationMatrix();
			}
			else state.transformation = state.rotation = Math::Identity();

			state.curScale = Math::LossyScale(state.transformation, state.rotation);
			auto setPose = [&](const Matrix4& trans, const Matrix4& rot) {
				state.curPose = rot;
				state.curPose[3] = trans[3];
			};
			
			if (rigidbody != nullptr) {
				Transform* rigidTransform = rigidbody->GetTransfrom();
				if (rigidTransform != nullptr && transform != nullptr) {
					Matrix4 relativeTransformation = Math::Identity();
					Matrix4 relativeRotation = Math::Identity();
					for (Transform* trans = transform; trans != rigidTransform; trans = trans->GetComponentInParents<Transform>(false)) {
						relativeTransformation = trans->LocalMatrix() * relativeTransformation;
						relativeRotation = trans->LocalRotationMatrix() * relativeRotation;
					}
					setPose(relativeTransformation, relativeRotation);
				}
				else setPose(state.transformation, state.rotation);
			}
			else setPose(state.transformation, state.rotation);

			return state;
		}

		inline static void UpdatePhysicsState(Collider* self, const ColliderComponentState& state) {
			if (self->m_body == nullptr) {
				if (self->m_rigidbody != nullptr)
					self->m_body = self->m_rigidbody->GetBody();
				else {
					self->m_body = self->Context()->Physics()->AddStaticBody(state.curPose);
					self->m_collider = nullptr;
				}
				self->m_dirty = true;
			}
			else if (self->m_rigidbody == nullptr && self->m_lastPose != state.curPose)
				self->m_body->SetPose(state.curPose);

			if (Math::SqrMagnitude(self->m_lastScale - state.curScale) > std::numeric_limits<float>::epsilon() * 8.0f) {
				self->m_lastScale = state.curScale;
				self->m_dirty = true;
			}

			if ((self->m_dirty || (self->m_collider == nullptr)) && (self->m_body != nullptr)) {
				self->m_collider = self->GetPhysicsCollider(self->m_collider, self->m_body, self->m_lastScale, self->m_listener);
				if (self->m_collider != nullptr) {
					self->m_collider->SetTrigger(self->m_isTrigger);
					self->m_collider->SetLayer(self->m_layer);
					self->m_collider->SetActive(self->ActiveInHeirarchy());
				}
			}

			if ((self->m_collider != nullptr) && (self->m_rigidbody != nullptr) && (self->m_dirty || state.curPose != self->m_lastPose))
				self->m_collider->SetLocalPose(state.curPose);
			self->m_lastPose = state.curPose;
			self->m_dirty = false;
		}

		class ColliderSynchJob : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<SimulationThreadBlock> m_threadBlock;

			std::recursive_mutex m_lock;
			std::set<Reference<Collider>> m_colliderSet;
			struct ActiveCollider {
				Collider* collider = nullptr;
				ColliderComponentState state = {};
			};
			std::vector<ActiveCollider> m_activeColliders;
			bool m_collidersDirty = false;

		public:
			inline ColliderSynchJob(SceneContext* context)
				: m_context(context)
				, m_threadBlock(SimulationThreadBlock::GetFor(context)) {}

			inline virtual ~ColliderSynchJob() {}

			struct Cache : public virtual ObjectCache<Reference<const Object>> {
				static Reference<ColliderSynchJob> Get(SceneContext* context) {
					static Cache cache;
					static std::mutex creationLock;
					std::unique_lock<std::mutex> lock(creationLock);
					return cache.GetCachedOrCreate(context, false, [&]() {
						const Reference<ColliderSynchJob> instance = Object::Instantiate<ColliderSynchJob>(context);
						context->StoreDataObject(instance);
						context->Physics()->OnPhysicsSynch();
						return instance;
						});
				}
			};

			inline void AddCollider(Collider* collider) {
				assert(collider != nullptr);
				std::unique_lock<std::recursive_mutex> lock(m_lock);
				m_colliderSet.insert(collider);
				m_collidersDirty = true;
			}

			inline void RemoveCollider(Collider* collider) {
				assert(collider != nullptr);
				std::unique_lock<std::recursive_mutex> lock(m_lock);
				m_colliderSet.erase(collider);
				m_collidersDirty = true;
			}

			inline void Synch() {
				std::unique_lock<std::recursive_mutex> lock(m_lock);
				auto fixColliderList = [&]() {
					if (!m_collidersDirty)
						return;
					m_activeColliders.clear();
					for (auto it = m_colliderSet.begin(); it != m_colliderSet.end(); ++it) {
						ActiveCollider collider = {};
						collider.collider = *it;
						m_activeColliders.push_back(collider);
					}
					m_collidersDirty = false;
				};
				fixColliderList();

				typedef void(*UpdateColliderStateFn)(ThreadBlock::ThreadInfo, void*);
				static const UpdateColliderStateFn updateColliderStates = [](ThreadBlock::ThreadInfo info, void* selfPtr) {
					ColliderSynchJob* const self = (ColliderSynchJob*)selfPtr;
					const size_t totalColliderCount = self->m_activeColliders.size();
					const size_t entriesPerThread = (self->m_activeColliders.size() + info.threadCount - 1u) / info.threadCount;
					ActiveCollider* const firstCollider = self->m_activeColliders.data() + (entriesPerThread * info.threadId);
					ActiveCollider* const lastCollider = self->m_activeColliders.data() +
						Math::Min(entriesPerThread * (info.threadId + 1u), totalColliderCount);
					for (ActiveCollider* ptr = firstCollider; ptr < lastCollider; ptr++) {
						assert(!ptr->collider->Destroyed());
						ptr->state = UpdateComponentState(ptr->collider);
					}
				};

				static const constexpr size_t minCollidersPerThread = 32u;
				const size_t threadCount = Math::Min(
					(m_activeColliders.size() + minCollidersPerThread - 1u) / minCollidersPerThread, m_threadBlock->DefaultThreadCount());
				if (threadCount <= 1u) {
					ThreadBlock::ThreadInfo info = {};
					info.threadCount = 1u;
					info.threadId = 0u;
					updateColliderStates(info, (void*)this);
				}
				else m_threadBlock->Execute(threadCount, (void*)this, Callback<ThreadBlock::ThreadInfo, void*>(updateColliderStates));

				{
					const ActiveCollider* const end = m_activeColliders.data() + m_activeColliders.size();
					for (const ActiveCollider* ptr = m_activeColliders.data(); ptr < end; ptr++) {
						assert(!ptr->collider->Destroyed());
						UpdatePhysicsState(ptr->collider, ptr->state);
					}
				}

				fixColliderList();
			}
		};

		class ColliderEventListener : public virtual Physics::PhysicsCollider::EventListener {
		private:
			struct EventCache : public virtual ObjectCache<Reference<Scene::PhysicsContext>>::StoredObject {
				const Reference<Scene::PhysicsContext> context;
				const Reference<ColliderSynchJob> synchJob;

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
					synchJob->Synch();
					std::unique_lock<std::mutex> lock(deadRefLock);
					deadRefBackBuffer ^= 1;
					deadRefs[deadRefBackBuffer].clear();
				}

				inline EventCache(Scene::PhysicsContext* ctx, ColliderSynchJob* synch) : context(ctx), synchJob(synch) {
					context->OnPhysicsSynch() += Callback(&EventCache::Synch, this);
				}

				inline virtual ~EventCache() {
					context->OnPhysicsSynch() -= Callback(&EventCache::Synch, this);
				}
			};

			class Registry : public virtual ObjectCache<Reference<Scene::PhysicsContext>> {
			public:
				inline static Reference<EventCache> GetCache(Scene::PhysicsContext* context, ColliderSynchJob* synchJob) {
					static Registry registry;
					return registry.GetCachedOrCreate(context, false, [&]()->Reference<EventCache> {
						return Object::Instantiate<EventCache>(context, synchJob);
						});
				}
			};

			Reference<EventCache> m_cache;
			Collider* m_owner;
			Callback<const Collider::ContactInfo&> m_callback;

		public:
			inline ColliderEventListener(Collider* owner, const Callback<const Collider::ContactInfo&>& callback)
				: m_cache(Registry::GetCache(owner->Context()->Physics(), ColliderSynchJob::Cache::Get(owner->Context())))
				, m_owner(owner), m_callback(callback) {}

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

			inline ColliderSynchJob* SynchJob()const { return m_cache->synchJob; }

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

		inline static void OnEnabledOrDisabled(Collider* self) {
			const bool active = self->ActiveInHeirarchy();
			ColliderSynchJob* synchJob = dynamic_cast<Helpers::ColliderEventListener*>(self->m_listener.operator->())->SynchJob();
			if (active)
				synchJob->AddCollider(self);
			else synchJob->RemoveCollider(self);
			if (self->m_collider != nullptr)
				self->m_collider->SetActive(active);
		}
	};

	Collider::Collider() 
		: m_listener(Object::Instantiate<Helpers::ColliderEventListener>(this, Callback(&Collider::NotifyContact, this))) {}

	Collider::~Collider() {
		dynamic_cast<Helpers::ColliderEventListener*>(m_listener.operator->())->OwnerDestroyed();
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
		Helpers::ColliderEventListener* listener = dynamic_cast<Helpers::ColliderEventListener*>(collider->Listener());
		if (listener == nullptr) return nullptr;
		else return listener->Owner();
	}

	void Collider::OnComponentInitialized() {
		SynchPhysicsCollider();
	}

	void Collider::OnComponentEnabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void Collider::OnComponentDisabled() {
		Helpers::OnEnabledOrDisabled(this);
	}

	void Collider::OnComponentDestroyed() {
		dynamic_cast<Helpers::ColliderEventListener*>(m_listener.operator->())->OwnerDead(m_collider);
		m_rigidbody = nullptr;
		m_body = nullptr;
		m_collider = nullptr;
	}

	void Collider::ColliderDirty() { m_dirty = true; }

	void Collider::SynchPhysicsCollider() {
		if (Destroyed()) return;
		const Helpers::ColliderComponentState state = Helpers::UpdateComponentState(this);
		Helpers::UpdatePhysicsState(this, state);
	}

	void Collider::NotifyContact(const ContactInfo& info) { m_onContact(info); }
}
