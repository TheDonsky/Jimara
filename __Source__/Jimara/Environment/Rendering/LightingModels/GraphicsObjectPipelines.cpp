#include "GraphicsObjectPipelines.h"


namespace Jimara {
	struct GraphicsObjectPipelines::Helpers {
		class BaseJob;
		class EndOfUpdateJob;

		class DescriptorPools;
		class DescriptorSetUpdateJob;

		class GraphicsObjectDescriptorManager;
		class GraphicsObjectDescriptorManagerCache;
		class GraphicsObjectDescriptorManagerCleanupJob;

		class PipelineInstance;
		class PipelineInstanceSet;
		class PipelineInstanceCollection;
		class PipelineCreationJob;
		class PipelineCreationFlushJob;

		class PerContextData;
		class PerContextDataCache;
	};





#pragma region JOB_WITH_FRAME_COUNTER_FOR_FILTERING
	/// <summary> 
	/// All jobs within the system should only execute once during a single update cycle,
	/// but there's a slim chance that in case of the Editor, for example, to query these tasks 
	/// anyway and cause 'double' update, which is bad news. To prevent all this,
	/// we have BaseJob and EndOfSimulationJob
	/// </summary>
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


	/// <summary>
	/// Requres all update jobs as dependencies and bumps frame counter to make sure 
	/// the update jobs are executed on the next frame
	/// </summary>
	class GraphicsObjectPipelines::Helpers::EndOfUpdateJob : public virtual JobSystem::Job {
	private:
		const std::shared_ptr<std::atomic<size_t>> m_frameCounter;
		const std::vector<Reference<JobSystem::Job>> m_descriptorUpdateJobs;
		std::atomic<size_t> m_lastFrameId;

	public:
		inline EndOfUpdateJob(
			const std::shared_ptr<std::atomic<size_t>>& frameCounter,
			const std::vector<Reference<JobSystem::Job>>& descriptorUpdateJobs)
			: m_frameCounter(frameCounter)
			, m_descriptorUpdateJobs(descriptorUpdateJobs) {
			assert(m_frameCounter != nullptr);
			m_lastFrameId = m_frameCounter->load() - 1u;
		}
		inline virtual ~EndOfUpdateJob() {}
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
#pragma endregion





#pragma region SHARED_DESCRIPTOR_POOLS
	/// <summary>
	/// We have fixed set of binding pools per context;
	/// They are shared between all graphics object pipelines more or less by random and 
	/// are updated simultanuously
	/// </summary>
	class GraphicsObjectPipelines::Helpers::DescriptorPools : public virtual Object {
	private:
		// List of all descriptor pools
		const std::vector<Reference<Graphics::BindingPool>> m_pools;

		// GetNextPool() returns pools from m_pools one after another and loops; This is the counter for it
		std::atomic<size_t> m_allocateCounter = 0u;

		// Constructor is private
		inline DescriptorPools(std::vector<Reference<Graphics::BindingPool>>&& pools) : m_pools(std::move(pools)) {}

	public:
		/// <summary>
		/// Creates pools
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="poolCount"> Binding pool count (0 means half of the thread count on the system) </param>
		/// <returns> Pool collection instance </returns>
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

		/// <summary> Virtual destructor </summary>
		inline virtual ~DescriptorPools() {}

		/// <summary> Number of pools </summary>
		inline size_t PoolCount()const { return m_pools.size(); }

		/// <summary> Pool by index </summary>
		inline Graphics::BindingPool* Pool(size_t index)const { return m_pools[index]; }

		/// <summary> Returns pools in rotation order </summary>
		inline Graphics::BindingPool* GetNextPool() {
			return m_pools[m_allocateCounter.fetch_add(1u) % m_pools.size()];
		}
	};


	/// <summary>
	/// Updates descriptor sets after the pipelines are generated
	/// (Created one per BindingPool within DescriptorPools)
	/// </summary>
	class GraphicsObjectPipelines::Helpers::DescriptorSetUpdateJob : public virtual BaseJob {
	private:
		// Scene context
		const Reference<Scene::GraphicsContext> m_context;

		// Target binding pool
		const Reference<Graphics::BindingPool> m_pool;

		// GraphicsObjectDescriptorManagerCleanupJob
		const Reference<JobSystem::Job> m_objectListCleanupJob;


	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene graphics context </param>
		/// <param name="pool"> Binding pool to update on each execution </param>
		/// <param name="objectListCleanupJob"> GraphicsObjectDescriptorManagerCleanupJob </param>
		/// <param name="frameCounter"> BaseJob filter </param>
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

		/// <summary> Virtual destructor </summary>
		inline virtual ~DescriptorSetUpdateJob() {}

	protected:
		/// <summary> Updates binding pool </summary>
		virtual void Run() final override {
			m_pool->UpdateAllBindingSets(m_context->InFlightCommandBufferIndex());
		}

		/// <summary> 
		/// Reports GraphicsObjectDescriptorManagerCleanupJob as dependency 
		/// to make sure all binding sets are already allocated when this one runs 
		/// </summary>
		virtual void CollectDependencies(Callback<JobSystem::Job*> addDependency) override {
			addDependency(m_objectListCleanupJob);
		}
	};
#pragma endregion





#pragma region GRAPHICS_OBJECT_COLLECTIONS
	/// <summary>
	/// After pipelines and binding sets are created, this job does some cleanup for corresponding 
	/// GraphicsObjectDescriptorManager objects
	/// </summary>
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCleanupJob : public virtual BaseJob {
	private:
		// PipelineCreationJob instances for the system
		const std::vector<Reference<JobSystem::Job>> m_pipelineCretionJobs;

		// GraphicsObjectDescriptorManager cleanup callbacks
		EventInstance<> m_onCleanup;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="creationJobs"> PipelineCreationJob instances for the system </param>
		/// <param name="frameCounter"> BaseJob filter </param>
		inline GraphicsObjectDescriptorManagerCleanupJob(
			const std::vector<Reference<JobSystem::Job>>& creationJobs,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter), m_pipelineCretionJobs(creationJobs) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~GraphicsObjectDescriptorManagerCleanupJob() {}

		/// <summary> GraphicsObjectDescriptorManager cleanup callbacks </summary>
		inline Event<>& OnCleanup() { return m_onCleanup; }

	protected:
		/// <summary> Executes cleanup callbacks </summary>
		virtual void Run() final override { m_onCleanup(); }

		/// <summary> Reports PipelineCreationJob instances as dependencies </summary>
		virtual void CollectDependencies(Callback<JobSystem::Job*> addDependency) override {
			const Reference<JobSystem::Job>* ptr = m_pipelineCretionJobs.data();
			const Reference<JobSystem::Job>* end = ptr + m_pipelineCretionJobs.size();
			while (ptr < end) {
				addDependency(*ptr);
				ptr++;
			}
		}
	};


	/// <summary>
	/// Manages added, removed and active GraphicsObjectDescriptor instances
	/// per GraphicsObjectDescriptor::Set
	/// </summary>
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		// Set
		const Reference<const GraphicsObjectDescriptor::Set> m_set;

		// GraphicsObjectDescriptorManagerCleanupJob for clearing m_added, m_removed and m_all
		const Reference<GraphicsObjectDescriptorManagerCleanupJob> m_cleanupJob;

		// GraphicsObjectDescriptor instances that got added during the last render cycle
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_added;

		// GraphicsObjectDescriptor instances that got removed during the last render cycle
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_removed;

		// Invoked on set->OnAdded() event
		void OnAdded(Jimara::GraphicsObjectDescriptor* const* elems, size_t count) {
			m_added.clear();
			Jimara::GraphicsObjectDescriptor* const* const end = elems + count;
			while (elems < end) {
				m_added.push_back(*elems);
				elems++;
			}
		}

		// Invoked on set->OnRemoved() event
		void OnRemoved(Jimara::GraphicsObjectDescriptor* const* elems, size_t count) {
			m_removed.clear();
			Jimara::GraphicsObjectDescriptor* const* const end = elems + count;
			while (elems < end) {
				m_removed.push_back(*elems);
				elems++;
			}
		}

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="set"> Object set </param>
		/// <param name="cleanupJob"> GraphicsObjectDescriptorManagerCleanupJob for clearing up content </param>
		inline GraphicsObjectDescriptorManager(
			const GraphicsObjectDescriptor::Set* set,
			GraphicsObjectDescriptorManagerCleanupJob* cleanupJob)
			: m_set(set), m_cleanupJob(cleanupJob) {
			assert(m_set != nullptr);
			m_set->OnAdded() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnRemoved, this);
			m_cleanupJob->OnCleanup() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::Clear, this);
		}

		/// <summary> Destructor </summary>
		inline virtual ~GraphicsObjectDescriptorManager() {
			m_set->OnAdded() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnRemoved, this);
			m_cleanupJob->OnCleanup() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::Clear, this);
			Clear();
		}

		/// <summary> Clears stored data </summary>
		inline void Clear() {
			OnAdded(nullptr, 0u);
			OnRemoved(nullptr, 0u);
		}

		/// <summary> Object set </summary>
		inline const GraphicsObjectDescriptor::Set* Set()const { return m_set; };

		/// <summary> Number of elements, added during the last render cycle </summary>
		inline size_t AddedElementCount()const { return m_added.size(); }

		/// <summary> Elements, added during the last render cycle </summary>
		inline const Reference<Jimara::GraphicsObjectDescriptor>* AddedElements()const { return m_added.data(); }

		/// <summary> Number of elements, removed during the last render cycle </summary>
		inline size_t RemovedElementCount()const { return m_removed.size(); }

		/// <summary> Elements, removed during the last render cycle </summary>
		inline const Reference<Jimara::GraphicsObjectDescriptor>* RemovedElements()const { return m_removed.data(); }
	};


	/// <summary>
	/// GraphicsObjectDescriptorManager instances are created on a per-GraphicsObjectDescriptor::Set basis within a single context;
	/// This is a cache for making instance management easy
	/// </summary>
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCache
		: public virtual ObjectCache<Reference<const Jimara::Object>> {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~GraphicsObjectDescriptorManagerCache() {}

		/// <summary>
		/// Gets cached instance
		/// </summary>
		/// <param name="set"> GraphicsObjectDescriptor::Set </param>
		/// <param name="cleanupJob"> Shared GraphicsObjectDescriptorManagerCleanupJob </param>
		/// <returns> Shared GraphicsObjectDescriptorManager instance </returns>
		Reference<GraphicsObjectDescriptorManager> Get(
			GraphicsObjectDescriptor::Set* set,
			GraphicsObjectDescriptorManagerCleanupJob* cleanupJob) {
			return GetCachedOrCreate(set, false, [&]() -> Reference<GraphicsObjectDescriptorManager> {
				Object::Instantiate<GraphicsObjectDescriptorManager>(set, cleanupJob);
				});
		}
	};
#pragma endregion





#pragma region GRAPHICS_PIPELINE_INSTANCES
	class GraphicsObjectPipelines::Helpers::PipelineInstance : public virtual Object {
		// __TODO__: Implement this crap!
	};


	class GraphicsObjectPipelines::Helpers::PipelineInstanceSet : public virtual Object {
	public:
		struct GraphicsObjectData {
			mutable ObjectInfo info;
			mutable Reference<const Object> cacheEntry;
			inline GraphicsObjectData() {}
			inline GraphicsObjectData(const GraphicsObjectDescriptor* desc) { info.m_descriptor = desc; }
		};

	private:
		const Reference<const GraphicsObjectDescriptorManager> m_set;
		
		bool m_isUninitialized = true;
		std::atomic<size_t> m_index = 0u;
		std::atomic<uint32_t> m_entriesAdded = 0u;
		
		std::mutex m_entryLock;
		ObjectSet<const GraphicsObjectDescriptor, GraphicsObjectData> m_entries;


		inline void AddEntries(const Reference<GraphicsObjectDescriptor>* const elements, const size_t count) {
			while (true) {
				const size_t index = m_index.fetch_add(1u);
				if (index >= count) break;
				GraphicsObjectDescriptor* graphicsObject = elements[index];
				// __TODO__: Check if we need this element!
				// __TODO__: Get viewport data if leyer is OK!
				// __TODO__: Get cached PipelineInstance for the viewport data!
				
				std::unique_lock<std::mutex> lock(m_entryLock);
				m_entries.Add(&graphicsObject, 1u, [&](const GraphicsObjectData* data, size_t cnt) {
					assert(cnt == 1u);
					assert(data->info.m_descriptor == graphicsObject);
					// __TODO__: Fill these in!
					data->info.m_viewportData = nullptr;
					data->info.m_graphicsPipeline = nullptr;
					data->info.m_bindingSets = {};
					data->cacheEntry = nullptr;
					});
			}
			m_entriesAdded = 1u;
		}

		inline void RemoveOldEntries() {
			const size_t count = m_set->RemovedElementCount();
			if (count <= 0u) return;
			std::unique_lock<std::mutex> lock(m_entryLock);
			m_entries.Remove(m_set->RemovedElements(), count, [](const auto&, const auto) {});
		}

		inline void AddNewEntries() {
			AddEntries(m_set->AddedElements(), m_set->AddedElementCount());
		}

		inline void AddAllEntries() {
			static thread_local std::vector<Reference<GraphicsObjectDescriptor>> all;
			if (!(m_entriesAdded.load() > 0u)) {
				all.clear();
				m_set->Set()->GetAll([&](GraphicsObjectDescriptor* desc) { all.push_back(desc); });
				if (!(m_entriesAdded.load() > 0u))
					AddEntries(all.data(), all.size());
			}
			all.clear();
		}

	public:
		inline PipelineInstanceSet(const GraphicsObjectDescriptorManager* set)
			: m_set(set) {
			assert(m_set != nullptr);
		}

		inline virtual ~PipelineInstanceSet() {}

		void UpdateObjects() {
			if (m_isUninitialized)
				AddAllEntries();
			else {
				RemoveOldEntries();
				AddNewEntries();
			}
		}

		void FlushChanges() {
			m_isUninitialized = false;
			m_index = 0u;
			m_entriesAdded = 0u;
		}

		inline size_t PipelineCount()const { return m_entries.Size(); }
		inline const GraphicsObjectData* Data()const { return m_entries.Data(); }
	};
	

	class GraphicsObjectPipelines::Helpers::PipelineInstanceCollection : public virtual Object {
	private:
		const Reference<SceneContext> m_context;
		std::mutex m_modifyLock;
		DelayedObjectSet<PipelineInstanceSet> m_pipelineSets;

		void Flush() {
			std::unique_lock<std::mutex> lock(m_modifyLock);
			m_pipelineSets.Flush(
				[&](const Reference<PipelineInstanceSet>* removed, size_t count) {},
				[&](const Reference<PipelineInstanceSet>* added, size_t count) {});
		}

	public:
		inline PipelineInstanceCollection(SceneContext* context) : m_context(context) {
			assert(m_context != nullptr);
			GraphicsObjectDescriptor::OnFlushSceneObjectCollections(m_context) += 
				Callback(&GraphicsObjectPipelines::Helpers::PipelineInstanceCollection::Flush, this);
		}

		inline virtual ~PipelineInstanceCollection() {
			GraphicsObjectDescriptor::OnFlushSceneObjectCollections(m_context) -= 
				Callback(&GraphicsObjectPipelines::Helpers::PipelineInstanceCollection::Flush, this);
		}

		inline void Add(PipelineInstanceSet* set) { 
			std::unique_lock<std::mutex> lock(m_modifyLock);
			m_pipelineSets.ScheduleAdd(set); 
		}

		inline void Remove(PipelineInstanceSet* set) {
			std::unique_lock<std::mutex> lock(m_modifyLock);
			m_pipelineSets.ScheduleRemove(set);
		}

		inline size_t SetCount()const { return m_pipelineSets.Size(); }
		inline PipelineInstanceSet* Set(size_t index)const { return m_pipelineSets[index]; }
	};
	
	
	class GraphicsObjectPipelines::Helpers::PipelineCreationJob : public virtual BaseJob {
	private:
		const Reference<const PipelineInstanceCollection> m_pipelineInstanceCollection;
		const size_t m_creationJobCount;
		const size_t m_index;

	public:
		inline PipelineCreationJob(
			PipelineInstanceCollection* pipelineInstanceCollection,
			size_t creationJobCount, size_t index,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter)
			, m_pipelineInstanceCollection(pipelineInstanceCollection)
			, m_creationJobCount(creationJobCount), m_index(index) {
			assert(m_pipelineInstanceCollection != nullptr);
		}

		inline virtual ~PipelineCreationJob() {}

	protected:
		virtual void Run() final override {
			size_t count = m_pipelineInstanceCollection->SetCount();
			for (size_t s = 0u; s < m_creationJobCount; s++)
				for (size_t i = ((s + m_index) % m_creationJobCount); i < count; i += m_creationJobCount)
					m_pipelineInstanceCollection->Set(i)->UpdateObjects();
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) override { }
	};


	class GraphicsObjectPipelines::Helpers::PipelineCreationFlushJob : public virtual BaseJob {
	private:
		const Reference<const PipelineInstanceCollection> m_pipelineInstanceCollection;
		const Reference<JobSystem::Job> m_cleanupJob;

	public:
		inline PipelineCreationFlushJob(
			PipelineInstanceCollection* pipelineInstanceCollection,
			JobSystem::Job* cleanupJob,
			const std::shared_ptr<const std::atomic<size_t>>& frameCounter)
			: BaseJob(frameCounter)
			, m_pipelineInstanceCollection(pipelineInstanceCollection), m_cleanupJob(cleanupJob) {
			assert(m_pipelineInstanceCollection != nullptr);
			assert(cleanupJob != nullptr);
		}

		inline virtual ~PipelineCreationFlushJob() {}

	protected:
		virtual void Run() final override {
			size_t count = m_pipelineInstanceCollection->SetCount();
			for (size_t i = 0u; i < count; i++)
				m_pipelineInstanceCollection->Set(i)->FlushChanges();
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) override { 
			addDependency(m_cleanupJob);
		}
	};
#pragma endregion





#pragma region MANAGEMENT_SYSTEM_PER_SCENE_CONTEXT
	class GraphicsObjectPipelines::Helpers::PerContextData
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		const Reference<SceneContext> m_context;
		const Reference<DescriptorPools> m_descriptorPools;
		const Reference<JobSystem::Job> m_endOfFrameJob;

	public:
		inline PerContextData(
			SceneContext* context,
			DescriptorPools* descriptorPools,
			JobSystem::Job* endOfFrameJob)
			: m_context(context)
			, m_descriptorPools(descriptorPools)
			, m_endOfFrameJob(endOfFrameJob) {
			assert(m_context != nullptr);
			assert(m_descriptorPools != nullptr);
			assert(m_endOfFrameJob != nullptr);
			m_context->Graphics()->RenderJobs().Add(m_endOfFrameJob);
		}

		virtual inline ~PerContextData() {
			m_context->Graphics()->RenderJobs().Remove(m_endOfFrameJob);
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

				const Reference<PipelineInstanceCollection> pipelineInstanceSets = 
					Object::Instantiate<PipelineInstanceCollection>(context);

				const std::shared_ptr<std::atomic<size_t>> frameCounter = std::make_shared<std::atomic<size_t>>();

				std::vector<Reference<JobSystem::Job>> pipelineCreationJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					pipelineCreationJobs.push_back(Object::Instantiate<PipelineCreationJob>(pipelineInstanceSets, pools->PoolCount(), i, frameCounter));
				const Reference<GraphicsObjectDescriptorManagerCleanupJob> cleanupJob =
					Object::Instantiate<GraphicsObjectDescriptorManagerCleanupJob>(pipelineCreationJobs, frameCounter);

				std::vector<Reference<JobSystem::Job>> updateAndFlushJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					updateAndFlushJobs.push_back(Object::Instantiate<DescriptorSetUpdateJob>(context->Graphics(), pools->Pool(i), cleanupJob, frameCounter));
				updateAndFlushJobs.push_back(Object::Instantiate<PipelineCreationFlushJob>(pipelineInstanceSets, cleanupJob, frameCounter));
				const Reference<EndOfUpdateJob> endOfFrameJob = Object::Instantiate<EndOfUpdateJob>(frameCounter, updateAndFlushJobs);

				// __TODO__: Implement this crap!
				context->Log()->Error("GraphicsObjectPipelines::Helpers::PerContextDataCache::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
				});
		}
	};
#pragma endregion





	Reference<GraphicsObjectPipelines> GraphicsObjectPipelines::Get(const Descriptor& desc) {
		if (desc.descriptorSet == nullptr) return nullptr;

		const Reference<Helpers::PerContextData> manager = Helpers::PerContextDataCache::Get(desc.descriptorSet->Context());
		if (manager == nullptr) return nullptr;

		desc.descriptorSet->Context()->Log()->Error("GraphicsObjectPipelines::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		// __TODO__: Implement this crap!
		return nullptr;
	}

	GraphicsObjectPipelines::~GraphicsObjectPipelines() {}

	inline size_t GraphicsObjectPipelines::ObjectCount()const { return m_objectInfoCount; }

	inline const GraphicsObjectPipelines::ObjectInfo& GraphicsObjectPipelines::Object(size_t index)const {
		return reinterpret_cast<const Helpers::PipelineInstanceSet::GraphicsObjectData*>(m_objectInfos)[index].info;
	}

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
