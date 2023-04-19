#include "GraphicsObjectPipelines.h"


namespace Jimara {
	struct GraphicsObjectPipelines::Helpers {
		class DescriptorPools;
		class PipelineCreationJob;
		class DescriptorSetUpdateJob;

		class GraphicsObjectDescriptorManager;
		class GraphicsObjectDescriptorManagerCache;

		class InstanceManager;
		class InstanceManagerCache;
	};





	class GraphicsObjectPipelines::Helpers::DescriptorPools : public virtual Object {
	private:
		const std::vector<Reference<Graphics::BindingPool>> m_pools;
		std::atomic<size_t> m_allocateCounter = 0u;

		inline DescriptorPools(std::vector<Reference<Graphics::BindingPool>>&& pools) : m_pools(std::move(pools)) {}
	public:
		inline Reference<DescriptorPools> Create(SceneContext* context, size_t poolCount = 0u) {
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





	class GraphicsObjectPipelines::Helpers::PipelineCreationJob : public virtual JobSystem::Job {
	private:
		const Reference<DescriptorPools> m_descriptorPools;

	public:
		inline PipelineCreationJob(DescriptorPools* pools) 
			: m_descriptorPools(pools) { 
			assert(m_descriptorPools != nullptr);
		}

		inline virtual ~PipelineCreationJob() {}

	protected:
		virtual void Execute() override {
			// __TODO__: Implement this crap!
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) override { }
	};





	class GraphicsObjectPipelines::Helpers::DescriptorSetUpdateJob : public virtual JobSystem::Job {
	private:
		const Reference<Scene::GraphicsContext> m_context;
		const Reference<Graphics::BindingPool> m_pool;
		const std::shared_ptr<const std::atomic<size_t>> m_frameCounter;
		std::atomic<size_t> m_lastFrameId;

	public:
		inline DescriptorSetUpdateJob(
			Scene::GraphicsContext* context, 
			Graphics::BindingPool* pool, 
			std::shared_ptr<const std::atomic<size_t>> frameCounter)
			: m_context(context), m_pool(pool), m_frameCounter(frameCounter) {
			assert(m_context != nullptr);
			assert(m_pool != nullptr); 
			assert(m_frameCounter != nullptr);
			m_lastFrameId = m_frameCounter->load() - 1u;
		}
		inline virtual ~DescriptorSetUpdateJob() {}

	protected:
		virtual void Execute() override { 
			const size_t frameId = m_frameCounter->load() - 1u;
			if (m_lastFrameId == frameId) return;
			m_lastFrameId = frameId;
			m_pool->UpdateAllBindingSets(m_context->InFlightCommandBufferIndex());
		}
		virtual void CollectDependencies(Callback<Job*> addDependency) override {
			// __TODO__: These should run after pipeline creation tasks...
		}
	};





	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager 
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		const Reference<const GraphicsObjectDescriptor::Set> m_set;
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_added;
		std::vector<Reference<Jimara::GraphicsObjectDescriptor>> m_removed;

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

	public:
		inline GraphicsObjectDescriptorManager(const GraphicsObjectDescriptor::Set* set) : m_set(set) {
			assert(m_set != nullptr);
			m_set->OnAdded() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() += Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
		}

		inline virtual ~GraphicsObjectDescriptorManager() {
			m_set->OnAdded() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			m_set->OnRemoved() -= Callback(&GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManager::OnAdded, this);
			Clear();
		}

		inline void Clear() {
			m_added.clear();
			m_removed.clear();
		}
	};
	class GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCache 
		: public virtual ObjectCache<Reference<const Jimara::Object>> {
	public:
		inline virtual ~GraphicsObjectDescriptorManagerCache() {}
		Reference<GraphicsObjectDescriptorManager> Get(GraphicsObjectDescriptor::Set* set) {
			return GetCachedOrCreate(set, false, [&]() -> Reference<GraphicsObjectDescriptorManager> {
				// __TODO__: Implement this crap!
				set->Context()->Log()->Error(
					"GraphicsObjectPipelines::Helpers::GraphicsObjectDescriptorManagerCache::Get - ",
					"Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				});
		}
	};





	class GraphicsObjectPipelines::Helpers::InstanceManager 
		: public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:

	public:

	};
	class GraphicsObjectPipelines::Helpers::InstanceManagerCache 
		: public virtual ObjectCache<Reference<const Jimara::Object>> {
	public:
		static Reference<InstanceManager> Get(SceneContext* context) {
			static InstanceManagerCache cache;
			return cache.GetCachedOrCreate(context, false, [&]() -> Reference<InstanceManager> {
				// __TODO__: Implement this crap!
				context->Log()->Error("GraphicsObjectPipelines::Helpers::ManagerCache::Get - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
				});
		}
	};





	Reference<GraphicsObjectPipelines> GraphicsObjectPipelines::Get(const Descriptor& desc) {
		if (desc.descriptorSet == nullptr) return nullptr;

		const Reference<Helpers::InstanceManager> manager = Helpers::InstanceManagerCache::Get(desc.descriptorSet->Context());
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
