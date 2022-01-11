#include "PhysicsContext.h"
#include "../../../Components/Physics/Collider.h"


namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
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
		};
	}

	size_t Scene::PhysicsContext::Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
		, const Callback<const RaycastHit&>& onHitFound
		, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
		, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
		, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const {

		HitTranslator translator;
		translator.onHitFound = onHitFound;
		translator.preFilter = preFilter;
		translator.postFilter = postFilter;
		const Callback<const Physics::RaycastHit&> onFound(&HitTranslator::OnHit, translator);
		const Function<Physics::PhysicsScene::QueryFilterFlag, Physics::PhysicsCollider*> preFilterCall(&HitTranslator::PreFilter, translator);
		const Function<Physics::PhysicsScene::QueryFilterFlag, const Physics::RaycastHit&> postFilterCall(&HitTranslator::PostFilter, translator);
		const size_t count = m_scene->Raycast(origin, direction, maxDistance
			, onFound, layerMask, flags
			, preFilter == nullptr ? nullptr : &preFilterCall
			, postFilter == nullptr ? nullptr : &postFilterCall);

		// The second attempt below should not be necessary, but in case there are some random colliders floating around, this will take care of them:
		if (count != translator.numFound && translator.numFound == 0
			&& ((flags & Physics::PhysicsScene::Query(Physics::PhysicsScene::QueryFlag::REPORT_MULTIPLE_HITS)) == 0) && preFilter == nullptr && postFilter == nullptr) {
			translator.numFound = 0;
			Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> pre([](Collider*) { return Physics::PhysicsScene::QueryFilterFlag::REPORT; });
			translator.preFilter = &pre;
			m_scene->Raycast(origin, direction, maxDistance, onFound, layerMask, flags, &preFilterCall);
		}

		return translator.numFound;
	}

	Physics::PhysicsInstance* Scene::PhysicsContext::APIInstance()const { return m_scene->APIInstance(); }

	float Scene::PhysicsContext::UpdateRate()const { return m_updateRate; }
	void Scene::PhysicsContext::SetUpdateRate(float rate) { m_updateRate = rate; }

	Event<>& Scene::PhysicsContext::OnPhysicsSynch() { return m_onPostPhysicsSynch; }


	void Scene::PhysicsContext::SynchIfReady(float deltaTime, float timeScale) {
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
				(*ptr)->PrePhysicsSynch();
				ptr++;
			}
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
				(*ptr)->PostPhysicsSynch();
				ptr++;
			}
		}
	}

	Scene::PhysicsContext::Data::Data(Physics::PhysicsInstance* instance, OS::Logger* logger)
		: context([&]() -> Reference<PhysicsContext> {
		Reference<Physics::PhysicsInstance> physicsInstance = instance;
		if (physicsInstance == nullptr) {
			logger->Warning("Scene::PhysicsContext::Data::Data - Null physics instance provided! Creating a default instance...");
			physicsInstance = Physics::PhysicsInstance::Create(logger);
		}
		Reference<PhysicsContext> ctx = new PhysicsContext(physicsInstance);
		ctx->ReleaseRef();
		return ctx;
			}()) {
		context->m_data.data = this;
	}

	void Scene::PhysicsContext::Data::OnOutOfScope()const {
		std::unique_lock<SpinLock> lock(context->m_data.lock);
		if (RefCount() > 0) return;
		else {
			context->m_data.data = nullptr;
			context->m_scene->SynchSimulation();
			Object::OnOutOfScope();
		}
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
#ifndef USE_REFACTORED_SCENE
}
#endif
}
