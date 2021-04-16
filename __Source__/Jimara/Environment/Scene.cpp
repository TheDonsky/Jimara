#include "Scene.h"
#include "../Graphics/Data/GraphicsPipelineSet.h"
#include "../Components/Interfaces/Updatable.h"
#include <mutex>


namespace Jimara {
	namespace {
		class SceneGraphicsContext : public virtual GraphicsContext {
		private:
			class SceneGraphicsData : public virtual Object {
			public:
				Graphics::GraphicsObjectSet sceneObjectPipelineSet;

				ObjectSet<Graphics::GraphicsPipeline::Descriptor> addedPipelines;
				ObjectSet<Graphics::GraphicsPipeline::Descriptor> removedPipelines;


				ObjectSet<LightDescriptor> sceneLightDescriptors;

				ObjectSet<LightDescriptor> addedLights;
				ObjectSet<LightDescriptor> removedLights;


				ObjectSet<GraphicsContext::GraphicsObjectSynchronizer> synchronizers;


				const Reference<SceneGraphicsContext> m_context;


			public:
				inline void AddCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Add(synchronizer);
				}

				inline void RemoveCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Remove(synchronizer);
				}

			private:
				inline void OnSceneObjectPipelinesAdded(const Reference<Graphics::GraphicsPipeline::Descriptor>* pipelines, size_t count, Graphics::GraphicsObjectSet*) {
					for (size_t i = 0; i < count; i++) AddCallbacks(pipelines[i]);
					m_context->m_onSceneObjectPipelinesAdded(pipelines, count);
				}

				inline void OnSceneObjectPipelinesRemoved(const Reference<Graphics::GraphicsPipeline::Descriptor>* pipelines, size_t count, Graphics::GraphicsObjectSet*) {
					for (size_t i = 0; i < count; i++) RemoveCallbacks(pipelines[i]);
					m_context->m_onSceneObjectPipelinesRemoved(pipelines, count);
				}

			public:
				inline SceneGraphicsData(SceneGraphicsContext* context) : m_context(context) {
					sceneObjectPipelineSet.AddChangeCallbacks(
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&SceneGraphicsData::OnSceneObjectPipelinesAdded, this),
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&SceneGraphicsData::OnSceneObjectPipelinesRemoved, this));
				}

				inline ~SceneGraphicsData() {
					sceneObjectPipelineSet.RemoveChangeCallbacks(
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&SceneGraphicsData::OnSceneObjectPipelinesAdded, this),
						Callback<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t, Graphics::GraphicsObjectSet*>(&SceneGraphicsData::OnSceneObjectPipelinesRemoved, this));
					GraphicsContext::WriteLock lock(m_context);
					m_context->m_data = nullptr;
				}
			};

			std::atomic<SceneGraphicsData*> m_data;

			std::mutex m_pendingPipelineLock;
			
			EventInstance<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t> m_onSceneObjectPipelinesAdded;
			EventInstance<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t> m_onSceneObjectPipelinesRemoved;

			EventInstance<const Reference<LightDescriptor>*, size_t> m_onSceneLightDescriptorsAdded;
			EventInstance<const Reference<LightDescriptor>*, size_t> m_onSceneLightDescriptorsRemoved;

			EventInstance<> m_onPostGraphicsSynch;

			const size_t m_synchThreadCount;
			ThreadBlock m_synchBlock;

			const std::unordered_map<std::string, uint32_t> m_lightTypeIds;

		public:
			inline SceneGraphicsContext(AppContext* context, const std::unordered_map<std::string, uint32_t>& lightTypeIds)
				: GraphicsContext(context->GraphicsDevice(), context->ShaderCache(), context->GraphicsMeshCache())
				, m_data(nullptr), m_synchThreadCount(std::thread::hardware_concurrency()), m_lightTypeIds(lightTypeIds) {
				m_data = new SceneGraphicsData(this);
			}

			Object* Data()const { return m_data; }

			inline void Synch() {
				WriteLock lock(this);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				{
					std::unique_lock<std::mutex> pendingLock(m_pendingPipelineLock);

					// Add/Remove Pipelines:
					if (data->removedPipelines.Size() > 0) {
						data->sceneObjectPipelineSet.RemovePipelines(data->removedPipelines.Data(), data->removedPipelines.Size());
						data->removedPipelines.Clear();
					}
					if (data->addedPipelines.Size() > 0) {
						data->sceneObjectPipelineSet.AddPipelines(data->addedPipelines.Data(), data->addedPipelines.Size());
						data->addedPipelines.Clear();
					}

					// Add/Remove Lights:
					if (data->removedLights.Size() > 0) {
						data->sceneLightDescriptors.Remove(data->removedLights.Data(), data->removedLights.Size(),
							[&](const Reference<LightDescriptor>* removed, size_t count) {
								for (size_t i = 0; i < count; i++) data->RemoveCallbacks(removed[i]);
								m_onSceneLightDescriptorsRemoved(removed, count); 
							});
						data->removedLights.Clear();
					}
					if (data->addedLights.Size() > 0) {
						data->sceneLightDescriptors.Add(data->addedLights.Data(), data->addedLights.Size(),
							[&](const Reference<LightDescriptor>* added, size_t count) { 
								for (size_t i = 0; i < count; i++) data->AddCallbacks(added[i]);
								m_onSceneLightDescriptorsAdded(added, count); 
							});
						data->addedLights.Clear();
					}
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


			virtual inline void AddSceneObjectPipeline(Graphics::GraphicsPipeline::Descriptor* descriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedPipelines.Add(descriptor);
				data->removedPipelines.Remove(descriptor);
			}

			virtual inline void RemoveSceneObjectPipeline(Graphics::GraphicsPipeline::Descriptor* descriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedPipelines.Remove(descriptor);
				data->removedPipelines.Add(descriptor);
			}

			virtual inline Event<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t>& OnSceneObjectPipelinesAdded() override { return m_onSceneObjectPipelinesAdded; }

			virtual inline Event<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t>& OnSceneObjectPipelinesRemoved() override { return m_onSceneObjectPipelinesRemoved; }

			virtual inline void GetSceneObjectPipelines(const Reference<Graphics::GraphicsPipeline::Descriptor>*& pipelines, size_t& count) override {
				SceneGraphicsData* data = m_data;
				if (data == nullptr) {
					pipelines = nullptr;
					count = 0;
				}
				else data->sceneObjectPipelineSet.GetAllPipelines(pipelines, count);
			}

			
			virtual uint32_t GetLightTypeId(const std::string& lightTypeName)const override {
				std::unordered_map<std::string, uint32_t>::const_iterator it = m_lightTypeIds.find(lightTypeName);
				if (it != m_lightTypeIds.end()) return it->second;
				else {
					Log()->Error("SceneGraphicsContext::GetLightTypeId - Unknown light type \"" + lightTypeName + "\"!");
					return (~0u);
				}
			}

			virtual void AddSceneLightDescriptor(LightDescriptor* descriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedLights.Add(descriptor);
				data->removedLights.Remove(descriptor);
			}

			virtual void RemoveSceneLightDescriptor(LightDescriptor* descriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedLights.Remove(descriptor);
				data->removedLights.Add(descriptor);
			}

			virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsAdded() override { return m_onSceneLightDescriptorsAdded; }

			virtual Event<const Reference<LightDescriptor>*, size_t>& OnSceneLightDescriptorsRemoved() override { return m_onSceneLightDescriptorsRemoved; }

			virtual void GetSceneLightDescriptors(const Reference<LightDescriptor>*& descriptors, size_t& count) override {
				SceneGraphicsData* data = m_data;
				if (data == nullptr) {
					descriptors = nullptr;
					count = 0;
				}
				else {
					descriptors = data->sceneLightDescriptors.Data();
					count = data->sceneLightDescriptors.Size();
				}
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

				DelayedObjectSet<Object> allComponents;
				ObjectSet<Updatable> updatables;
			};

			SceneData* m_data;

			inline FullSceneContext(AppContext* appContext, GraphicsContext* graphics) 
				: SceneContext(appContext, graphics) {
				m_data = new SceneData(this);
			}

		public:
			inline static Reference<SceneContext> Create(AppContext* context, const std::unordered_map<std::string, uint32_t>& lightTypeIds) {
				Reference<GraphicsContext> graphics = Object::Instantiate<SceneGraphicsContext>(context, lightTypeIds);
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

	Scene::Scene(AppContext* context, const std::unordered_map<std::string, uint32_t>& lightTypeIds)
		: m_context(FullSceneContext::Create(context, lightTypeIds)) {
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
