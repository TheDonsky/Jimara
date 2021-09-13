#include "Scene.h"
#include "../Core/Stopwatch.h"
#include "../Graphics/Data/GraphicsPipelineSet.h"
#include "../Components/Interfaces/Updatable.h"
#include "../Components/Interfaces/PhysicsUpdaters.h"
#include "../Components/Physics/Collider.h"
#include <mutex>


namespace Jimara {
	namespace {
		template<typename Type>
		struct AddRemoveEvents {
			EventInstance<const Reference<Type>*, size_t> onAdded;
			EventInstance<const Reference<Type>*, size_t> onRemoved;
		};

		template<typename ObjectType>
		class DelayedObjectSet {
		private:
			ObjectSet<ObjectType> m_added;
			ObjectSet<ObjectType> m_removed;
			ObjectSet<ObjectType> m_active;

		public:
			inline void Add(Object* object) {
				ObjectType* instance = dynamic_cast<ObjectType*>(object);
				if (instance == nullptr) return;
				m_added.Add(instance);
				m_removed.Remove(instance);
			}

			inline void Remove(Object* object) {
				ObjectType* instance = dynamic_cast<ObjectType*>(object);
				if (instance == nullptr) return;
				m_added.Remove(instance);
				m_removed.Add(instance);
			}

			template<typename OnRemovedCallback, typename OnAddedCallback>
			inline void Flush(OnRemovedCallback onRemoved, OnAddedCallback onAdded) {
				if (m_removed.Size() > 0) {
					m_active.Remove(m_removed.Data(), m_removed.Size(), onRemoved);
					m_removed.Clear();
				}
				if (m_added.Size() > 0) {
					m_active.Add(m_added.Data(), m_added.Size(), onAdded);
					m_added.Clear();
				}
			}

			inline const Reference<ObjectType>* Data()const { return m_active.Data(); }

			inline size_t Size()const { return m_active.Size(); }

			inline void Clear() { m_active.Clear(); }

			inline void ClearAll() {
				Clear();
				m_added.Clear();
				m_removed.Clear();
			}
		};




		class SceneGraphicsContext : public virtual GraphicsContext {
		private:
			AddRemoveEvents<GraphicsObjectDescriptor> m_onSceneObjectSetChanged;
			AddRemoveEvents<LightDescriptor> m_onSceneLightSetChanged;

			std::mutex m_pendingPipelineLock;

			class SceneGraphicsData : public virtual Object {
			private:
				const Reference<SceneGraphicsContext> m_context;

			public:
				ObjectSet<GraphicsContext::GraphicsObjectSynchronizer> synchronizers;

			private:
				inline void AddCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Add(synchronizer);
				}

				inline void RemoveCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Remove(synchronizer);
				}

			public:

				template<typename Type>
				class ObjectCollection : public virtual DelayedObjectSet<Type> {
				private:
					SceneGraphicsData* const m_owner;
					AddRemoveEvents<Type>* m_events;

				public:
					inline ObjectCollection(SceneGraphicsData* owner, AddRemoveEvents<Type>* events) : m_owner(owner), m_events(events) {}

					inline void Update() {
						DelayedObjectSet<Type>::Flush(
							[&](const Reference<Type>* removed, size_t count) {
								for (size_t i = 0; i < count; i++) m_owner->RemoveCallbacks(removed[i]);
								m_events->onRemoved(removed, count);
							}, [&](const Reference<Type>* added, size_t count) {
								for (size_t i = 0; i < count; i++) m_owner->AddCallbacks(added[i]);
								m_events->onAdded(added, count);
							});
					}
				};

				ObjectCollection<GraphicsObjectDescriptor> sceneObjects;
				ObjectCollection<LightDescriptor> sceneLights;

				inline SceneGraphicsData(SceneGraphicsContext* context) 
					: m_context(context)
					, sceneObjects(this, &context->m_onSceneObjectSetChanged)
					, sceneLights(this, &context->m_onSceneLightSetChanged) { }

				inline ~SceneGraphicsData() { 
					std::unique_lock<std::mutex> lock(m_context->m_pendingPipelineLock);
					m_context->m_data = nullptr; 
				}
			};

			std::atomic<SceneGraphicsData*> m_data;

			EventInstance<> m_onPostGraphicsSynch;

			Reference<LightingModel> m_defaultLightingModel;

			const size_t m_synchThreadCount;
			ThreadBlock m_synchBlock;

			const std::unordered_map<std::string, uint32_t> m_lightTypeIds;
			const size_t m_perLightDataSize;

			template<typename Type, typename GetSet>
			inline void Add(const GetSet& getSet, Type* object) {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				Reference<SceneGraphicsData> data = m_data.load();
				if (data == nullptr) return;
				getSet(data)->Add(object);
			}

			template<typename Type, typename GetSet>
			inline void Remove(const GetSet& getSet, Type* object) {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				Reference<SceneGraphicsData> data = m_data.load();
				if (data == nullptr) return;
				getSet(data)->Remove(object);
			}

			template<typename Type, typename GetSet>
			inline void GetValues(const GetSet& getSet, const Reference<Type>*& values, size_t& count) {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				Reference<SceneGraphicsData> data = m_data.load();
				if (data == nullptr) {
					values = nullptr;
					count = 0;
				}
				else {
					auto set = getSet(data);
					values = set->Data();
					count = set->Size();
				}
			}


		public:
			inline SceneGraphicsContext(
				AppContext* context, ShaderLoader* shaderLoader, const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize, LightingModel* defaultLightingModel)
				: GraphicsContext(context->GraphicsDevice(), shaderLoader, context->ShaderCache(), context->GraphicsMeshCache())
				, m_data(nullptr)
				, m_defaultLightingModel(defaultLightingModel == nullptr ? ForwardLightingModel::Instance() : defaultLightingModel)
				, m_synchThreadCount(std::thread::hardware_concurrency())
				, m_lightTypeIds(lightTypeIds), m_perLightDataSize(perLightDataSize) {
				m_data = new SceneGraphicsData(this);
			}
			
			inline virtual ~SceneGraphicsContext(){}

			Object* Data()const { return m_data; }

			inline void Synch() {
				WriteLock lock(this);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				{
					// Add/Remove Objects&Lights:
					std::unique_lock<std::mutex> pendingLock(m_pendingPipelineLock);
					data->sceneObjects.Update();
					data->sceneLights.Update();
				}
				{
					auto synchJob = [](ThreadBlock::ThreadInfo threadInfo, void* dataAddr) {
						SceneGraphicsData* data = (SceneGraphicsData*)dataAddr;
						const Reference<GraphicsObjectSynchronizer>* const begin = data->synchronizers.Data();
						const Reference<GraphicsObjectSynchronizer>* const end = begin + data->synchronizers.Size();
						for (const Reference<GraphicsObjectSynchronizer>* ptr = begin + threadInfo.threadId; ptr < end; ptr += threadInfo.threadCount)
							(*ptr)->OnGraphicsSynch();
					};
					m_synchBlock.Execute(m_synchThreadCount, data, Callback<ThreadBlock::ThreadInfo, void*>(synchJob));
				}
				m_onPostGraphicsSynch();
			}

			virtual Event<>& OnPostGraphicsSynch() override { return m_onPostGraphicsSynch; }

			virtual LightingModel* DefaultLightingModel()const { return m_defaultLightingModel; }

			virtual inline void AddSceneObject(GraphicsObjectDescriptor* descriptor) override {
				Add([&](SceneGraphicsData* data) { return &data->sceneObjects; }, descriptor);
			}
			virtual inline void RemoveSceneObject(GraphicsObjectDescriptor* descriptor) override {
				Remove([&](SceneGraphicsData* data) { return &data->sceneObjects; }, descriptor);
			}
			virtual inline Event<const Reference<GraphicsObjectDescriptor>*, size_t>& OnSceneObjectsAdded() override { 
				return m_onSceneObjectSetChanged.onAdded; 
			}
			virtual inline Event<const Reference<GraphicsObjectDescriptor>*, size_t>& OnSceneObjectsRemoved() override { 
				return m_onSceneObjectSetChanged.onRemoved; 
			}
			virtual inline void GetSceneObjects(const Reference<GraphicsObjectDescriptor>*& descriptors, size_t& count) override {
				GetValues([&](SceneGraphicsData* data) { return &data->sceneObjects; }, descriptors, count);
			}
			
			virtual bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const override {
				std::unordered_map<std::string, uint32_t>::const_iterator it = m_lightTypeIds.find(lightTypeName);
				if (it != m_lightTypeIds.end()) {
					lightTypeId = it->second;
					return true;
				}
				else return false;
			}

			virtual size_t PerLightDataSize()const { return m_perLightDataSize; }

			virtual void AddSceneLightDescriptor(LightDescriptor* descriptor) override {
				Add([&](SceneGraphicsData* data) { return &data->sceneLights; }, descriptor);
			}
			virtual void RemoveSceneLightDescriptor(LightDescriptor* descriptor) override {
				Remove([&](SceneGraphicsData* data) { return &data->sceneLights; }, descriptor);
			}
			virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsAdded() override { 
				return m_onSceneLightSetChanged.onAdded; 
			}
			virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsRemoved() override { 
				return m_onSceneLightSetChanged.onRemoved; 
			}
			virtual void GetSceneLightDescriptors(const Reference<LightDescriptor>*& descriptors, size_t& count) override {
				GetValues([&](SceneGraphicsData* data) { return &data->sceneLights; }, descriptors, count);
			}
		};


		class ScenePhysicsContext : public virtual PhysicsContext {
		private:
			const Reference<Physics::PhysicsScene> m_scene;
			EventInstance<> m_onPostPhysicsSynch;
			std::atomic<float> m_updateRate = 60.0f;
			Stopwatch m_stopwatch;
			std::atomic<float> m_deltaTime = 0.0f;
			std::atomic<float> m_scaledDeltaTime = 0.0f;

		public:
			inline ScenePhysicsContext(Physics::PhysicsInstance* instance) : m_scene(instance->CreateScene(std::thread::hardware_concurrency() / 4)) {
				m_scene->SimulateAsynch(0.01f);
			}

			inline virtual ~ScenePhysicsContext() {
				m_scene->SynchSimulation();
			}

			inline virtual Vector3 Gravity()const override { return m_scene->Gravity(); }
			inline virtual void SetGravity(const Vector3& value) override { m_scene->SetGravity(value); }

			inline virtual bool LayersInteract(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b)const override { return m_scene->LayersInteract(a, b); }
			inline virtual void FilterLayerInteraction(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b, bool enableIntaraction) {
				m_scene->FilterLayerInteraction(a, b, enableIntaraction);
			}

			inline virtual Reference<Physics::DynamicBody> AddRigidBody(const Matrix4& transform, bool enabled) override { return m_scene->AddRigidBody(transform, enabled); }
			inline virtual Reference<Physics::StaticBody> AddStaticBody(const Matrix4& transform, bool enabled) override { return m_scene->AddStaticBody(transform, enabled); }

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

			inline virtual size_t Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
				, const Callback<const RaycastHit&>& onHitFound
				, const Physics::PhysicsCollider::LayerMask& layerMask, Physics::PhysicsScene::QueryFlags flags
				, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter
				, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter)const override {
				
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

			inline virtual Event<>& OnPostPhysicsSynch() override { return m_onPostPhysicsSynch; }

			inline virtual Physics::PhysicsInstance* APIInstance()const override { return m_scene->APIInstance(); }

			inline virtual float UpdateRate()const override { return m_updateRate; }
			inline virtual void SetUpdateRate(float rate)override { m_updateRate = rate; }

			inline virtual float ScaledDeltaTime()const override { return m_scaledDeltaTime; }
			inline virtual float UnscaledDeltaTime()const override { return m_deltaTime; }

			template<typename PreUpdate, typename PostUpdate>
			inline void SynchIfReady(const PreUpdate& preUpdate, const PostUpdate& postUpdate, float timeScale = 1.0f) {
				float rate = UpdateRate();
				float minInterval = (rate > 0.0f ? (1.0f / rate) : 0.0f);
				if (m_stopwatch.Elapsed() <= minInterval) return;
				m_deltaTime = m_stopwatch.Reset();
				m_scaledDeltaTime = m_deltaTime * timeScale;
				preUpdate();
				m_scene->SynchSimulation();
				m_scene->SimulateAsynch(m_scaledDeltaTime);
				m_onPostPhysicsSynch();
				postUpdate();
			}
		};

		class FullSceneContext : public virtual SceneContext {
		private:
			std::recursive_mutex m_updateLock;
			Stopwatch m_stopwatch;
			std::atomic<float> m_timeScale = 1.0f;
			std::atomic<float> m_deltaTime = 0.0f;
			std::atomic<float> m_scaledDeltaTime = 0.0f;

			class SceneData : public virtual Object {
			private:
				const Reference<FullSceneContext> m_context;

			public:
				inline SceneData(FullSceneContext* context) : m_context(context) {}

				inline virtual ~SceneData() {
					std::unique_lock<std::recursive_mutex> lock(m_context->m_updateLock);
					m_context->m_data = nullptr;
				}

				DelayedObjectSet<Object> allComponents;
				ObjectSet<Updatable> updatables;
				ObjectSet<PrePhysicsSynchUpdater> prePhysicsSynchUpdaters;
				ObjectSet<PostPhysicsSynchUpdater> postPhysicsSynchUpdaters;
			};

			SceneData* m_data;

			inline FullSceneContext(AppContext* appContext, GraphicsContext* graphics, PhysicsContext* physics, const OS::Input* input, Audio::AudioScene* audioScene)
				: SceneContext(appContext, graphics, physics, input, audioScene) {
				m_data = new SceneData(this);
			}

		public:
			inline static Reference<SceneContext> Create(
				AppContext* context, ShaderLoader* shaderLoader, const OS::Input* input, 
				const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize, LightingModel* defaultLightingModel) {
				Reference<GraphicsContext> graphics = Object::Instantiate<SceneGraphicsContext>(context, shaderLoader, lightTypeIds, perLightDataSize, defaultLightingModel);
				Reference<PhysicsContext> physics = Object::Instantiate<ScenePhysicsContext>(context->PhysicsInstance());
				Reference<Audio::AudioScene> audioScene = context->AudioDevice()->CreateScene();
				Reference<SceneContext> scene = new FullSceneContext(context, graphics, physics, input, audioScene);
				scene->ReleaseRef();
				return scene;
			}

			Object* Data()const { return m_data; }

			inline void Update() {
				std::unique_lock<std::recursive_mutex> lock(m_updateLock);
				if (m_data == nullptr) return;
				m_deltaTime = m_stopwatch.Reset();
				m_scaledDeltaTime = m_deltaTime * m_timeScale;
				m_data->allComponents.Flush(
					[&](const Reference<Object>* removed, size_t count) {
						for (size_t i = 0; i < count; i++) {
							Object* component = removed[i];
							m_data->updatables.Remove(dynamic_cast<Updatable*>(component));
							m_data->prePhysicsSynchUpdaters.Remove(dynamic_cast<PrePhysicsSynchUpdater*>(component));
							m_data->postPhysicsSynchUpdaters.Remove(dynamic_cast<PostPhysicsSynchUpdater*>(component));
						}
					}, [&](const Reference<Object>* added, size_t count) {
						for (size_t i = 0; i < count; i++) {
							Object* component = added[i];
							m_data->updatables.Add(dynamic_cast<Updatable*>(component));
							m_data->prePhysicsSynchUpdaters.Add(dynamic_cast<PrePhysicsSynchUpdater*>(component));
							m_data->postPhysicsSynchUpdaters.Add(dynamic_cast<PostPhysicsSynchUpdater*>(component));
						}
					});
				dynamic_cast<ScenePhysicsContext*>(Physics())->SynchIfReady(
					[&]() {
						const Reference<PrePhysicsSynchUpdater>* updaters = m_data->prePhysicsSynchUpdaters.Data();
						size_t count = m_data->prePhysicsSynchUpdaters.Size();
						for (size_t i = 0; i < count; i++) updaters[i]->PrePhysicsSynch();
					}, [&]() {
						const Reference<PostPhysicsSynchUpdater>* updaters = m_data->postPhysicsSynchUpdaters.Data();
						size_t count = m_data->postPhysicsSynchUpdaters.Size();
						for (size_t i = 0; i < count; i++) updaters[i]->PostPhysicsSynch();
					}, m_timeScale);
				{
					const Reference<Updatable>* updatables = m_data->updatables.Data();
					size_t count = m_data->updatables.Size();
					for (size_t i = 0; i < count; i++) updatables[i]->Update();
				}
			}

			inline void ComponentDestroyed(Component* component) {
				std::unique_lock<std::recursive_mutex> lock(m_updateLock);
				if (m_data == nullptr) return;
				m_data->allComponents.Remove(component);
			}

			inline std::recursive_mutex& UpdateLock() { return m_updateLock; }

			virtual float ScaledDeltaTime()const override { return m_scaledDeltaTime; }

			virtual float UnscaledDeltaTime()const override { return m_deltaTime; }

		protected:
			inline virtual void ComponentInstantiated(Component* component) override {
				std::unique_lock<std::recursive_mutex> lock(m_updateLock);
				if (m_data == nullptr || component == nullptr) return;
				m_data->allComponents.Add(component);
				component->OnDestroyed() += Callback<Component*>(&FullSceneContext::ComponentDestroyed, this);
			}
		};

		class RootComponent : public virtual Component {
		private:
			const Callback<const void*> m_resetRootComponent;

		public:
			inline RootComponent(const Callback<const void*>& resetRootComponent, SceneContext* context) 
				: Component(context, "SceneRoot"), m_resetRootComponent(resetRootComponent) {
				OnDestroyed() += Callback<Component*>(&RootComponent::OnDestroyedByUser, this);
			}

			inline virtual void SetParent(Component*) override {
				Context()->Log()->Fatal("Scene Root Object can not have a parent!");
			}

			inline void OnDestroyedByUser(Component*) {
				OnDestroyed() -= Callback<Component*>(&RootComponent::OnDestroyedByUser, this);
				m_resetRootComponent((const void*)(&m_resetRootComponent));
			}
		};
	}

	Scene::Scene(
		AppContext* context, ShaderLoader* shaderLoader, const OS::Input* input, 
		const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize, LightingModel* defaultLightingModel)
		: m_context(FullSceneContext::Create(context, shaderLoader, input, lightTypeIds, perLightDataSize, defaultLightingModel)) {
		m_sceneGraphicsData = dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Data();
		m_sceneGraphicsData->ReleaseRef();
		m_sceneData = dynamic_cast<FullSceneContext*>(m_context.operator->())->Data();
		m_sceneData->ReleaseRef();

		void (*resetRootComponent)(Scene*, const void*) = [](Scene* scene, const void* callbackPtr) {
			std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
			const Callback<const void*>& resetRootComponent = *((const Callback<const void*>*)callbackPtr);
			scene->m_rootObject = Object::Instantiate<RootComponent>(resetRootComponent, scene->m_context);
		};
		m_rootObject = Object::Instantiate<RootComponent>(Callback<const void*>(resetRootComponent, this), m_context);
	}

	Scene::~Scene() { 
		m_rootObject->OnDestroyed() -= Callback<Component*>(&RootComponent::OnDestroyedByUser, dynamic_cast<RootComponent*>(m_rootObject.operator->()));
		m_rootObject->Destroy();
		SynchGraphics();
	}

	SceneContext* Scene::Context()const { return m_context; }

	Component* Scene::RootObject()const { return m_rootObject; }

	void Scene::SynchGraphics() { dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Synch(); }

	void Scene::Update() { dynamic_cast<FullSceneContext*>(m_context.operator->())->Update(); }

	std::recursive_mutex& Scene::UpdateLock() { return dynamic_cast<FullSceneContext*>(m_context.operator->())->UpdateLock(); }
}
