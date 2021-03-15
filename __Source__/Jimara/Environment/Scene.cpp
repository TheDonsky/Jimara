#include "Scene.h"
#include "../Graphics/Data/GraphicsPipelineSet.h"
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

				ObjectSet<GraphicsContext::GraphicsObjectSynchronizer> synchronizers;


			private:
				const Reference<SceneGraphicsContext> m_context;

				inline void AddCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Add(synchronizer);
				}

				inline void RemoveCallbacks(Object* object) {
					GraphicsContext::GraphicsObjectSynchronizer* synchronizer = dynamic_cast<GraphicsContext::GraphicsObjectSynchronizer*>(object);
					if (synchronizer != nullptr) synchronizers.Remove(synchronizer);
				}

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

			const size_t m_synchThreadCount;
			ThreadBlock m_synchBlock;

		public:
			inline SceneGraphicsContext(AppContext* context)
				: GraphicsContext(context->GraphicsDevice(), context->ShaderCache(), context->GraphicsMeshCache())
				, m_data(nullptr), m_synchThreadCount(std::thread::hardware_concurrency()) {
				m_data = new SceneGraphicsData(this);
			}

			Object* Data()const { return m_data; }

			inline void Synch() {
				WriteLock lock(this);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				{
					std::unique_lock<std::mutex> pendingLock(m_pendingPipelineLock);
					if (data->removedPipelines.Size() > 0) {
						data->sceneObjectPipelineSet.RemovePipelines(data->removedPipelines.Data(), data->removedPipelines.Size());
						data->removedPipelines.Clear();
					}
					if (data->addedPipelines.Size() > 0) {
						data->sceneObjectPipelineSet.AddPipelines(data->addedPipelines.Data(), data->addedPipelines.Size());
						data->addedPipelines.Clear();
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
			}

			virtual inline void AddSceneObjectPipeline(Graphics::GraphicsPipeline::Descriptor* decriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedPipelines.Add(decriptor);
				data->removedPipelines.Remove(decriptor);
			}

			virtual inline void RemoveSceneObjectPipeline(Graphics::GraphicsPipeline::Descriptor* decriptor) override {
				std::unique_lock<std::mutex> lock(m_pendingPipelineLock);
				SceneGraphicsData* data = m_data;
				if (data == nullptr) return;
				data->addedPipelines.Remove(decriptor);
				data->removedPipelines.Add(decriptor);
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
		};

		class RootComponent : public virtual Component {
		public:
			inline RootComponent(SceneContext* context) : Component(context, "SceneRoot") {}

			inline virtual void SetParent(Component*) override {
				Context()->Log()->Fatal("Scene Root Object can not have a parent!");
			}
		};
	}

	Scene::Scene(AppContext* context) 
		: m_context([&]() -> Reference<SceneContext> {
		Reference<GraphicsContext> graphics = Object::Instantiate<SceneGraphicsContext>(context);
		return Object::Instantiate<SceneContext>(context, graphics);
			}()) {
		m_sceneGraphicsData = dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Data();
		m_sceneGraphicsData->ReleaseRef();
		m_rootObject = Object::Instantiate<RootComponent>(m_context);
	}

	Scene::~Scene() { m_rootObject->Destroy(); }

	SceneContext* Scene::Context()const { return m_context; }

	Component* Scene::RootObject()const { return m_rootObject; }

	void Scene::SynchGraphics() { dynamic_cast<SceneGraphicsContext*>(m_context->Graphics())->Synch(); }
}
