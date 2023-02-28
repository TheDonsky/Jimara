#include "CanvasRenderer.h"
#include "../LightingModelPipelines.h"


namespace Jimara {
	namespace UI {
		struct CanvasRenderer::Helpers {
			class CanvasViewport : public virtual ViewportDescriptor {
			private:
				SpinLock m_canvasLock;
				Reference<const Canvas> m_canvas;
				Matrix4 m_projectionMatrix = Math::Identity();

				void OnCanvasDestroyed(Component*) {
					std::unique_lock<SpinLock> lock(m_canvasLock);
					if (m_canvas == nullptr) return;
					m_canvas->OnDestroyed() += Callback<Component*>(&CanvasViewport::OnCanvasDestroyed, this);
					m_canvas = nullptr;
				}

			public:
				inline CanvasViewport(const Canvas* canvas) : ViewportDescriptor(canvas->Context()), m_canvas(canvas) {
					m_canvas->OnDestroyed() += Callback<Component*>(&CanvasViewport::OnCanvasDestroyed, this);
				}

				virtual ~CanvasViewport() {
					OnCanvasDestroyed(nullptr);
				}

				inline void Update() { 
					std::unique_lock<SpinLock> lock(m_canvasLock);
					const Vector2 resolution = m_canvas == nullptr ? Vector2(1.0f) : m_canvas->Size();
					const float signY = (resolution.y >= 0.0f) ? 1.0f : -1.0f;
					const float aspectRatio = resolution.x / Math::Max(resolution.y * signY, std::numeric_limits<float>::epsilon()) * signY;
					const constexpr float closePlane = 0.0f;
					const constexpr float farPlane = 2.0f;
					m_projectionMatrix = Math::Orthographic(resolution.y, aspectRatio, closePlane, farPlane);
				}

				inline Reference<const Canvas> GetCanvas() {
					std::unique_lock<SpinLock> lock(m_canvasLock);
					const Reference<const Canvas> canvas = m_canvas;
					return canvas;
				}

				inline virtual Matrix4 ViewMatrix()const { 
					return Matrix4(
						Vector4(1.0f, 0.0f, 0.0f, 0.0f),
						Vector4(0.0f, 1.0f, 0.0f, 0.0f),
						Vector4(0.0f, 0.0f, 1.0f, 0.0f),
						Vector4(0.0f, 0.0f, 1.0f, 1.0f));
				}
				inline virtual Matrix4 ProjectionMatrix()const { return m_projectionMatrix; }
				inline virtual Vector4 ClearColor()const { return {}; }
			};


			struct ComponentHierarchyInfo {
				size_t childChainStart = 0u;
				size_t childChainSize = 0u;
			};

			class ComponentPipelines : public virtual JobSystem::Job {
			private:
				const Reference<CanvasViewport> m_canvasViewport;
				const Reference<LightingModelPipelines> m_pipelineObjects;
				const Reference<GraphicsObjectDescriptor::Set> m_canvasObjects;

				Graphics::Texture::PixelFormat m_pixelFormat = RenderImages::MainColor()->Format();
				Graphics::Texture::Multisampling m_sampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_1;
				
				Reference<LightingModelPipelines::Instance> m_objectPipelines;

				std::vector<ComponentHierarchyInfo> m_componentInfos;
				std::vector<size_t> m_childChain;
				std::vector<size_t> m_pipelineOrder;

				inline void OnElementsChanged(GraphicsObjectDescriptor*const*, size_t) {
					m_componentInfos.clear();
					m_childChain.clear();
					m_pipelineOrder.clear();
				}

				inline void ExtractChildStructure() {
					if (m_objectPipelines == nullptr) return;
					LightingModelPipelines::Reader pipelines(m_objectPipelines);

					if (m_pipelineOrder.size() > pipelines.PipelineCount())
						m_pipelineOrder.clear();
					while (m_pipelineOrder.size() < pipelines.PipelineCount())
						m_pipelineOrder.push_back(m_pipelineOrder.size());

					m_childChain.clear();
					m_componentInfos.clear();
					
					const Reference<const Component> canvas = m_canvasViewport->GetCanvas();

					for (size_t i = 0u; i < pipelines.PipelineCount(); i++) {
						const ViewportGraphicsObjectSet::ObjectInfo objectInfo = pipelines.GraphicsObject(i);
						ComponentHierarchyInfo childChain = {};
						childChain.childChainStart = m_childChain.size();
						
						const Component* component = objectInfo.viewportData->GetComponent(0u, 0u);
						
						while (component != nullptr && component != canvas) {
							m_childChain.push_back(component->IndexInParent());
							component = component->Parent();
						}

						if (component == nullptr)
							m_childChain.push_back(~size_t(0u));

						std::reverse(
							m_childChain.data() + childChain.childChainSize,
							m_childChain.data() + m_childChain.size());

						childChain.childChainSize = (m_childChain.size() - childChain.childChainStart);
						m_componentInfos.push_back(childChain);
					}
				}

			public:
				inline ComponentPipelines(CanvasViewport* canvas, LightingModelPipelines* pipelines) 
					: m_canvasViewport(canvas), m_pipelineObjects(pipelines), m_canvasObjects(canvas->GetCanvas()->GraphicsObjects()) {
					m_canvasObjects->OnAdded() += Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsChanged, this);
					m_canvasObjects->OnRemoved() += Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsChanged, this);
					Reference<RenderStack> stack = RenderStack::Main(canvas->Context());
					m_sampleCount = stack->SampleCount();
				}
				inline virtual ~ComponentPipelines() {
					m_canvasObjects->OnRemoved() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsChanged, this);
					m_canvasObjects->OnAdded() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsChanged, this);
				}

				inline GraphicsObjectDescriptor::Set* CanvasObjects() { return m_canvasObjects; }
				inline LightingModelPipelines* ModelPipelines()const { return m_pipelineObjects; }
				inline LightingModelPipelines::Instance* ObjectPipelines()const { return m_objectPipelines; }
				inline const std::vector<size_t>& PipelineOrder()const { return m_pipelineOrder; }

				inline bool UpdatePipelines(Graphics::Texture::PixelFormat pixelFormat, Graphics::Texture::Multisampling sampleCount) {
					if (m_pixelFormat == pixelFormat &&
						m_sampleCount == sampleCount &&
						m_objectPipelines != nullptr) return false;

					m_pixelFormat = pixelFormat;
					m_sampleCount = sampleCount;
					m_objectPipelines = nullptr;

					OnElementsChanged(nullptr, 0u);

					LightingModelPipelines::RenderPassDescriptor renderPassDesc = {};
					{
						renderPassDesc.sampleCount = sampleCount;
						renderPassDesc.colorAttachmentFormats.Push(pixelFormat);
						renderPassDesc.renderPassFlags = Graphics::RenderPass::Flags::RESOLVE_COLOR;
					}

					m_objectPipelines = m_pipelineObjects->GetInstance(renderPassDesc);
					if (m_objectPipelines == nullptr) {
						m_canvasViewport->Context()->Log()->Error(
							"CanvasRenderer::Helpers::ComponentPipelines::UpdatePipelines - Failed to create LightingModelPipelines::Instance!",
							" [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}

					return true;
				}

				inline void Update() {
					const bool pipelinesChanged = UpdatePipelines(m_pixelFormat, m_sampleCount);
					const bool pipelineSetDirty = m_componentInfos.empty();
					if (pipelinesChanged || pipelineSetDirty)
						ExtractChildStructure();
				}

				inline void Sort() {
					const ComponentHierarchyInfo* const infos = m_componentInfos.data();
					const size_t* const childChain = m_childChain.data();
					
					std::sort(m_pipelineOrder.begin(), m_pipelineOrder.end(), [&](const size_t& ai, const size_t& bi) -> bool {
						const ComponentHierarchyInfo a = infos[ai];
						const ComponentHierarchyInfo b = infos[bi];

						const size_t* aPtr = childChain + a.childChainStart;
						const size_t* bPtr = childChain + b.childChainStart;
						
						const size_t* const aEnd = aPtr + a.childChainSize;
						const size_t* const bEnd = bPtr + b.childChainSize;
						
						while ((aPtr < aEnd) && (bPtr < bEnd)) {
							const size_t childIdA = *aPtr;
							const size_t childIdB = *bPtr;

							if (childIdA < childIdB) return true;
							else if (childIdA > childIdB) return false;
							
							aPtr++;
							bPtr++;
						}

						return a.childChainSize > b.childChainSize;
						});
				}

			protected:
				inline virtual void Execute() override { ExtractChildStructure(); }
				inline virtual void CollectDependencies(Callback<Job*>) override {}
			};



			class SortJob : public virtual JobSystem::Job {
			private:
				const Reference<ComponentPipelines> m_pipelines;

			public:
				inline SortJob(CanvasViewport* canvas, ComponentPipelines* pipelines) : m_pipelines(pipelines) {}
				inline virtual ~SortJob() {}

			protected:
				inline virtual void Execute() override { m_pipelines->Sort(); }
				inline virtual void CollectDependencies(Callback<Job*>) override {}
			};



			class Renderer : public virtual RenderStack::Renderer {
			private:
				const Reference<CanvasViewport> m_viewport;
				const Reference<ComponentPipelines> m_pipelines;
				const Reference<SortJob> m_sortJob;

				inline void OnCanvasObjectsFlushed() {
					m_viewport->Update();
					m_pipelines->Update();
				}

			public:
				inline Renderer(CanvasViewport* viewport, ComponentPipelines* pipelines, SortJob* sortJob)
					: m_viewport(viewport), m_pipelines(pipelines), m_sortJob(sortJob) {
					m_pipelines->CanvasObjects()->OnFlushed() += Callback(&Renderer::OnCanvasObjectsFlushed, this);
					m_viewport->Context()->Graphics()->SynchPointJobs().Add(m_pipelines);
				}

				inline virtual ~Renderer() {
					m_pipelines->CanvasObjects()->OnFlushed() -= Callback(&Renderer::OnCanvasObjectsFlushed, this);
					m_viewport->Context()->Graphics()->SynchPointJobs().Remove(m_pipelines);
				}

				inline virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, RenderImages* images) final override {
					// __TODO__: Render the stuff!
				}

				inline virtual void GetDependencies(Callback<JobSystem::Job*> report) { 
					report(m_sortJob);
				}
			};
		};

		Reference<RenderStack::Renderer> CanvasRenderer::CreateFor(const Canvas* canvas) {
			if (canvas == nullptr) return nullptr;
			const Reference<Helpers::CanvasViewport> viewport = Object::Instantiate<Helpers::CanvasViewport>(canvas);
			const Reference<LightingModelPipelines> pipelineObjects = [&]() -> Reference<LightingModelPipelines> {
				LightingModelPipelines::Descriptor desc = {};
				{
					desc.viewport = viewport;
					desc.descriptorSet = canvas->GraphicsObjects();
					desc.layers = LayerMask::All();
					desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/UnlitRendering/Jimara_UnlitRenderer.jlm");
				}
				return LightingModelPipelines::Get(desc);
			}();
			if (pipelineObjects == nullptr) {
				canvas->Context()->Log()->Error("CanvasRenderer::CreateFor - Failed to create lighting model pipelines!");
				return nullptr;
			}

			canvas->Context()->Log()->Error("CanvasRenderer::CreateFor - Not yet implemented!");
			return nullptr;
		}
	}
}
