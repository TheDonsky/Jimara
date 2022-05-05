#include "ViewportObjectQuery.h"
#include "ObjectIdRenderer.h"



namespace Jimara {
	namespace {
		struct ResultReport : public virtual Object {
			Callback<Object*, ViewportObjectQuery::Result> processResult;
			Reference<Object> userData;
			ViewportObjectQuery::Result queryResult;

			inline ResultReport(Object* data, const Callback<Object*, ViewportObjectQuery::Result>& process, const ViewportObjectQuery::Result& result)
				: processResult(process), userData(data), queryResult(result) {}

			inline static void Execute(Object* recordPtr) {
				ResultReport* record = dynamic_cast<ResultReport*>(recordPtr);
				record->queryResult.component =
					(record->queryResult.graphicsObject == nullptr ? nullptr
						: record->queryResult.graphicsObject->GetComponent(record->queryResult.instanceIndex, record->queryResult.primitiveIndex));
				if (record->queryResult.component != nullptr && record->queryResult.component->Destroyed())
					record->queryResult.component = nullptr;
				record->processResult(record->userData, record->queryResult);
			}
		};

		struct SingleRequest {
			Size2 position;
			Reference<Object> userData;
			Callback<Object*, ViewportObjectQuery::Result> processResult;

			inline SingleRequest(Size2 pos, Object* data, Callback<Object*, ViewportObjectQuery::Result> process)
				: position(pos), userData(data), processResult(process) {}
			inline SingleRequest() : SingleRequest(Size2(0, 0), nullptr, Callback(Unused<Object*, ViewportObjectQuery::Result>)) {}

			inline void operator()(Scene::LogicContext* context, const ViewportObjectQuery::Result& result)const {
				Reference<ResultReport> report = Object::Instantiate<ResultReport>(userData, processResult, result);
				context->ExecuteAfterUpdate(Callback<Object*>(&ResultReport::Execute), report);
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


		static Graphics::ShaderClass QUERY_KERNEL_SHADER_CLASS("ViewportObjectQuery_Kernel");

		class Query 
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
		private:
			const Reference<Graphics::Shader> m_shader;
			const Reference<Scene::LogicContext> m_context;
			Graphics::BufferReference<SizeBuffer> m_sizeBuffer;
			Graphics::ArrayBufferReference<Vector2> m_requestBuffer;
			Graphics::ArrayBufferReference<GPU_Result> m_resultBuffer;
			std::vector<SingleRequest> m_requests;
			std::vector<Reference<GraphicsObjectDescriptor>> m_graphicsObjects;
			ObjectIdRenderer::ResultBuffers m_renderResults;

			bool AllocateResults(size_t size) {
				if (size <= 0) size = 1;
				if (m_resultBuffer == nullptr || m_resultBuffer->ObjectCount() <= size) {
					Graphics::ArrayBufferReference<GPU_Result> resultBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<GPU_Result>(
						size, Graphics::ArrayBuffer::CPUAccess::CPU_READ_WRITE);
					if (resultBuffer == nullptr) {
						m_context->Log()->Error("ViewportObjectQuery::Query::Make - Failed to allocate result buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					m_resultBuffer = resultBuffer;
				}
				return true;
			}

			inline bool GetRenderResults(const ObjectIdRenderer::Reader& renderer) {
				m_renderResults = renderer.LastResults();
				if (m_renderResults.vertexPosition == nullptr || m_renderResults.vertexNormal == nullptr ||
					m_renderResults.objectIndex == nullptr || m_renderResults.instanceIndex == nullptr || m_renderResults.primitiveIndex == nullptr) {
					m_context->Log()->Error("ViewportObjectQuery::Query::Make - ObjectIdRenderer did not provide correct buffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				else return true;
			}

			inline void CacheGraphicsObjects(const ObjectIdRenderer::Reader& renderer) {
				m_graphicsObjects.clear();
				for (uint32_t i = 0; i < renderer.DescriptorCount(); i++)
					m_graphicsObjects.push_back(renderer.Descriptor(i));
			}

			inline bool UpdateSizeBuffer(size_t queryCount) {
				if (m_sizeBuffer == nullptr) {
					m_sizeBuffer = m_context->Graphics()->Device()->CreateConstantBuffer<SizeBuffer>();
					if (m_sizeBuffer == nullptr) {
						m_context->Log()->Error("ViewportObjectQuery::Query::Make - Failed to create size buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
				}
				{
					SizeBuffer& buffer = m_sizeBuffer.Map();
					buffer.queryCount = static_cast<uint32_t>(queryCount);
					buffer.objectCount = static_cast<uint32_t>(m_graphicsObjects.size());
					buffer.invalidObjectIndex = ~(uint32_t(0));
					m_sizeBuffer->Unmap(true);
				}
				return true;
			}

			inline bool UpdateQueryBuffer(const std::vector<SingleRequest>& requests) {
				if (m_requestBuffer == nullptr || m_requestBuffer->ObjectCount() <= requests.size()) {
					Graphics::ArrayBufferReference<Vector2> resultBuffer = m_context->Graphics()->Device()->CreateArrayBuffer<Vector2>(
						requests.size() + 1, Graphics::ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
					if (resultBuffer == nullptr) {
						m_context->Log()->Error("ViewportObjectQuery::Query::Make - Failed to allocate query buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					m_requestBuffer = resultBuffer;
				}
				{
					Vector2* positions = m_requestBuffer.Map();
					if (m_renderResults.vertexPosition == nullptr) {
						for (size_t i = 0; i < requests.size(); i++)
							positions[i] = Vector2(2.0f, 2.0f);
					}
					else {
						const Size3 size = m_renderResults.vertexPosition->TargetView()->TargetTexture()->Size();
						const Vector2 sizef(static_cast<float>(size.x), static_cast<float>(size.y));
						for (size_t i = 0; i < requests.size(); i++) {
							const Size2 position = requests[i].position;
							positions[i] = Vector2(static_cast<float>(position.x) / sizef.x, static_cast<float>(position.y) / sizef.y);
						}
					}
					m_requestBuffer->Unmap(true);
				}
				return true;
			}

		public:
			inline Query(Graphics::Shader* shader, Scene::LogicContext* context)
				: m_shader(shader), m_context(context) {}

			inline static Reference<Query> Create(Scene::LogicContext* context) {
				Reference<Graphics::SPIRV_Binary> shaderModule = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("")
					->GetShaderModule(&QUERY_KERNEL_SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
				if (shaderModule == nullptr) {
					context->Log()->Error("ViewportObjectQuery::Query::Create - Failed to read Shader binary!");
					return nullptr;
				}
				Reference<Graphics::Shader> shader = Graphics::ShaderCache::ForDevice(context->Graphics()->Device())->GetShader(shaderModule);
				if (shader == nullptr) {
					context->Log()->Error("ViewportObjectQuery::Query::Create - Failed to create Shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else return Object::Instantiate<Query>(shader, context);
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
				if (m_resultBuffer == nullptr || m_requests.empty()) return;
				const GPU_Result* resultData = m_resultBuffer.Map();
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
						result.graphicsObject = (result.objectIndex >= m_graphicsObjects.size()) ? nullptr : m_graphicsObjects[result.objectIndex];
						result.component = nullptr;
						result.viewportPosition = request.position;
					}
					request(m_context, result);
				}
				m_resultBuffer->Unmap(false);
				m_requests.clear();
				m_graphicsObjects.clear();
			}

			inline bool Empty()const { return m_requests.empty(); }


			/** GraphicsPipeline::Descriptor: */
			inline static constexpr uint32_t NumThreads() { return 256; }
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual Size3 NumBlocks()override { return Size3((m_requests.size() + NumThreads() - 1) / NumThreads(), 1, 1); }

			/** PipelineDescriptor: */
			inline virtual size_t BindingSetCount()const override { return 1; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return this; }

			/** PipelineDescriptor::BindingSetDescriptor: */
			inline virtual bool SetByEnvironment()const override { return false; }

			inline virtual size_t ConstantBufferCount()const override { return 1; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0 }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return m_sizeBuffer; }

			inline virtual size_t StructuredBufferCount()const override { return 2; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) + 6 };
			}
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override {
				return index == 0 ? m_requestBuffer.operator->() : m_resultBuffer.operator->();
			}

			inline virtual size_t TextureSamplerCount()const override { return 5; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) + 1 };
			}
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override {
				return
					(index == 0) ? m_renderResults.vertexPosition :
					(index == 1) ? m_renderResults.vertexNormal :
					(index == 2) ? m_renderResults.objectIndex :
					(index == 3) ? m_renderResults.instanceIndex :
					(index == 4) ? m_renderResults.primitiveIndex : nullptr;
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
				std::vector<std::pair<Reference<Query>, Reference<Graphics::ComputePipeline>>> queries;
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

				for (size_t i = 0; i < data->viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(); i++) {
					Reference<Query> query = Query::Create(view->Context());
					Reference<Graphics::ComputePipeline> pipeline;
					if (query == nullptr)
						pipeline = Reference<Graphics::ComputePipeline>(nullptr);
					else pipeline = data->viewport->Context()->Graphics()->Device()->CreateComputePipeline(query, 1);
					data->queries.push_back(std::make_pair(query, pipeline));
				}
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
					Graphics::Pipeline::CommandBufferInfo commandBuffer = data->viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();
					std::pair<Reference<Query>, Reference<Graphics::ComputePipeline>> queryPipeline = data->queries[commandBuffer.inFlightBufferId];
					Query* query = queryPipeline.first;
					if (query != nullptr) {
						query->Notify();
						std::vector<SingleRequest> requests = data->queryQueue.Swap();
						if (query->Make(data->renderer, requests) && queryPipeline.second != nullptr)
							queryPipeline.second->Execute(Graphics::Pipeline::CommandBufferInfo(commandBuffer.commandBuffer, 0));
						requests.clear();
					}
				}

				// Remove job if no longer needed:
				if (m_retire && data->queryQueue.Empty()) {
					for (size_t i = 0; i < data->queries.size(); i++)
						if (data->queries[i].first != nullptr && (!data->queries[i].first->Empty())) return;
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
			GraphicsLayerMask layerMask;

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
				std::hash<Jimara::GraphicsLayerMask>()(desc.layerMask));
		}
	};
}

namespace Jimara {
	namespace {
		class ViewportObjectQueryCache : public virtual ObjectCache<ViewportObjectQuery_Configuration> {
		public:
			inline static Reference<ViewportObjectQuery> GetFor(
				const ViewportDescriptor* viewport, const GraphicsLayerMask& layers,
				Reference<ViewportObjectQueryCache::StoredObject>(*createFn)(const ViewportDescriptor*, const GraphicsLayerMask&)) {
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

	Reference<ViewportObjectQuery> ViewportObjectQuery::GetFor(const ViewportDescriptor* viewport, GraphicsLayerMask layers) {
		Reference<ViewportObjectQueryCache::StoredObject>(*createFn)(const ViewportDescriptor*, const GraphicsLayerMask&) =
			[](const ViewportDescriptor* view, const GraphicsLayerMask& layers) -> Reference<ViewportObjectQueryCache::StoredObject> {
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
