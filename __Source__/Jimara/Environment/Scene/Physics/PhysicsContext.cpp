#include "PhysicsContext.h"
#include "../../../Components/Physics/Collider.h"


namespace Jimara {
	Vector3 Scene::PhysicsContext::Gravity()const { return m_scene->Gravity(); }
	void Scene::PhysicsContext::SetGravity(const Vector3& value) { m_scene->SetGravity(value); }

	bool Scene::PhysicsContext::LayersInteract(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b)const { return m_scene->LayersInteract(a, b); }
	void Scene::PhysicsContext::FilterLayerInteraction(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b, bool enableIntaraction) {
		m_scene->FilterLayerInteraction(a, b, enableIntaraction);
	}

	Reference<Physics::DynamicBody> Scene::PhysicsContext::AddRigidBody(const Matrix4& transform, bool enabled) { return m_scene->AddRigidBody(transform, enabled); }
	Reference<Physics::StaticBody> Scene::PhysicsContext::AddStaticBody(const Matrix4& transform, bool enabled) { return m_scene->AddStaticBody(transform, enabled); }

	namespace {
		struct HitTranslator {
			Callback<const RaycastHit&> onHitFound = nullptr;
			size_t numFound = 0;

			const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter = nullptr;
			const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter = nullptr;

			inline static RaycastHit Translate(const Physics::RaycastHit& hit) {
				Collider* collider = Collider::GetOwner(hit.collider);
				RaycastHit ht;
				ht.collider = collider;
				ht.normal = hit.normal;
				ht.point = hit.point;
				ht.distance = hit.distance;
				return ht;
			}

			inline void OnHit(const Physics::RaycastHit& hit) {
				RaycastHit ht = Translate(hit);
				if (ht.collider == nullptr) return;
				else {
					onHitFound(ht);
					numFound++;
				}
			}

			inline Physics::PhysicsScene::QueryFilterFlag PreFilter(Physics::PhysicsCollider* collider) {
				Collider* component = Collider::GetOwner(collider);
				if (component == nullptr) return Physics::PhysicsScene::QueryFilterFlag::DISCARD;
				else return (*preFilter)(component);
			}

			inline Physics::PhysicsScene::QueryFilterFlag PostFilter(const Physics::RaycastHit& hit) {
				RaycastHit ht = Translate(hit);
				if (ht.collider == nullptr) return Physics::PhysicsScene::QueryFilterFlag::DISCARD;
				else return (*postFilter)(ht);
			}

			template<typename Query>
			inline static size_t Sweep(const Query& query
				, const Vector3& direction, float maxDistance
				, const Callback<const RaycastHit&>& onHitFound
				, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
				, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
				, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter) {

				HitTranslator translator;
				translator.onHitFound = onHitFound;
				translator.preFilter = preFilter;
				translator.postFilter = postFilter;
				const Callback<const Physics::RaycastHit&> onFound(&HitTranslator::OnHit, translator);
				const Function<Physics::PhysicsScene::QueryFilterFlag, Physics::PhysicsCollider*> preFilterCall(&HitTranslator::PreFilter, translator);
				const Function<Physics::PhysicsScene::QueryFilterFlag, const Physics::RaycastHit&> postFilterCall(&HitTranslator::PostFilter, translator);
				const size_t count = query(direction, maxDistance
					, onFound, layerMask, flags
					, preFilter == nullptr ? nullptr : &preFilterCall
					, postFilter == nullptr ? nullptr : &postFilterCall);

				// The second attempt below should not be necessary, but in case there are some random colliders floating around, this will take care of them:
				if (count != translator.numFound && translator.numFound == 0
					&& ((flags & Physics::PhysicsScene::Query(Physics::PhysicsScene::QueryFlag::REPORT_MULTIPLE_HITS)) == 0) && preFilter == nullptr && postFilter == nullptr) {
					translator.numFound = 0;
					Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> pre([](Collider*) { return Physics::PhysicsScene::QueryFilterFlag::REPORT; });
					translator.preFilter = &pre;
					query(direction, maxDistance, onFound, layerMask, flags, &preFilterCall);
				}

				return translator.numFound;
			}
		};
	}

	size_t Scene::PhysicsContext::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
		, const Callback<const RaycastHit&>& onHitFound
		, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
		, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
		, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const {
		return HitTranslator::Sweep(
			[&](const auto&... args) { return m_scene->Raycast(origin, args...); },
			direction, maxDistance, onHitFound, layerMask, flags, preFilter, postFilter);
	}

	size_t Scene::PhysicsContext::Sweep(const Physics::SphereShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
		, const Callback<const RaycastHit&>& onHitFound
		, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
		, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
		, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const {
		return HitTranslator::Sweep(
			[&](const auto&... args) { return m_scene->Sweep(shape, pose, args...); },
			direction, maxDistance, onHitFound, layerMask, flags, preFilter, postFilter);
	}

	size_t Scene::PhysicsContext::Sweep(const Physics::CapsuleShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
		, const Callback<const RaycastHit&>& onHitFound
		, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
		, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
		, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const {
		return HitTranslator::Sweep(
			[&](const auto&... args) { return m_scene->Sweep(shape, pose, args...); },
			direction, maxDistance, onHitFound, layerMask, flags, preFilter, postFilter);
	}

	size_t Scene::PhysicsContext::Sweep(const Physics::BoxShape& shape, const Matrix4& pose, const Vector3& direction, float maxDistance
		, const Callback<const RaycastHit&>& onHitFound
		, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
		, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
		, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const {
		return HitTranslator::Sweep(
			[&](const auto&... args) { return m_scene->Sweep(shape, pose, args...); },
			direction, maxDistance, onHitFound, layerMask, flags, preFilter, postFilter);
	}

	Physics::PhysicsInstance* Scene::PhysicsContext::APIInstance()const { return m_scene->APIInstance(); }

	float Scene::PhysicsContext::UpdateRate()const { return m_updateRate; }
	void Scene::PhysicsContext::SetUpdateRate(float rate) { m_updateRate = rate; }

	Event<>& Scene::PhysicsContext::OnPhysicsSynch() { return m_onPostPhysicsSynch; }


	void Scene::PhysicsContext::SynchIfReady(float deltaTime, float timeScale, LogicContext* context) {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		
		// Update timers:
		{
			float rate = UpdateRate();
			float minInterval = (rate > 0.0f ? (1.0f / rate) : 0.0f);
			m_elapsed = m_elapsed + deltaTime;
			m_scaledElapsed = m_scaledElapsed + (deltaTime * timeScale);
			if (m_elapsed.load() <= minInterval) return;
			m_time->Update(m_scaledElapsed.load());
			m_elapsed = m_scaledElapsed = 0.0f;
		}
		
		// Update PrePhysicsSynchUpdatingComponent-s:
		{
			const Reference<PrePhysicsSynchUpdatingComponent>* ptr = data->prePhysicsSynchUpdaters.Data();
			const Reference<PrePhysicsSynchUpdatingComponent>* const end = ptr + data->prePhysicsSynchUpdaters.Size();
			while (ptr < end) {
				PrePhysicsSynchUpdatingComponent* component = (*ptr);
				if (component->ActiveInHeirarchy())
					component->PrePhysicsSynch();
				ptr++;
			}
			context->FlushComponentSets();
		}

		// Synchronize simulation:
		{
			// __TODO__: Simulate with constant-duration substeps in a separate thread...
			m_scene->SynchSimulation();
			m_scene->SimulateAsynch(m_time->ScaledDeltaTime());
			m_onPostPhysicsSynch();
		}

		// Update PostPhysicsSynchUpdatingComponent-s:
		{
			const Reference<PostPhysicsSynchUpdatingComponent>* ptr = data->postPhysicsSynchUpdaters.Data();
			const Reference<PostPhysicsSynchUpdatingComponent>* const end = ptr + data->postPhysicsSynchUpdaters.Size();
			while (ptr < end) {
				PostPhysicsSynchUpdatingComponent* component = (*ptr);
				if (component->ActiveInHeirarchy())
					component->PostPhysicsSynch();
				ptr++;
			}
			context->FlushComponentSets();
		}
	}

	Scene::PhysicsContext::Data::Data(Physics::PhysicsScene* scene)
		: context([&]() -> Reference<PhysicsContext> {
		Reference<PhysicsContext> ctx = new PhysicsContext(scene);
		ctx->ReleaseRef();
		return ctx;
			}()) {
		context->m_data.data = this;
	}

	Reference<Scene::PhysicsContext::Data> Scene::PhysicsContext::Data::Create(CreateArgs& createArgs) {
		if (createArgs.physics.physicsInstance == nullptr) {
			if (createArgs.createMode == CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::PhysicsContext::Data::Create - Null physics instance provided! Creating a default instance...");
			else if (createArgs.createMode == CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::PhysicsContext::Data::Create - Null physics instance provided!");
				return nullptr;
			}
			createArgs.physics.physicsInstance = Physics::PhysicsInstance::Create(createArgs.logic.logger);
			if (createArgs.physics.physicsInstance == nullptr) {
				createArgs.logic.logger->Error("Scene::PhysicsContext::Data::Create - Failed to create a physics instance!");
				return nullptr;
			}
		}

		if (createArgs.physics.simulationThreadCount <= 0)
			createArgs.physics.simulationThreadCount = (std::thread::hardware_concurrency() / 4);
		if (createArgs.physics.simulationThreadCount <= 0)
			createArgs.physics.simulationThreadCount = 1;
		Reference<Physics::PhysicsScene> scene = createArgs.physics.physicsInstance->CreateScene(createArgs.physics.simulationThreadCount);
		if (scene == nullptr) {
			createArgs.logic.logger->Error("Scene::PhysicsContext::Data::Create - Failed to create a physics scene!");
			return nullptr;
		}

		return Object::Instantiate<Data>(scene);
	}

	void Scene::PhysicsContext::Data::OnOutOfScope()const {
		{
			std::unique_lock<SpinLock> lock(context->m_data.lock);
			if (RefCount() > 0) return;
			else {
				context->m_data.data = nullptr;
				context->m_scene->SynchSimulation();
			}
		}
		Object::OnOutOfScope();
	}

	void Scene::PhysicsContext::Data::ComponentEnabled(Component* component) {
		if (component == nullptr)
			return;
		{
			PrePhysicsSynchUpdatingComponent* updater = dynamic_cast<PrePhysicsSynchUpdatingComponent*>(component);
			if (updater != nullptr) prePhysicsSynchUpdaters.Add(updater);
		}
		{
			PostPhysicsSynchUpdatingComponent* updater = dynamic_cast<PostPhysicsSynchUpdatingComponent*>(component);
			if (updater != nullptr) postPhysicsSynchUpdaters.Add(updater);
		}
	}
	void Scene::PhysicsContext::Data::ComponentDisabled(Component* component) {
		if (component == nullptr)
			return;
		{
			PrePhysicsSynchUpdatingComponent* updater = dynamic_cast<PrePhysicsSynchUpdatingComponent*>(component);
			if (updater != nullptr) prePhysicsSynchUpdaters.Remove(updater);
		}
		{
			PostPhysicsSynchUpdatingComponent* updater = dynamic_cast<PostPhysicsSynchUpdatingComponent*>(component);
			if (updater != nullptr) postPhysicsSynchUpdaters.Remove(updater);
		}
	}
}
