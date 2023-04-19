#include "GraphicsObjectPipelines.h"


namespace Jimara {
	struct GraphicsObjectPipelines::Helpers {
		class BaseJob;

		class DescriptorPools;
		class PipelineCreationJob;
		class DescriptorSetUpdateJob;
		class FrameCounterJob;

		class GraphicsObjectDescriptorManager;
		class GraphicsObjectDescriptorManagerCache;
		class GraphicsObjectDescriptorManagerCleanupJob;

		class PerContextData;
		class PerContextDataCache;
	};





	class GraphicsObjectPipelines::Helpers::BaseJob : public virtual JobSystem::Job {
	private:
		const std::shared_ptr<const std::atomic<size_t>> m_frameCounter;
		std::atomic<size_t> m_lastFrameId;

	protected:
		inline BaseJob(const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: m_frameCounter(frameCounter) {
			assert(m_frameCounter != nullptr);
			m_lastFrameId = m_frameCounter->load() - 1u;
		}

		inline virtual ~BaseJob() {}

		virtual void Run() = 0;

		inline virtual void Execute() final override {
			const size_t frameId = m_frameCounter->load() - 1u;
			if (m_lastFrameId == frameId) return;
			m_lastFrameId = frameId;
			Run();
		}
	};





	class GraphicsObjectPipelines::Helpers::DescriptorPools : public virtual Object {
	private:
		const std::vector<Reference<Graphics::BindingPool>> m_pools;
		std::atomic<size_t> m_allocateCounter = 0u;

		inline DescriptorPools(std::vector<Reference<Graphics::BindingPool>>&& pools) : m_pools(std::move(pools)) {}
	public:
		inline static Reference<DescriptorPools> Create(SceneContext* context, size_t poolCount = 0u) {
			if (poolCount <= 0u)
				poolCount = Math::Max(size_t(1u), size_t(std::thread::hardware_concurrency() >> 1u));
			std::vector<Reference<Graphics::BindingPool>> pools;
			for (size_t i = 0u; i < poolCount; i++) {
				const Reference<Graphics::BindingPool> pool = context->Graphics()->Device()->CreateBindingPool(
					context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (pool == nullptr) {
					context->Log()->Error(
						"GraphicsObjectPipelines::Helpers::DescriptorPools::Create - ",
						"Failed to create binding pool ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				pools.push_back(pool);
			}
			const Reference<DescriptorPools> result = new DescriptorPools(std::move(pools));
			result->ReleaseRef();
			return result;
		}

		inline virtual ~DescriptorPools() {}

		inline size_t PoolCount()const { return m_pools.size(); }

		inline Graphics::BindingPool* Pool(size_t index)const { return m_pools[index]; }

		inline Graphics::BindingPool* GetNextPool() {
			return m_pools[m_allocateCounter.fetch_add(1u) % m_pools.size()];
		}
	};





	class GraphicsObjectPipelines::Helpers::DescriptorSetUpdateJob : public virtual BaseJob {
	private:
		const Reference<Scene::GraphicsContext> m_context;
		const Reference<Graphics::BindingPool> m_pool;
		const Reference<JobSystem::Job> m_objectListCleanupJob;
		

	public:
		inline DescriptorSetUpdateJob(
			Scene::GraphicsContext* context, 
			Graphics::BindingPool* pool, 
			JobSystem::Job* objectListCleanupJob,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter)
			, m_context(context), m_pool(pool), m_objectListCleanupJob(objectListCleanupJob) {
			assert(m_context != nullptr);
			assert(m_pool != nullptr); 
			assert(m_objectListCleanupJob != nullptr);
		}
		inline virtual ~DescriptorSetUpdateJob() {}

	protected:
		virtual void Run() final override { 
			m_pool->UpdateAllBindingSets(m_context->InFlightCommandBufferIndex());
		}
		virtual void CollectDependencies(Callback<JobSystem::Job*> addDependency) override {
			addDependency(m_objectListCleanupJob);
		}
	};




	class GraphicsObjectPipelines::Helpers::FrameCounterJob : public virtual JobSystem::Job {
	private:
		const std::shared_ptr<std::atomic<size_t>> m_frameCounter;
		const std::vector<Reference<JobSystem::Job>> m_descriptorUpdateJobs;
		std::atomic<size_t> m_lastFrameId;

	public:
		inline FrameCounterJob(
			const std::shared_ptr<std::atomic<size_t>>& frameCounter,
			const std::vector<Reference<JobSystem::Job>>& descriptorUpdateJobs)
			: m_frameCounter(frameCounter)
			, m_descriptorUpdateJobs(descriptorUpdateJobs) {
			assert(m_frameCounter != nullptr);
			m_lastFrameId = m_frameCounter->load() - 1u;
		}
		inline virtual ~FrameCounterJob() {}
		inline virtual void Execute() final override { 
			(*m_frameCounter.operator->())++; 
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) {
			const Reference<JobSystem::Job>* ptr = m_descriptorUpdateJobs.data();
			const Reference<JobSystem::Job>* const end = ptr + m_descriptorUpdateJobs.size();
			while (ptr < end) {
				addDependency(*ptr);
				ptr++;
			}
		}
	};





	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCleanupJob : public virtual BaseJob {
	private:
		const std::vector<Reference<JobSystem::Job>> m_pipelineCretionJobs;
		EventInstance<> m_onCleanup;

	public:
		inline GraphicsObjectDescriptorManagerCleanupJob(
			const std::vector<Reference<JobSystem::Job>>& creationJobs,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter), m_pipelineCretionJobs(creationJobs) {}

		inline virtual ~GraphicsObjectDescriptorManagerCleanupJob() {}

		inline Event<>& OnCleanup() { return m_onCleanup; }

	protected:
		virtual void Run() final override { m_onCleanup(); }
		virtual void CollectDependencies(Callback<JobSystem::Job*> addDependency) override {
			const Reference<JobSystem::Job>* ptr = m_pipelineCretionJobs.data();
			const Reference<JobSystem::Job>* end = ptr + m_pipelineCretionJobs.size();
			while (ptr < end) {
				addDependency(*ptr);
				ptr++;
			}
		}
	};
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager 
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		const Reference<const GraphicsObjectDescriptor::Set> m_set;
		const Reference<GraphicsObjectDescriptorManagerCleanupJob> m_cleanupJob;
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_added;
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_removed;
		std::mutex m_allLock;
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_all;

		void OnAdded(Jimara::GraphicsObjectDescriptor* const* elems, size_t count) {
			m_added.clear();
			Jimara::GraphicsObjectDescriptor* const* const end = elems + count;
			while (elems < end) {
				m_added.push_back(*elems);
				elems++;
			}
		}
		void OnRemoved(Jimara::GraphicsObjectDescriptor* const* elems, size_t count) {
			m_removed.clear();
			Jimara::GraphicsObjectDescriptor* const* const end = elems + count;
			while (elems < end) {
				m_removed.push_back(*elems);
				elems++;
			}
		}
		void OnFlushed() {
			std::unique_lock<std::mutex> m_allLock;
			m_all.clear();
		}
		void StoreAllElements() {
			std::unique_lock<std::mutex> m_allLock;
			if (!m_all.empty()) return;
			m_set->GetAll([&](Jimara::GraphicsObjectDescriptor* elem) { m_all.push_back(elem); });
		}

	public:
		inline GraphicsObjectDescriptorManager(
			const GraphicsObjectDescriptor::Set* set,
			GraphicsObjectDescriptorManagerCleanupJob* cleanupJob) 
			: m_set(set), m_cleanupJob(cleanupJob) {
			assert(m_set != nullptr);
			m_set->OnAdded() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnRemoved, this);
			m_set->OnFlushed() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnFlushed, this);
			m_cleanupJob->OnCleanup() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::Clear, this);
		}

		inline virtual ~GraphicsObjectDescriptorManager() {
			m_set->OnAdded() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnRemoved, this);
			m_set->OnFlushed() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnFlushed, this);
			m_cleanupJob->OnCleanup() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::Clear, this);
			Clear();
		}

		inline void Clear() {
			OnAdded(nullptr, 0u);
			OnRemoved(nullptr, 0u);
			OnFlushed();
		}

		inline size_t AddedElementCount()const { return m_added.size(); }
		inline const Reference<Jimara::GraphicsObjectDescriptor>* AddedElements()const { return m_added.data(); }

		inline size_t RemovedElementCount()const { return m_removed.size(); }
		inline const Reference<Jimara::GraphicsObjectDescriptor>* RemovedElements()const { return m_removed.data(); }

		inline size_t LiveCount() {
			StoreAllElements();
			return m_all.size();
		}
		inline const Reference<Jimara::GraphicsObjectDescriptor>* LiveElements() {
			StoreAllElements();
			return m_all.data();
		}
	};
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCache 
		: public virtual ObjectCache<Reference<const Jimara::Object>> {
	public:
		inline virtual ~GraphicsObjectDescriptorManagerCache() {}
		Reference<GraphicsObjectDescriptorManager> Get(
			GraphicsObjectDescriptor::Set* set,
			GraphicsObjectDescriptorManagerCleanupJob* cleanupJob) {
			return GetCachedOrCreate(set, false, [&]() -> Reference<GraphicsObjectDescriptorManager> {
				Object::Instantiate<GraphicsObjectDescriptorManager>(set, cleanupJob);
				});
		}
	};





	class GraphicsObjectPipelines::Helpers::PipelineCreationJob : public virtual BaseJob {
	private:
		const Reference<DescriptorPools> m_descriptorPools;

	public:
		inline PipelineCreationJob(
			DescriptorPools* pools,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter),
			m_descriptorPools(pools) {
			assert(m_descriptorPools != nullptr);
		}

		inline virtual ~PipelineCreationJob() {}

	protected:
		virtual void Run() final override {
			// __TODO__: Implement this crap!
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) override { }
	};





	class GraphicsObjectPipelines::Helpers::PerContextData
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		const Reference<SceneContext> m_context;
		const Reference<DescriptorPools> m_descriptorPools;
		const Reference<JobSystem::Job> m_frameCounterJob;

	public:
		inline PerContextData(
			SceneContext* context,
			DescriptorPools* descriptorPools,
			JobSystem::Job* frameCounterJob)
			: m_context(context)
			, m_descriptorPools(descriptorPools)
			, m_frameCounterJob(frameCounterJob) {
			assert(m_context != nullptr);
			assert(m_descriptorPools != nullptr);
			assert(m_frameCounterJob != nullptr);
			m_context->Graphics()->RenderJobs().Add(m_frameCounterJob);
		}

		virtual inline ~PerContextData() {
			m_context->Graphics()->RenderJobs().Remove(m_frameCounterJob);
		}
	};
	class GraphicsObjectPipelines::Helpers::PerContextDataCache
		: public virtual ObjectCache<Reference<const Jimara::Object>> {
	public:
		static Reference<PerContextData> Get(SceneContext* context) {
			static PerContextDataCache cache;
			return cache.GetCachedOrCreate(context, false, [&]() -> Reference<PerContextData> {
				const Reference<DescriptorPools> pools = DescriptorPools::Create(context);
				if (pools == nullptr) return nullptr;

				const std::shared_ptr<std::atomic<size_t>> frameCounter = std::make_shared<std::atomic<size_t>>();

				std::vector<Reference<JobSystem::Job>> pipelineCreationJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					pipelineCreationJobs.push_back(Object::Instantiate<PipelineCreationJob>(pools, frameCounter));
				const Reference<GraphicsObjectDescriptorManagerCleanupJob> cleanupJob =
					Object::Instantiate<GraphicsObjectDescriptorManagerCleanupJob>(pipelineCreationJobs, frameCounter);

				std::vector<Reference<JobSystem::Job>> bindingSetUpdateJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					bindingSetUpdateJobs.push_back(Object::Instantiate<DescriptorSetUpdateJob>(context->Graphics(), pools->Pool(i), cleanupJob, frameCounter));
				const Reference<FrameCounterJob> frameCounterJob = Object::Instantiate<FrameCounterJob>(frameCounter, bindingSetUpdateJobs);

				// __TODO__: Implement this crap!
				context->Log()->Error("GraphicsObjectPipelines::Helpers::PerContextDataCache::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
				});
		}
	};





	Reference<GraphicsObjectPipelines> GraphicsObjectPipelines::Get(const Descriptor& desc) {
		if (desc.descriptorSet == nullptr) return nullptr;

		const Reference<Helpers::PerContextData> manager = Helpers::PerContextDataCache::Get(desc.descriptorSet->Context());
		if (manager == nullptr) return nullptr;

		desc.descriptorSet->Context()->Log()->Error("GraphicsObjectPipelines::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		// __TODO__: Implement this crap!
		return nullptr;
	}

	GraphicsObjectPipelines::~GraphicsObjectPipelines() {}

	void GraphicsObjectPipelines::GetUpdateTasks(const Callback<JobSystem::Job*> recordUpdateTasks)const {
		// __TODO__: Implement this crap!
	}

	void GraphicsObjectPipelines::ObjectInfo::ExecutePipeline(const Graphics::InFlightBufferInfo& inFlightBuffer)const {
		const Reference<Graphics::BindingSet>* ptr = m_bindingSets.Data();
		const Reference<Graphics::BindingSet>* const end = ptr + m_bindingSets.Size();
		while (ptr < end) {
			(*ptr)->Bind(inFlightBuffer);
			ptr++;
		}
		const Graphics::IndirectDrawBufferReference indirectBuffer = m_viewportData->IndirectBuffer();
		if (indirectBuffer == nullptr)
			m_graphicsPipeline->Draw(inFlightBuffer, m_viewportData->IndexCount(), m_viewportData->InstanceCount());
		else m_graphicsPipeline->DrawIndirect(inFlightBuffer, indirectBuffer, m_viewportData->InstanceCount());
	}
}
