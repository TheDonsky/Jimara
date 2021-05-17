#include "Scene.h"
#include "../Graphics/Data/GraphicsPipelineSet.h"
#include "../Components/Interfaces/Updatable.h"
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
			AddRemoveEvents<Graphics::GraphicsPipeline::Descriptor> m_onScenePipelineSetChanged;
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

				inline ~SceneGraphicsData() { m_context->m_data = nullptr; }
			};

			std::atomic<SceneGraphicsData*> m_data;

			EventInstance<> m_onPostGraphicsSynch;

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
			inline SceneGraphicsContext(AppContext* context, ShaderLoader* shaderLoader, const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize)
				: GraphicsContext(context->GraphicsDevice(), shaderLoader, context->ShaderCache(), context->GraphicsMeshCache())
				, m_data(nullptr), m_synchThreadCount(std::thread::hardware_concurrency())
				, m_lightTypeIds(lightTypeIds), m_perLightDataSize(perLightDataSize) {
				m_data = new SceneGraphicsData(this);
			}

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


		class FullSceneContext : public virtual SceneContext {
		private:
			std::recursive_mutex m_updateLock;

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
			};

			SceneData* m_data;

			inline FullSceneContext(AppContext* appContext, GraphicsContext* graphics) 
				: SceneContext(appContext, graphics) {
				m_data = new SceneData(this);
			}

		public:
			inline static Reference<SceneContext> Create(
				AppContext* context, ShaderLoader* shaderLoader, const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize) {
				Reference<GraphicsContext> graphics = Object::Instantiate<SceneGraphicsContext>(context, shaderLoader, lightTypeIds, perLightDataSize);
				Reference<SceneContext> scene = new FullSceneContext(context, graphics);
				scene->ReleaseRef();
				return scene;
			}

			Object* Data()const { return m_data; }

			inline void Update() {
				std::unique_lock<std::recursive_mutex> lock(m_updateLock);
				if (m_data == nullptr) return;
				m_data->allComponents.Flush(
					[&](const Reference<Object>* removed, size_t count) {
						for (size_t i = 0; i < count; i++) {
							Object* component = removed[i];
							m_data->updatables.Remove(dynamic_cast<Updatable*>(component));
						}
					}, [&](const Reference<Object>* added, size_t count) {
						for (size_t i = 0; i < count; i++) {
							Object* component = added[i];
							m_data->updatables.Add(dynamic_cast<Updatable*>(component));
						}
					});
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

		protected:
			inline virtual void ComponentInstantiated(Component* component) override {
				std::unique_lock<std::recursive_mutex> lock(m_updateLock);
				if (m_data == nullptr || component == nullptr) return;
				m_data->allComponents.Add(component);
				component->OnDestroyed() += Callback<Component*>(&FullSceneContext::ComponentDestroyed, this);
			}
		};

		class RootComponent : public virtual Component {
		public:
			inline RootComponent(SceneContext* context) : Component(context, "SceneRoot") {}

			inline virtual void SetParent(Component*) override {
				Context()->Log()->Fatal("Scene Root Object can not have a parent!");
			}
		};
	}

	Scene::Scene(AppContext* context, ShaderLoader* shaderLoader, const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize)
		: m_context(FullSceneContext::Create(context, shaderLoader, lightTypeIds, perLightDataSize)) {
		m_sceneGraphicsData = dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Data();
		m_sceneGraphicsData->ReleaseRef();
		m_sceneData = dynamic_cast<FullSceneContext*>(m_context.operator->())->Data();
		m_sceneData->ReleaseRef();
		m_rootObject = Object::Instantiate<RootComponent>(m_context);
	}

	Scene::~Scene() { m_rootObject->Destroy(); }

	SceneContext* Scene::Context()const { return m_context; }

	Component* Scene::RootObject()const { return m_rootObject; }

	void Scene::SynchGraphics() { dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Synch(); }

	void Scene::Update() { dynamic_cast<FullSceneContext*>(m_context.operator->())->Update(); }
}
