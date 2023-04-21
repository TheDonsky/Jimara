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

		struct BindingSetInstance;
		class BindingSetInstanceCache;
		class PipelineInstanceSet;
		class PipelineInstanceCollection;
		class PipelineCreationJob;
		class PipelineCreationFlushJob;

		class PerContextData;
		class PerContextDataCache;

		class Instance;
		class InstanceCache;
	};





#pragma region JOB_WITH_FRAME_COUNTER_FOR_FILTERING
	/// <summary> 
	/// All jobs within the system should only execute once during a single update cycle,
	/// but there's a slim chance that in case of the Editor, for example, to query these tasks 
	/// anyway and cause 'double' update, which is bad news. To prevent all this,
	/// we have BaseJob and EndOfSimulationJob
	/// </summary>
	class GraphicsObjectPipelines::Helpers::BaseJob : public virtual JobSystem::Job {
	public:
		using Toggle = std::shared_ptr<const std::atomic<uint32_t>>;

	private:
		const Toggle m_toggle;

	protected:
		inline BaseJob(const Toggle& toggle)
			: m_toggle(toggle) {
			assert(m_toggle != nullptr);
		}

		inline virtual ~BaseJob() {}

		virtual void Run() = 0;

		inline virtual void Execute() final override {
			if (m_toggle->load() <= 0u) return;
			Run();
		}
	};


	/// <summary>
	/// Requres all update jobs as dependencies and bumps frame counter to make sure 
	/// the update jobs are executed on the next frame
	/// </summary>
	class GraphicsObjectPipelines::Helpers::EndOfUpdateJob : public virtual JobSystem::Job {
	private:
		const Reference<SceneContext> m_context;
		const std::shared_ptr<std::atomic<uint32_t>> m_toggle;
		const std::vector<Reference<JobSystem::Job>> m_descriptorUpdateJobs;

		void OnStartFrame() { m_toggle->store(1u); }

	public:
		inline EndOfUpdateJob(
			SceneContext* context,
			const std::shared_ptr<std::atomic<uint32_t>>& toggle,
			const std::vector<Reference<JobSystem::Job>>& descriptorUpdateJobs)
			: m_context(context), m_toggle(toggle), m_descriptorUpdateJobs(descriptorUpdateJobs) {
			assert(m_context != nullptr);
			assert(m_toggle != nullptr);
			GraphicsObjectDescriptor::OnFlushSceneObjectCollections(m_context) +=
				Callback(&GraphicsObjectPipelines::Helpers::EndOfUpdateJob::OnStartFrame, this);
		}
		inline virtual ~EndOfUpdateJob() {
			GraphicsObjectDescriptor::OnFlushSceneObjectCollections(m_context) -=
				Callback(&GraphicsObjectPipelines::Helpers::EndOfUpdateJob::OnStartFrame, this);
		}
		inline virtual void Execute() final override { m_toggle->store(0u); }
		inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
			const Reference<JobSystem::Job>* ptr = m_descriptorUpdateJobs.data();
			const Reference<JobSystem::Job>* const end = ptr + m_descriptorUpdateJobs.size();
			while (ptr < end) {
				addDependency(*ptr);
				ptr++;
			}
		}
		inline void GetDependencies(const Callback<Job*>& addDependency) { CollectDependencies(addDependency); }
		BaseJob::Toggle Toggle()const { return m_toggle; }
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
		/// <param name="toggle"> BaseJob enabler </param>
		inline DescriptorSetUpdateJob(
			Scene::GraphicsContext* context,
			Graphics::BindingPool* pool,
			JobSystem::Job* objectListCleanupJob,
			const BaseJob::Toggle& toggle)
			: BaseJob(toggle)
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
		/// <param name="toggle"> BaseJob enabler </param>
		inline GraphicsObjectDescriptorManagerCleanupJob(
			const std::vector<Reference<JobSystem::Job>>& creationJobs,
			const BaseJob::Toggle& toggle)
			: BaseJob(toggle), m_pipelineCretionJobs(creationJobs) {}

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
				return Object::Instantiate<GraphicsObjectDescriptorManager>(set, cleanupJob);
				});
		}
	};
#pragma endregion





#pragma region GRAPHICS_PIPELINE_INSTANCES
	struct GraphicsObjectPipelines::Helpers::BindingSetInstance : public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
		Stacktor<Reference<Graphics::BindingSet>, 4u> bindingSets;
		Stacktor<Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>>, 4u> vertexBuffers;
		Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBuffer;
		Reference<Graphics::Experimental::VertexInput> vertexInput;
	};

	class GraphicsObjectPipelines::Helpers::BindingSetInstanceCache : public virtual ObjectCache<Reference<const Jimara::Object>>::StoredObject {
	private:
		struct InstanceCache : public virtual ObjectCache<Reference<const Jimara::Object>> {
			inline Reference<const BindingSetInstance> Get(
				const GraphicsObjectDescriptor::ViewportData* viewportData, DescriptorPools* pools,
				Graphics::Experimental::GraphicsPipeline* pipeline, size_t firstBindingSetIndex, OS::Logger* log) {
				return GetCachedOrCreate(viewportData, false, [&]() -> Reference<BindingSetInstance> {
					auto fail = [&](const auto&... message) {
						log->Error("GraphicsObjectPipelines::Helpers::BindingSetInstanceCache::Get - ", message...);
						return nullptr;
					};
					const Reference<BindingSetInstance> result = Object::Instantiate<BindingSetInstance>();

					// Create binding sets:
					{
						Graphics::BindingPool* pool = pools->GetNextPool();
						Graphics::BindingSet::Descriptor desc = {};
						desc.pipeline = pipeline;
						desc.find = viewportData->BindingSearchFunctions();
						for (size_t i = firstBindingSetIndex; i < pipeline->BindingSetCount(); i++) {
							desc.bindingSetId = i;
							const Reference<Graphics::BindingSet> set = pool->AllocateBindingSet(desc);
							if (set == nullptr)
								return fail("Failed to create binding set for set ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							result->bindingSets.Push(set);
						}
					}

					// Create vertex input:
					{
						static thread_local std::vector<Graphics::ResourceBinding<Graphics::ArrayBuffer>*> constBindings;
						constBindings.clear();
						const size_t count = viewportData->VertexBufferCount() + viewportData->InstanceBufferCount();
						for (size_t i = 0u; i < count; i++) {
							const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> binding =
								Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
							result->vertexBuffers.Push(binding);
							constBindings.push_back(binding);
						}
						result->indexBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
						result->vertexInput = pipeline->CreateVertexInput(constBindings.data(), result->indexBuffer);
						constBindings.clear();
						if (result->vertexInput == nullptr)
							return fail("Failed to create vertex input! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					return result;
					});
			}
		};
		const Reference<InstanceCache> m_cache = Object::Instantiate<InstanceCache>();
		const Reference<DescriptorPools> m_pools;
		const Reference<Graphics::ShaderSet> m_shaderSet;
		const Reference<Graphics::Experimental::Pipeline> m_environmentPipeline;
		const Reference<OS::Logger> m_log;

	public:
		inline BindingSetInstanceCache(DescriptorPools* pools, Graphics::ShaderSet* shaderSet,
			Graphics::Experimental::Pipeline* environmentPipeline, OS::Logger* log) 
			: m_pools(pools), m_shaderSet(shaderSet), m_environmentPipeline(environmentPipeline), m_log(log) {
			assert(m_cache != nullptr);
			assert(m_pools != nullptr);
			assert(m_shaderSet != nullptr);
			assert(m_environmentPipeline != nullptr);
			assert(m_log != nullptr);
		}

		inline virtual ~BindingSetInstanceCache() {}

		inline Graphics::ShaderSet* ShaderSet()const { return m_shaderSet; }

		inline Graphics::Experimental::Pipeline* EnvironmentPipeline()const { return m_environmentPipeline; }

		inline Reference<const BindingSetInstance> Get(
			const GraphicsObjectDescriptor::ViewportData* viewportData, Graphics::Experimental::GraphicsPipeline* pipeline) {
			const size_t firstBindingSet = m_environmentPipeline == nullptr ? size_t(0u) : m_environmentPipeline->BindingSetCount();
			return m_cache->Get(viewportData, m_pools, pipeline, firstBindingSet, m_log);
		}

		class Factory : public virtual ObjectCache<Reference<const Jimara::Object>> {
		private:
			const Reference<DescriptorPools> m_pools;
			const Reference<OS::Logger> m_logger;

		public:
			inline Factory(DescriptorPools* pools, OS::Logger* logger)
				: m_pools(pools), m_logger(logger) {
				assert(m_pools != nullptr); 
				assert(m_logger != nullptr);
			}
			inline ~Factory() {}
			inline Reference<BindingSetInstanceCache> Get(
				const OS::Path& lightingModel,
				Graphics::ShaderLoader* shaderLoader,
				Graphics::RenderPass* renderPass) {
				auto fail = [&](const auto&... message) {
					m_logger->Error("GraphicsObjectPipelines::Helpers::BindingSetInstanceCache::Factory::Get - ", message...);
					return nullptr;
				};

				// Make sure input is valid:
				if (shaderLoader == nullptr)
					return fail("Shader loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				if (renderPass == nullptr)
					return fail("Render pass not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Load shader set:
				const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet(lightingModel);
				if (shaderSet == nullptr)
					return fail("Failed to load shader set for '", lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Get environment pipeline:
				static const Graphics::ShaderClass blankShader("Jimara/Environment/Rendering/LightingModels/Jimara_LightingModel_BlankShader");
				Graphics::Experimental::GraphicsPipeline::Descriptor desc = {};
				desc.vertexShader = shaderSet->GetShaderModule(&blankShader, Graphics::PipelineStage::VERTEX);
				desc.fragmentShader = shaderSet->GetShaderModule(&blankShader, Graphics::PipelineStage::FRAGMENT);
				if (desc.vertexShader == nullptr || desc.fragmentShader == nullptr)
					return fail("Failed to load blank shader for '", lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Reference<Graphics::Experimental::Pipeline> environmentPipeline = renderPass->GetGraphicsPipeline(desc);
				if (environmentPipeline == nullptr)
					return fail("Failed to create environment pipeline for '", lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create cached instance:
				return GetCachedOrCreate(shaderSet, false, [&]() -> Reference<BindingSetInstanceCache> {
					return Object::Instantiate<BindingSetInstanceCache>(m_pools, shaderSet, environmentPipeline, m_logger);
					});
			}
		};
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
		const Reference<BindingSetInstanceCache> m_pipelineInstanceCache;
		const Reference<Graphics::RenderPass> m_renderPass;
		const Reference<const ViewportDescriptor> m_viewport;
		const LayerMask m_layersMask;
		
		std::atomic<size_t> m_index = 0u;
		std::atomic<uint32_t> m_isUninitialized = 1u;
		std::atomic<uint32_t> m_entriesRemoved = 0u;
		std::atomic<uint32_t> m_entriesAdded = 0u;
		
		std::shared_mutex m_entryLock;
		ObjectSet<const GraphicsObjectDescriptor, GraphicsObjectData> m_entries;


		inline void AddEntries(const Reference<GraphicsObjectDescriptor>* const elements, const size_t count) {
			static const constexpr std::string_view FUNCTION_NAME = "GraphicsObjectPipelines::Helpers::PipelineInstanceSet::AddEntries - ";

			while (true) {
				// Increment index:
				const size_t index = m_index.fetch_add(1u);
				if (index >= count) break;

				// Filter out elements with invalid layer masks:
				GraphicsObjectDescriptor* const graphicsObject = elements[index];
				if (graphicsObject == nullptr || (!m_layersMask[graphicsObject->Layer()])) continue;

				// Get viewport data:
				const Reference<const GraphicsObjectDescriptor::ViewportData> viewportData = graphicsObject->GetViewportData(m_viewport);
				if (viewportData == nullptr) continue;
				if (viewportData->ShaderClass() == nullptr) {
					m_set->Set()->Context()->Log()->Warning(
						FUNCTION_NAME, "GraphicsObjectDescriptor::ViewportData has no ShaderClass! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}

				// Get shaders:
				const Graphics::ShaderClass* const shaderClass = viewportData->ShaderClass();
				const Reference<Graphics::SPIRV_Binary> vertexShader = m_pipelineInstanceCache->ShaderSet()
					->GetShaderModule(shaderClass, Graphics::PipelineStage::VERTEX);
				if (vertexShader == nullptr) {
					m_set->Set()->Context()->Log()->Error(
						FUNCTION_NAME, "Failed to load vertex shader for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				const Reference<Graphics::SPIRV_Binary> fragmentShader = m_pipelineInstanceCache->ShaderSet()
					->GetShaderModule(shaderClass, Graphics::PipelineStage::FRAGMENT);
				if (fragmentShader == nullptr) {
					m_set->Set()->Context()->Log()->Error(
						FUNCTION_NAME, "Failed to load vertex shader for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}

				// Get pipeline:
				Graphics::Experimental::GraphicsPipeline::Descriptor graphicsPipelineDescriptor = {};
				{
					graphicsPipelineDescriptor.vertexShader = vertexShader;
					graphicsPipelineDescriptor.fragmentShader = fragmentShader;
					graphicsPipelineDescriptor.blendMode = viewportData->BlendMode();
					graphicsPipelineDescriptor.indexType = viewportData->GeometryType();
					graphicsPipelineDescriptor.vertexInput = viewportData->VertexInputInfo();
				}
				const Reference<Graphics::Experimental::GraphicsPipeline> pipeline = m_renderPass->GetGraphicsPipeline(graphicsPipelineDescriptor);
				if (pipeline == nullptr) {
					m_set->Set()->Context()->Log()->Error(
						FUNCTION_NAME, "Failed to get / create graphics pipeline for '", shaderClass->ShaderPath(), "'![File:", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}

				// Get pipeline instance:
				const Reference<const BindingSetInstance> pipelineInstance = m_pipelineInstanceCache->Get(viewportData, pipeline);
				if (pipelineInstance == nullptr) {
					m_set->Set()->Context()->Log()->Error(
						FUNCTION_NAME, "Failed to create binding sets! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				
				// Insert pipelineInstance in current collection:
				std::unique_lock<std::shared_mutex> lock(m_entryLock);
				m_entries.Add(&graphicsObject, 1u, [&](const GraphicsObjectData* data, size_t cnt) {
					if (cnt <= 0) return;
					assert(cnt == 1u);
					assert(data->info.m_descriptor == graphicsObject);
					data->info.m_viewportData = viewportData;
					data->info.m_graphicsPipeline = pipeline;
					data->info.m_vertexBufferBindings = pipelineInstance->vertexBuffers;
					data->info.m_indexBufferBinding = pipelineInstance->indexBuffer;
					data->info.m_vertexInput = pipelineInstance->vertexInput;
					data->info.m_bindingSets = pipelineInstance->bindingSets;
					data->cacheEntry = pipelineInstance;
					});
			}
			m_entriesAdded = 1u;
		}

		inline void RemoveOldEntries() {
			const size_t count = m_set->RemovedElementCount();
			if (count <= 0u || m_entriesRemoved.load() > 0u) return;
			std::unique_lock<std::shared_mutex> lock(m_entryLock);
			if (m_entriesRemoved.load() > 0u) return;
			m_entries.Remove(m_set->RemovedElements(), count, [](const auto&, const auto) {});
			m_entriesRemoved = 1u;
		}

		inline void AddNewEntries() {
			AddEntries(m_set->AddedElements(), m_set->AddedElementCount());
		}

		inline void AddAllEntries() {
			static thread_local std::vector<Reference<GraphicsObjectDescriptor>> all;
			if (m_entriesAdded.load() <= 0u) {
				all.clear();
				m_set->Set()->GetAll([&](GraphicsObjectDescriptor* desc) { all.push_back(desc); });
				if (m_entriesAdded.load() <= 0u)
					AddEntries(all.data(), all.size());
			}
			all.clear();
		}

	public:
		inline PipelineInstanceSet(
			const GraphicsObjectDescriptorManager* set,
			BindingSetInstanceCache* pipelineInstanceCache,
			Graphics::RenderPass* renderPass,
			const ViewportDescriptor* viewport,
			const LayerMask& layerMask)
			: m_set(set)
			, m_pipelineInstanceCache(pipelineInstanceCache)
			, m_renderPass(renderPass)
			, m_viewport(viewport)
			, m_layersMask(layerMask) {
			assert(m_set != nullptr);
			assert(m_pipelineInstanceCache != nullptr);
			assert(m_renderPass != nullptr);
		}

		inline virtual ~PipelineInstanceSet() {}

		void UpdateObjects() {
			if (m_isUninitialized.load() > 0u)
				AddAllEntries();
			else {
				RemoveOldEntries();
				AddNewEntries();
			}
		}

		void FlushChanges() {
			m_isUninitialized = 0u;
			m_index = 0u;
			m_entriesRemoved = 0u;
			m_entriesAdded = 0u;
		}

		inline void Preinitialize() {
			if (m_isUninitialized.load() <= 0u) return;
			AddAllEntries();
			FlushChanges();
		}

		inline BindingSetInstanceCache* PipelineInstances()const { return m_pipelineInstanceCache; }

		inline std::shared_mutex& InstanceLock() { return m_entryLock; }
		inline const ObjectSet<const GraphicsObjectDescriptor, GraphicsObjectData>& Instances()const { return m_entries; }
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
			const BaseJob::Toggle& toggle)
			: BaseJob(toggle)
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
			const BaseJob::Toggle& toggle)
			: BaseJob(toggle)
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
	public:
		const Reference<SceneContext> context;
		const Reference<DescriptorPools> descriptorPools;
		const Reference<EndOfUpdateJob> endOfFrameJob;
		const Reference<GraphicsObjectDescriptorManagerCleanupJob> cleanupJob;
		const Reference<PipelineInstanceCollection> pipelineInstanceCollection;
		const Reference<BindingSetInstanceCache::Factory> bindingSetInstanceCaches;
		const Reference<GraphicsObjectDescriptorManagerCache> descriptorManagerCache;

		inline PerContextData(
			SceneContext* sceneContext,
			DescriptorPools* pools,
			EndOfUpdateJob* frameEndJob,
			GraphicsObjectDescriptorManagerCleanupJob* cleanup,
			PipelineInstanceCollection* pipelineInstanceCollection)
			: context(sceneContext)
			, descriptorPools(pools)
			, endOfFrameJob(frameEndJob)
			, cleanupJob(cleanup)
			, pipelineInstanceCollection(pipelineInstanceCollection)
			, bindingSetInstanceCaches(Object::Instantiate<BindingSetInstanceCache::Factory>(pools, sceneContext->Log()))
			, descriptorManagerCache(Object::Instantiate<GraphicsObjectDescriptorManagerCache>()) {
			assert(context != nullptr);
			assert(descriptorPools != nullptr);
			assert(endOfFrameJob != nullptr);
			assert(pipelineInstanceCollection != nullptr);
			assert(bindingSetInstanceCaches != nullptr);
			context->Graphics()->RenderJobs().Add(endOfFrameJob);
		}

		virtual inline ~PerContextData() {
			context->Graphics()->RenderJobs().Remove(endOfFrameJob);
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

				const Reference<PipelineInstanceCollection> pipelineInstanceSets = Object::Instantiate<PipelineInstanceCollection>(context);
				const std::shared_ptr<std::atomic<uint32_t>> toggle = std::make_shared<std::atomic<uint32_t>>();

				std::vector<Reference<JobSystem::Job>> pipelineCreationJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					pipelineCreationJobs.push_back(Object::Instantiate<PipelineCreationJob>(pipelineInstanceSets, pools->PoolCount(), i, toggle));
				const Reference<GraphicsObjectDescriptorManagerCleanupJob> cleanupJob =
					Object::Instantiate<GraphicsObjectDescriptorManagerCleanupJob>(pipelineCreationJobs, toggle);

				std::vector<Reference<JobSystem::Job>> updateAndFlushJobs;
				for (size_t i = 0u; i < pools->PoolCount(); i++)
					updateAndFlushJobs.push_back(Object::Instantiate<DescriptorSetUpdateJob>(context->Graphics(), pools->Pool(i), cleanupJob, toggle));
				updateAndFlushJobs.push_back(Object::Instantiate<PipelineCreationFlushJob>(pipelineInstanceSets, cleanupJob, toggle));
				const Reference<EndOfUpdateJob> endOfFrameJob = Object::Instantiate<EndOfUpdateJob>(context, toggle, updateAndFlushJobs);

				return Object::Instantiate<PerContextData>(context, pools, endOfFrameJob, cleanupJob, pipelineInstanceSets);
				});
		}
	};
#pragma endregion




#pragma region CONCRETE_IMPLEMENTATION
#pragma warning(disable: 4250)
	class GraphicsObjectPipelines::Helpers::Instance 
		: public virtual GraphicsObjectPipelines
		, public virtual ObjectCache<Descriptor>::StoredObject {
	public:
		friend class InstanceCache;
		
		struct Data;

		struct DataPtr : public virtual Object {
			SpinLock lock;
			Data* data = nullptr;
		};

		struct Data : public virtual Object {
			const Reference<DataPtr> weakPtr = Object::Instantiate<DataPtr>();
			const Reference<PerContextData> perContextData;
			const Reference<PipelineInstanceSet> pipelines;

			inline Data(PerContextData* contextData, PipelineInstanceSet* pipelineSet) 
				: perContextData(contextData), pipelines(pipelineSet) {
				weakPtr->data = this;
				perContextData->pipelineInstanceCollection->Add(pipelines);
			}

			inline virtual ~Data() {
				assert(weakPtr->data == nullptr);
				perContextData->pipelineInstanceCollection->Remove(pipelines);
			}

			inline virtual void OnOutOfScope()const override {
				const Reference<DataPtr> weak = weakPtr;
				{
					std::unique_lock<SpinLock> lock(weak->lock);
					weak->data = nullptr;
					if (RefCount() > 0u) return;
				}
				Object::OnOutOfScope();
			}
		};

	private:
		const Reference<DataPtr> m_dataPtr;

	public:
		inline static Reference<Data> GetData(const GraphicsObjectPipelines* self) {
			const Instance* instance = dynamic_cast<const Instance*>(self);
			if (instance == nullptr) return nullptr;
			std::unique_lock<SpinLock> lock(instance->m_dataPtr->lock);
			const Reference<Data> data = instance->m_dataPtr->data;
			return data;
		}

		inline Instance(Data* data) 
			: GraphicsObjectPipelines(data->pipelines->PipelineInstances()->EnvironmentPipeline())
			, m_dataPtr(data->weakPtr) {
			data->perContextData->context->StoreDataObject(data);
		}

		inline virtual ~Instance() {
			Reference<Data> data = GetData(this);
			if (data != nullptr)
				data->perContextData->context->EraseDataObject(data);
		}
	};


	class GraphicsObjectPipelines::Helpers::InstanceCache : public virtual ObjectCache<Descriptor> {
	public:
		static Reference<Instance> Get(const Descriptor& desc, bool preinitialize) {
			if (desc.descriptorSet == nullptr) return nullptr;
			auto fail = [&](const auto&... message) {
				desc.descriptorSet->Context()->Log()->Error("GraphicsObjectPipelines::Helpers::InstanceCache::Get - ", message...);
				return nullptr;
			};
			static InstanceCache cache;
			const Reference<Instance> instance = cache.GetCachedOrCreate(desc, false, [&]() -> Reference<Instance> {
				const Reference<PerContextData> contextData = Helpers::PerContextDataCache::Get(desc.descriptorSet->Context());
				if (contextData == nullptr)
					return fail("Failed to retrieve Per-Context Data! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<GraphicsObjectDescriptorManager> descriptors = contextData->descriptorManagerCache
					->Get(desc.descriptorSet, contextData->cleanupJob);
				if (descriptors == nullptr)
					return fail("Failed to retrieve GraphicsObjectDescriptorManager! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<BindingSetInstanceCache> bindingSets = contextData->bindingSetInstanceCaches->Get(
					desc.lightingModel, desc.descriptorSet->Context()->Graphics()->Configuration().ShaderLoader(), desc.renderPass);
				if (bindingSets == nullptr)
					return fail("Failed to create BindingSetInstanceCache! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<PipelineInstanceSet> pipelines = Object::Instantiate<PipelineInstanceSet>(
					descriptors, bindingSets, desc.renderPass, desc.viewportDescriptor, desc.layers);

				const Reference<Instance::Data> data = Object::Instantiate<Instance::Data>(contextData, pipelines);
				return Object::Instantiate<Instance>(data);
				});

			if (instance == nullptr) return nullptr;
			if (preinitialize) {
				Reference<Instance::Data> data = Instance::GetData(instance);
				if (data != nullptr)
					data->pipelines->Preinitialize();
			}
			return instance;
		}
	};
#pragma warning(default: 4250)
#pragma endregion





	Reference<GraphicsObjectPipelines> GraphicsObjectPipelines::Get(const Descriptor& desc, bool preinitialize) {
		return Helpers::InstanceCache::Get(desc, preinitialize);
	}

	GraphicsObjectPipelines::~GraphicsObjectPipelines() {}

	GraphicsObjectPipelines::Reader::Reader(const GraphicsObjectPipelines& pipelines)
		: Reader(Helpers::Instance::GetData(&pipelines)) {}

	GraphicsObjectPipelines::Reader::Reader(const Reference<const Jimara::Object>& data)
		: m_data(data)
		, m_lock([&]() -> std::shared_mutex& {
		const Helpers::Instance::Data* instanceData = dynamic_cast<const Helpers::Instance::Data*>(data.operator->());
		if (instanceData != nullptr) 
			return instanceData->pipelines->InstanceLock();
		static std::shared_mutex defaultLock;
		return defaultLock;
			}()) {
		const Helpers::Instance::Data* instanceData = dynamic_cast<const Helpers::Instance::Data*>(data.operator->());
		if (instanceData != nullptr) {
			m_objectInfos = reinterpret_cast<const void*>(instanceData->pipelines->Instances().Data());
			m_objectInfoCount = instanceData->pipelines->Instances().Size();
		}
	}

	GraphicsObjectPipelines::Reader::~Reader() {}

	size_t GraphicsObjectPipelines::Reader::ObjectCount()const { return m_objectInfoCount; }

	const GraphicsObjectPipelines::ObjectInfo& GraphicsObjectPipelines::Reader::Object(size_t index)const {
		return reinterpret_cast<const Helpers::PipelineInstanceSet::GraphicsObjectData*>(m_objectInfos)[index].info;
	}

	void GraphicsObjectPipelines::GetUpdateTasks(const Callback<JobSystem::Job*> recordUpdateTasks)const {
		const Reference<Helpers::Instance::Data> data = Helpers::Instance::GetData(this);
		if (data != nullptr)
			data->perContextData->endOfFrameJob->GetDependencies(recordUpdateTasks);
	}

	void GraphicsObjectPipelines::ObjectInfo::ExecutePipeline(const Graphics::InFlightBufferInfo& inFlightBuffer)const {
		{
			const Reference<Graphics::BindingSet>* ptr = m_bindingSets.Data();
			const Reference<Graphics::BindingSet>* const end = ptr + m_bindingSets.Size();
			while (ptr < end) {
				(*ptr)->Bind(inFlightBuffer);
				ptr++;
			}
		}
		{
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>>* ptr = m_vertexBufferBindings.Data();
			auto setRefs = [&](size_t count, const auto& getRef) {
				for (size_t i = 0u; i < count; i++) {
					const auto ref = getRef(i);
					(*ptr)->BoundObject() = (ref != nullptr) ? ref->Buffer() : nullptr;
					ptr++;
				}
			};
			setRefs(m_viewportData->VertexBufferCount(), [&](size_t index) { return m_viewportData->VertexBuffer(index); });
			setRefs(m_viewportData->InstanceBufferCount(), [&](size_t index) { return m_viewportData->InstanceBuffer(index); });
			m_indexBufferBinding->BoundObject() = m_viewportData->IndexBuffer();
			m_vertexInput->Bind(inFlightBuffer);
		}
		const Graphics::IndirectDrawBufferReference indirectBuffer = m_viewportData->IndirectBuffer();
		if (indirectBuffer == nullptr)
			m_graphicsPipeline->Draw(inFlightBuffer, m_viewportData->IndexCount(), m_viewportData->InstanceCount());
		else m_graphicsPipeline->DrawIndirect(inFlightBuffer, indirectBuffer, m_viewportData->InstanceCount());
	}


	bool GraphicsObjectPipelines::Descriptor::operator==(const Descriptor& other)const {
		return
			descriptorSet == other.descriptorSet &&
			viewportDescriptor == other.viewportDescriptor &&
			renderPass == other.renderPass &&
			layers == other.layers &&
			lightingModel == other.lightingModel;
	}

	bool GraphicsObjectPipelines::Descriptor::operator<(const Descriptor& other)const { return !((*this) == other); }
}


namespace std {
	size_t hash<Jimara::GraphicsObjectPipelines::Descriptor>::operator()(const Jimara::GraphicsObjectPipelines::Descriptor& descriptor)const {
		return Jimara::MergeHashes(
			Jimara::MergeHashes(
				std::hash<Jimara::GraphicsObjectDescriptor::Set*>()(descriptor.descriptorSet),
				std::hash<const Jimara::ViewportDescriptor*>()(descriptor.viewportDescriptor)),
			Jimara::MergeHashes(
				Jimara::MergeHashes(
					std::hash<Jimara::Graphics::RenderPass*>()(descriptor.renderPass),
					std::hash<Jimara::LayerMask>()(descriptor.layers)),
				std::hash<Jimara::OS::Path>()(descriptor.lightingModel)));
	}
}
