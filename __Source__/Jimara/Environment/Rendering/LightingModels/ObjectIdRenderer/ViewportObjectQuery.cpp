#include "ViewportObjectQuery.h"
#include "ObjectIdRenderer.h"


namespace Jimara {
	namespace {
		struct ResultReport {
			Callback<Object*, ViewportObjectQuery::Result> processResult;
			Reference<Object> userData;
			ViewportObjectQuery::Result queryResult;

			inline ResultReport(Object* data, const Callback<Object*, ViewportObjectQuery::Result>& process, const ViewportObjectQuery::Result& result)
				: processResult(process), userData(data), queryResult(result) {}

			inline void Report() {
				queryResult.component =
					(queryResult.graphicsObjectData == nullptr ? nullptr
						: queryResult.graphicsObjectData->GetComponent(queryResult.instanceIndex, queryResult.primitiveIndex));
				if (queryResult.component != nullptr && queryResult.component->Destroyed())
					queryResult.component = nullptr;
				processResult(userData, queryResult);
			}
		};

		struct SingleRequest {
			Size2 position;
			Reference<Object> userData;
			Callback<Object*, ViewportObjectQuery::Result> processResult;

			inline SingleRequest(Size2 pos, Object* data, Callback<Object*, ViewportObjectQuery::Result> process)
				: position(pos), userData(data), processResult(process) {}
			inline SingleRequest() : SingleRequest(Size2(0, 0), nullptr, Callback(Unused<Object*, ViewportObjectQuery::Result>)) {}
		};

		struct BatchReport : public virtual Object {
			std::vector<ResultReport> results;

			inline void Add(const SingleRequest& request, const ViewportObjectQuery::Result& result) {
				results.push_back(ResultReport(request.userData, request.processResult, result));
			}

			inline static void Execute(Object* recordPtr) {
				BatchReport* record = dynamic_cast<BatchReport*>(recordPtr);
				for (size_t i = 0; i < record->results.size(); i++)
					record->results[i].Report();
			}
		};

		class QueryQueue {
		private:
			mutable SpinLock m_lock;
			std::vector<SingleRequest> m_backBuffer;
			std::vector<SingleRequest> m_frontBuffer;

		public:
			inline void Add(Size2 position, Callback<Object*, ViewportObjectQuery::Result> processResult, Object* userData) {
				std::unique_lock<SpinLock> lock(m_lock);
				m_backBuffer.push_back(SingleRequest(position, userData, processResult));
			}

			inline std::vector<SingleRequest>& Swap() {
				std::unique_lock<SpinLock> lock(m_lock);
				m_frontBuffer.clear();
				std::swap(m_backBuffer, m_frontBuffer);
				return m_frontBuffer;
			}

			inline bool Empty()const {
				std::unique_lock<SpinLock> lock(m_lock);
				return m_backBuffer.empty() && m_frontBuffer.empty();
			}
		};

		struct GPU_Result {
			alignas(16) Vector3 objectPosition;
			alignas(16) Vector3 objectNormal;
			alignas(4) uint32_t objectIndex;
			alignas(4) uint32_t instanceIndex;
			alignas(4) uint32_t primitiveIndex;
		};

		static_assert(sizeof(uint32_t) == sizeof(float));

		struct SizeBuffer {
			alignas(4) uint32_t queryCount;
			alignas(4) uint32_t objectCount;
			alignas(4) uint32_t invalidObjectIndex;
		};


		static Graphics::ShaderClass QUERY_KERNEL_SHADER_CLASS("Jimara/Environment/Rendering/LightingModels/ObjectIdRenderer/ViewportObjectQuery_Kernel");

		class Query : public virtual Object {
		private:
			const Reference<Scene::LogicContext> m_context;
			const Graphics::BufferReference<SizeBuffer> m_sizeBuffer;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_vertexPositionTex;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_vertexNormalTex;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_objectIndexTex;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_instanceIndexTex;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> m_primitiveIndexTex;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_queryBuffer;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_resultBuffer;
			const Reference<Graphics::BindingSet> m_bindingSet;
			const Reference<Graphics::Experimental::ComputePipeline> m_queryPipeline;
			std::vector<SingleRequest> m_requests;
			std::vector<std::pair<Reference<GraphicsObjectDescriptor>, Reference<const GraphicsObjectDescriptor::ViewportData>>> m_graphicsObjects;

			bool AllocateResults(size_t size) {
				if (size <= 0) size = 1;
				if (m_resultBuffer->BoundObject() == nullptr || m_resultBuffer->BoundObject()->ObjectCount() <= size) {
					Graphics::ArrayBufferReference<GPU_Result> resultBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<GPU_Result>(
						size, Graphics::ArrayBuffer::CPUAccess::CPU_READ_WRITE);
					if (resultBuffer == nullptr) {
						m_context->Log()->Error("ViewportObjectQuery::Query::Make - Failed to allocate result buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					m_resultBuffer->BoundObject() = resultBuffer;
				}
				return true;
			}

			inline bool GetRenderResults(const ObjectIdRenderer::Reader& renderer) {
				const ObjectIdRenderer::ResultBuffers renderResults = renderer.LastResults();
				{
					m_vertexPositionTex->BoundObject() = renderResults.vertexPosition;
					m_vertexNormalTex->BoundObject() = renderResults.vertexNormal;
					m_objectIndexTex->BoundObject() = renderResults.objectIndex;
					m_instanceIndexTex->BoundObject() = renderResults.instanceIndex;
					m_primitiveIndexTex->BoundObject() = renderResults.primitiveIndex;
				}
				if (renderResults.vertexPosition == nullptr || renderResults.vertexNormal == nullptr ||
					renderResults.objectIndex == nullptr || renderResults.instanceIndex == nullptr || renderResults.primitiveIndex == nullptr) {
					m_context->Log()->Error("ViewportObjectQuery::Query::Make - ObjectIdRenderer did not provide correct buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				else return true;
			}

			inline void CacheGraphicsObjects(const ObjectIdRenderer::Reader& renderer) {
				m_graphicsObjects.clear();
				for (uint32_t i = 0; i < renderer.DescriptorCount(); i++) {
					const auto desc = renderer.Descriptor(i);
					m_graphicsObjects.push_back(std::pair<Reference<GraphicsObjectDescriptor>, Reference<const GraphicsObjectDescriptor::ViewportData>>(
						desc.objectDescriptor, desc.viewportData));
				}
			}

			inline bool UpdateSizeBuffer(size_t queryCount) {
				SizeBuffer& buffer = m_sizeBuffer.Map();
				buffer.queryCount = static_cast<uint32_t>(queryCount);
				buffer.objectCount = static_cast<uint32_t>(m_graphicsObjects.size());
				buffer.invalidObjectIndex = ~(uint32_t(0));
				m_sizeBuffer->Unmap(true);
				return true;
			}

			inline bool UpdateQueryBuffer(const std::vector<SingleRequest>& requests) {
				if (m_queryBuffer->BoundObject() == nullptr || m_queryBuffer->BoundObject()->ObjectCount() <= requests.size()) {
					Graphics::ArrayBufferReference<Vector2> resultBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<Vector2>(
						requests.size() + 1, Graphics::ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
					if (resultBuffer == nullptr) {
						m_context->Log()->Error("ViewportObjectQuery::Query::Make - Failed to allocate query buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					m_queryBuffer->BoundObject() = resultBuffer;
				}
				{
					Vector2* positions = reinterpret_cast<Vector2*>(m_queryBuffer->BoundObject()->Map());
					if (m_vertexPositionTex->BoundObject() == nullptr) {
						for (size_t i = 0; i < requests.size(); i++)
							positions[i] = Vector2(2.0f, 2.0f);
					}
					else {
						const Size3 size = m_vertexPositionTex->BoundObject()->TargetView()->TargetTexture()->Size();
						const Vector2 sizef(static_cast<float>(size.x), static_cast<float>(size.y));
						for (size_t i = 0; i < requests.size(); i++) {
							const Size2 position = requests[i].position;
							positions[i] = Vector2(static_cast<float>(position.x) / sizef.x, static_cast<float>(position.y) / sizef.y);
						}
					}
					m_queryBuffer->BoundObject()->Unmap(true);
				}
				return true;
			}

		public:
			inline Query(
				Scene::LogicContext* context,
				Graphics::Buffer* sizeBuffer,
				Graphics::ResourceBinding<Graphics::TextureSampler>* vertexPositionTex,
				Graphics::ResourceBinding<Graphics::TextureSampler>* vertexNormalTex,
				Graphics::ResourceBinding<Graphics::TextureSampler>* objectIndexTex,
				Graphics::ResourceBinding<Graphics::TextureSampler>* instanceIndexTex,
				Graphics::ResourceBinding<Graphics::TextureSampler>* primitiveIndexTex,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* queryBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* resultBuffer,
				Graphics::BindingSet* bindingSet,
				Graphics::Experimental::ComputePipeline* queryPipeline)
				: m_context(context)
				, m_sizeBuffer(sizeBuffer)
				, m_vertexPositionTex(vertexPositionTex)
				, m_vertexNormalTex(vertexNormalTex)
				, m_objectIndexTex(objectIndexTex)
				, m_instanceIndexTex(instanceIndexTex)
				, m_primitiveIndexTex(primitiveIndexTex)
				, m_queryBuffer(queryBuffer)
				, m_resultBuffer(resultBuffer)
				, m_bindingSet(bindingSet)
				, m_queryPipeline(queryPipeline) {}

			inline static Reference<Query> Create(
				Scene::LogicContext* context,
				Graphics::BindingPool* bindingPool,
				Graphics::Experimental::ComputePipeline* pipeline) {
				auto fail = [&](const auto&... message) {
					context->Log()->Error("ViewportObjectQuery::Query::Create - ", message...);
					return nullptr;
				};

				if (bindingPool == nullptr)
					return fail("Binding pool not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Graphics::BufferReference<SizeBuffer> sizeBuffer = context->Graphics()->Device()->CreateConstantBuffer<SizeBuffer>();
				if (sizeBuffer == nullptr)
					return fail("Failed to create size buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> sizeBufferBinding =
					Object::Instantiate<const Graphics::ResourceBinding<Graphics::Buffer>>(sizeBuffer);
				auto getConstantBuffer = [&](const auto& desc) { return sizeBufferBinding; };

				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> vertexPositionTex =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> vertexNormalTex =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> objectIndexTex =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> instanceIndexTex =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> primitiveIndexTex =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				auto getTextureSampler = [&](const Graphics::BindingSet::BindingDescriptor& desc) {
					return
						(desc.bindingName == "vertexPositionTex") ? vertexPositionTex :
						(desc.bindingName == "vertexNormalTex") ? vertexNormalTex :
						(desc.bindingName == "objectIndexTex") ? objectIndexTex :
						(desc.bindingName == "instanceIndexTex") ? instanceIndexTex :
						(desc.bindingName == "primitiveIndexTex") ? primitiveIndexTex : nullptr;
				};

				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> queryBuffer =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> resultBuffer =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				auto getStructuredBuffer = [&](const Graphics::BindingSet::BindingDescriptor& desc) {
					return
						(desc.bindingName == "queryBuffer") ? queryBuffer :
						(desc.bindingName == "resultBuffer") ? resultBuffer : nullptr;
				};

				Graphics::BindingSet::Descriptor setDescriptor = {};
				{
					setDescriptor.pipeline = pipeline;
					setDescriptor.bindingSetId = 0u;
					setDescriptor.find.constantBuffer = &getConstantBuffer;
					setDescriptor.find.textureSampler = &getTextureSampler;
					setDescriptor.find.structuredBuffer = &getStructuredBuffer;
				}
				const Reference<Graphics::BindingSet> bindingSet = bindingPool->AllocateBindingSet(setDescriptor);
				if (bindingSet == nullptr)
					return fail("Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				return Object::Instantiate<Query>(
					context, sizeBuffer,
					vertexPositionTex, vertexNormalTex, objectIndexTex, instanceIndexTex, primitiveIndexTex,
					queryBuffer, resultBuffer, bindingSet, pipeline);
			}

			inline bool Make(ObjectIdRenderer* renderer, const std::vector<SingleRequest>& requests) {
				m_requests.clear();
				m_graphicsObjects.clear();
				if (!AllocateResults(requests.size())) return false;
				ObjectIdRenderer::Reader reader(renderer);
				if (!GetRenderResults(reader)) return false;
				CacheGraphicsObjects(reader);
				if (!UpdateSizeBuffer(requests.size())) return false;
				if (!UpdateQueryBuffer(requests)) return false;
				m_requests = requests;
				return true;
			}

			inline void Notify() {
				if (m_resultBuffer->BoundObject() == nullptr || m_requests.empty()) return;
				Reference<BatchReport> batchReport = Object::Instantiate<BatchReport>();
				const GPU_Result* resultData = reinterpret_cast<const GPU_Result*>(m_resultBuffer->BoundObject()->Map());
				for (size_t i = 0; i < m_requests.size(); i++) {
					const SingleRequest& request = m_requests[i];
					ViewportObjectQuery::Result result = {};
					{
						const GPU_Result& data = resultData[i];
						result.objectPosition = data.objectPosition;
						result.objectNormal = data.objectNormal;
						result.objectIndex = data.objectIndex;
						result.instanceIndex = data.instanceIndex;
						result.primitiveIndex = data.primitiveIndex;
					}
					{
						if (result.objectIndex < m_graphicsObjects.size()) {
							const auto& pair = m_graphicsObjects[result.objectIndex];
							result.graphicsObject = pair.first.operator->();
							result.graphicsObjectData = pair.second.operator->();
						}
						result.component = nullptr;
						result.viewportPosition = request.position;
					}
					batchReport->Add(request, result);
				}
				m_resultBuffer->BoundObject()->Unmap(false);
				if (batchReport->results.size() > 0)
					m_context->ExecuteAfterUpdate(Callback<Object*>(&BatchReport::Execute), batchReport);
				m_requests.clear();
				m_graphicsObjects.clear();
			}

			inline bool Empty()const { return m_requests.empty(); }

			inline void Execute(Graphics::CommandBuffer* commandBuffer) {
				m_bindingSet->Update(0u);
				m_bindingSet->Bind(Graphics::InFlightBufferInfo(commandBuffer, 0u));
				static const constexpr uint32_t NUM_THREADS = 256;
				m_queryPipeline->Dispatch(commandBuffer, Size3((m_requests.size() + NUM_THREADS - 1) / NUM_THREADS, 1, 1));
			}
		};

		class ViewportObjectQueryJob : public virtual JobSystem::Job {
		private:
			const Reference<Scene::GraphicsContext> m_graphicsContext;
			std::atomic<bool> m_retire = false;

			struct Data : public virtual Object {
				Reference<ObjectIdRenderer> renderer;
				Reference<const ViewportDescriptor> viewport;
				QueryQueue queryQueue;
				Reference<Graphics::BindingPool> bindingPool;
				Reference<Graphics::Experimental::ComputePipeline> queryPipeline;
				std::vector<Reference<Query>> inFlightQueries;
				Reference<ViewportObjectQueryJob> owner;

				inline virtual void OnOutOfScope()const override {
					{
						std::unique_lock<SpinLock> lock(owner->m_dataLock);
						if (RefCount() > 0) return;
						owner->m_data = nullptr;
					}
					Object::OnOutOfScope();
				}
			};
			
			SpinLock m_dataLock;
			Data* m_data;

			Reference<Data> GetData() {
				std::unique_lock<SpinLock> dataLock(m_dataLock);
				Reference<Data> data = m_data;
				return data;
			}

		public:
			inline ViewportObjectQueryJob(ObjectIdRenderer* renderer, const ViewportDescriptor* view) 
				: m_graphicsContext(view->Context()->Graphics()) {
				Reference<Data> data = Object::Instantiate<Data>();
				m_data = data;
				data->renderer = renderer;
				data->viewport = view;
				data->owner = this;

				auto fail = [&](const auto&... message) {
					data->viewport->Context()->Log()->Error("ViewportObjectQueryJob::ViewportObjectQueryJob - ", message...);
				};
				data->bindingPool = data->viewport->Context()->Graphics()->Device()->CreateBindingPool(1u);
				if (data->bindingPool == nullptr)
					fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::ShaderSet> shaderSet = data->viewport->Context()->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
				if (shaderSet == nullptr)
					fail("Failed to load quey shader module! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else {
					const Reference<Graphics::SPIRV_Binary> shader =
						shaderSet->GetShaderModule(&QUERY_KERNEL_SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
					if (shader == nullptr)
						fail("Failed to read Shader binary! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					else data->queryPipeline = data->viewport->Context()->Graphics()->Device()->GetComputePipeline(shader);
				}
				if (data->queryPipeline == nullptr)
					fail("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else if (data->queryPipeline->BindingSetCount() != 1u)
					fail("Pipeline binding set count expected to be exactly 1! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else for (size_t i = 0; i < data->viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(); i++)
					data->inFlightQueries.push_back(Query::Create(view->Context(), data->bindingPool, data->queryPipeline));

				m_graphicsContext->RenderJobs().Add(this);
				view->Context()->StoreDataObject(m_data);
			}

			inline virtual ~ViewportObjectQueryJob() {
				Reference<Data> data = GetData();
				if (data != nullptr)
					data->viewport->Context()->EraseDataObject(m_data);
			}

			inline void Schedule(Size2 position, Callback<Object*, ViewportObjectQuery::Result> processResult, Object* userData) {
				Reference<Data> data = GetData();
				if (data != nullptr)
					data->queryQueue.Add(position, processResult, userData);
			}

			inline void Retire() { m_retire = true; }

		protected:
			inline virtual void Execute() {
				Reference<ViewportObjectQueryJob> self(this);
				Reference<Data> data = GetData();
				if (data == nullptr) {
					m_graphicsContext->RenderJobs().Remove(this);
					return;
				}

				// Notify and refresh the query:
				{
					Graphics::InFlightBufferInfo commandBuffer = data->viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();
					Query* query = data->inFlightQueries[commandBuffer.inFlightBufferId];

					if (query != nullptr) {
						query->Notify();
						std::vector<SingleRequest> requests = data->queryQueue.Swap();
						if (query->Make(data->renderer, requests))
							query->Execute(commandBuffer.commandBuffer);
						requests.clear();
					}
				}

				// Remove job if no longer needed:
				if (m_retire && data->queryQueue.Empty()) {
					for (size_t i = 0; i < data->inFlightQueries.size(); i++)
						if (data->inFlightQueries[i] != nullptr && (!data->inFlightQueries[i]->Empty())) return;
					data->viewport->Context()->EraseDataObject(m_data);
					m_graphicsContext->RenderJobs().Remove(this);
				}
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) { 
				Reference<Data> data = GetData();
				if (data != nullptr)
					addDependency(data->renderer);
			}
		};

		struct ViewportObjectQuery_Configuration {
			Reference<const ViewportDescriptor> descriptor;
			LayerMask layerMask;

			inline bool operator==(const ViewportObjectQuery_Configuration& other)const {
				return descriptor == other.descriptor && layerMask == other.layerMask;
			}

			inline bool operator<(const ViewportObjectQuery_Configuration& other)const {
				return descriptor < other.descriptor || (descriptor == other.descriptor && layerMask < other.layerMask);
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::ViewportObjectQuery_Configuration> {
		size_t operator()(const Jimara::ViewportObjectQuery_Configuration& desc)const {
			return Jimara::MergeHashes(
				std::hash<const Jimara::ViewportDescriptor*>()(desc.descriptor),
				std::hash<Jimara::LayerMask>()(desc.layerMask));
		}
	};
}

namespace Jimara {
	namespace {
		class ViewportObjectQueryCache : public virtual ObjectCache<ViewportObjectQuery_Configuration> {
		public:
			inline static Reference<ViewportObjectQuery> GetFor(
				const ViewportDescriptor* viewport, const LayerMask& layers,
				Reference<ViewportObjectQueryCache::StoredObject>(*createFn)(const ViewportDescriptor*, const LayerMask&)) {
				if (viewport == nullptr) return nullptr;
				static ViewportObjectQueryCache cache;
				ViewportObjectQuery_Configuration config;
				config.descriptor = viewport;
				config.layerMask = layers;
				return cache.GetCachedOrCreate(config, false, [&]() { return createFn(viewport, layers); });
			}
		};
	}

#pragma warning(disable: 4250)
	class ViewportObjectQuery::Cached : public ViewportObjectQuery, public virtual ViewportObjectQueryCache::StoredObject {
	public:
		inline Cached(JobSystem::Job* job) : ViewportObjectQuery(job) { }
	};
#pragma warning(default: 4250)

	Reference<ViewportObjectQuery> ViewportObjectQuery::GetFor(const ViewportDescriptor* viewport, LayerMask layers) {
		Reference<ViewportObjectQueryCache::StoredObject>(*createFn)(const ViewportDescriptor*, const LayerMask&) =
			[](const ViewportDescriptor* view, const LayerMask& layers) -> Reference<ViewportObjectQueryCache::StoredObject> {
			if (view == nullptr) return nullptr;
			Reference<ObjectIdRenderer> renderer = ObjectIdRenderer::GetFor(view, layers);
			if (renderer == nullptr) {
				view->Context()->Log()->Error("ViewportObjectQuery::GetFor - Failed to get ObjectIdRenderer!");
				return nullptr;
			}
			Reference<ViewportObjectQueryJob> job = Object::Instantiate<ViewportObjectQueryJob>(renderer, view);
			Reference<ViewportObjectQuery> query = new Cached(job);
			query->ReleaseRef();
			return query;
		};
		return ViewportObjectQueryCache::GetFor(viewport, layers, createFn);
	}

	ViewportObjectQuery::ViewportObjectQuery(JobSystem::Job* job) : m_job(job) {}

	ViewportObjectQuery::~ViewportObjectQuery() {
		dynamic_cast<ViewportObjectQueryJob*>(m_job.operator->())->Retire();
	}

	void ViewportObjectQuery::QueryAsynch(Size2 position, Callback<Object*, Result> processResult, Object* userData) {
		dynamic_cast<ViewportObjectQueryJob*>(m_job.operator->())->Schedule(position, processResult, userData);
	}
}
