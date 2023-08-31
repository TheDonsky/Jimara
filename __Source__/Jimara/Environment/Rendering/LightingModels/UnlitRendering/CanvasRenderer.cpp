#include "CanvasRenderer.h"
#include "../GraphicsObjectPipelines.h"
#include "../../Helpers/ImageOverlay/ImageOverlayRenderer.h"


namespace Jimara {
	namespace UI {
		struct CanvasRenderer::Helpers {
			struct ViewportBuffer_t {
				alignas(16) Matrix4 view;
				alignas(16) Matrix4 projection;
				alignas(16) Matrix4 viewPose;
			};

			inline static void UpdateViewportBuffer(const Graphics::BufferReference<ViewportBuffer_t>& viewportBuffer, const ViewportDescriptor* viewport) {
				ViewportBuffer_t& buffer = viewportBuffer.Map();
				buffer.view = viewport->ViewMatrix();
				buffer.projection = viewport->ProjectionMatrix();
				buffer.viewPose = Math::Inverse(buffer.view);
				viewportBuffer->Unmap(true);
			}

			class CanvasViewport : public virtual ViewportDescriptor {
			private:
				SpinLock m_canvasLock;
				Reference<const Canvas> m_canvas;
				Matrix4 m_projectionMatrix = Math::Identity();

				void OnCanvasDestroyed(Component*) {
					const Reference<const Canvas> canvas = [&]() {
						std::unique_lock<SpinLock> lock(m_canvasLock);
						const Reference<const Canvas> canvasPtr = m_canvas;
						m_canvas = nullptr;
						return canvasPtr;
					}();
					if (canvas != nullptr)
						canvas->OnDestroyed() -= Callback<Component*>(&CanvasViewport::OnCanvasDestroyed, this);
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

			struct GraphicsObjectInfo {
				Reference<GraphicsObjectDescriptor> descriptor;
				mutable Reference<const GraphicsObjectDescriptor::ViewportData> data;
				mutable size_t lastPipelineIndex = ~size_t(0u);

				inline GraphicsObjectInfo(GraphicsObjectDescriptor* desc = nullptr) : descriptor(desc) {}
			};

			class ComponentPipelines : public virtual JobSystem::Job {
			private:
				const Reference<CanvasViewport> m_canvasViewport;
				const Reference<GraphicsObjectDescriptor::Set> m_canvasObjects;
				
				std::shared_mutex m_graphicsObjectLock;
				ObjectSet<GraphicsObjectDescriptor, GraphicsObjectInfo> m_graphicsObjects;

				std::vector<ComponentHierarchyInfo> m_componentInfos;
				std::vector<size_t> m_childChain;
				std::vector<size_t> m_pipelineOrder;

				inline void OnElementsChanged(GraphicsObjectDescriptor*const*, size_t) {
					m_componentInfos.clear();
					m_childChain.clear();
					m_pipelineOrder.clear();
				}

				inline void OnElementsAdded(GraphicsObjectDescriptor* const* elements, size_t count) {
					OnElementsChanged(elements, count);
					std::unique_lock<std::shared_mutex> lock(m_graphicsObjectLock);
					static thread_local std::vector<GraphicsObjectDescriptor*> elementsToRemove;
					elementsToRemove.clear();
					m_graphicsObjects.Add(elements, count, [&](const GraphicsObjectInfo* ptr, size_t cnt) {
						const GraphicsObjectInfo* const end = ptr + cnt;
						while (ptr < end) {
							ptr->data = ptr->descriptor->GetViewportData(m_canvasViewport);
							if (ptr->data == nullptr) 
								elementsToRemove.push_back(ptr->descriptor);
							ptr++;
						}
						});
					if (elementsToRemove.size() > 0u) {
						m_graphicsObjects.Remove(elementsToRemove.data(), elementsToRemove.size());
						elementsToRemove.clear();
					}
				}

				inline void OnElementsRemoved(GraphicsObjectDescriptor* const* elements, size_t count) {
					OnElementsChanged(elements, count);
					std::unique_lock<std::shared_mutex> lock(m_graphicsObjectLock);
					m_graphicsObjects.Remove(elements, count);
				}

				inline void AddAllElements() {
					m_canvasObjects->GetAll([&](GraphicsObjectDescriptor* desc) {
						OnElementsAdded(&desc, 1u);
						});
				}

				inline void ExtractChildStructure() {
					std::shared_lock<std::shared_mutex> lock(m_graphicsObjectLock);

					const size_t pipelineCount = m_graphicsObjects.Size();
					if (m_pipelineOrder.size() > pipelineCount)
						m_pipelineOrder.clear();
					while (m_pipelineOrder.size() < pipelineCount)
						m_pipelineOrder.push_back(m_pipelineOrder.size());

					m_childChain.clear();
					m_componentInfos.clear();
					
					const Reference<const Component> canvas = m_canvasViewport->GetCanvas();

					for (size_t i = 0u; i < pipelineCount; i++) {
						const GraphicsObjectDescriptor::ViewportData* viewportData = m_graphicsObjects[i].data;
						ComponentHierarchyInfo childChain = {};
						childChain.childChainStart = m_childChain.size();
						
						const Component* component = viewportData->GetComponent(0u, 0u);
						
						while (component != nullptr && component != canvas) {
							m_childChain.push_back(component->IndexInParent());
							component = component->Parent();
						}

						if (component == nullptr)
							m_childChain.push_back(~size_t(0u));

						std::reverse(
							m_childChain.data() + childChain.childChainStart,
							m_childChain.data() + m_childChain.size());

						childChain.childChainSize = (m_childChain.size() - childChain.childChainStart);
						m_componentInfos.push_back(childChain);
					}
				}

			public:
				inline ComponentPipelines(CanvasViewport* canvas) 
					: m_canvasViewport(canvas)
					, m_canvasObjects(canvas->GetCanvas()->GraphicsObjects()) {
					m_canvasObjects->OnAdded() += Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsAdded, this);
					m_canvasObjects->OnRemoved() += Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsRemoved, this);
					AddAllElements();
				}
				inline virtual ~ComponentPipelines() {
					m_canvasObjects->OnAdded() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsAdded, this);
					m_canvasObjects->OnRemoved() -= Callback<GraphicsObjectDescriptor* const*, size_t>(&ComponentPipelines::OnElementsRemoved, this);
				}

				inline GraphicsObjectDescriptor::Set* CanvasObjects() { return m_canvasObjects; }
				inline std::shared_mutex& GraphicsObjectLock() { return m_graphicsObjectLock; }
				inline const ObjectSet<GraphicsObjectDescriptor, GraphicsObjectInfo>& GraphicsObjects()const { return m_graphicsObjects; }
				inline const std::vector<size_t>& PipelineOrder()const { return m_pipelineOrder; }

				inline void Update() {
					if (m_componentInfos.empty())
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
				inline SortJob(ComponentPipelines* pipelines) : m_pipelines(pipelines) {}
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

				const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;
				Reference<GraphicsObjectPipelines> m_objectPipelines;
				Reference<Graphics::BindingPool> m_bindingPool;
				Stacktor<Reference<Graphics::BindingSet>, 4u> m_environmentBindings;

				Reference<RenderImages> m_lastImages;
				Reference<Graphics::FrameBuffer> m_frameBuffer;
				Reference<ImageOverlayRenderer> m_imageOverlayRenderer;

				inline bool UpdatePipelines(Graphics::Texture::PixelFormat pixelFormat, Graphics::Texture::Multisampling sampleCount) {
					// Check if update is even needed:
					if (m_objectPipelines != nullptr &&
						m_objectPipelines->RenderPass()->ColorAttachmentFormat(0u) == pixelFormat &&
						m_objectPipelines->RenderPass()->SampleCount() == sampleCount) 
						return true;

					// If we fail, some cleanup will be needed:
					auto fail = [&](const auto&... message) {
						m_objectPipelines = nullptr;
						m_environmentBindings.Clear();
						m_viewport->Context()->Log()->Error("CanvasRenderer::Helpers::Renderer::UpdatePipelines - ", message...);
						return false;
					};

					// Get/Create GraphicsObjectPipelines: 
					{
						const Reference<Graphics::RenderPass> renderPass = m_viewport->Context()->Graphics()->Device()->GetRenderPass(
							sampleCount, 1u, &pixelFormat, Graphics::Texture::PixelFormat::OTHER,
							(sampleCount == Graphics::Texture::Multisampling::SAMPLE_COUNT_1) ? Graphics::RenderPass::Flags::NONE : Graphics::RenderPass::Flags::CLEAR_COLOR);
						if (renderPass == nullptr)
							return fail("Failed to create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						GraphicsObjectPipelines::Descriptor desc = {};
						{
							desc.descriptorSet = m_pipelines->CanvasObjects();
							desc.viewportDescriptor = m_viewport;
							desc.renderPass = renderPass;
							desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/UnlitRendering/Jimara_UnlitRenderer.jlm");
						}
						m_objectPipelines = GraphicsObjectPipelines::Get(desc);
						if (m_objectPipelines == nullptr)
							return fail("Failed to get/create graphics object pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					// Make sure descriptor pool exists:
					if (m_bindingPool == nullptr) {
						m_bindingPool = m_viewport->Context()->Graphics()->Device()->CreateBindingPool(
							m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
						if (m_bindingPool == nullptr)
							return fail("Failed to create a binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}

					// Create binding sets:
					if (m_environmentBindings.Size() < m_objectPipelines->EnvironmentPipeline()->BindingSetCount()) {
						Graphics::BindingSet::Descriptor desc = {};
						desc.pipeline = m_objectPipelines->EnvironmentPipeline();

						const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> jimara_BindlessTextures =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
								m_viewport->Context()->Graphics()->Bindless().SamplerBinding());
						auto findBindlessSamplers = [&](const auto&) { return jimara_BindlessTextures; };
						desc.find.bindlessTextureSamplers = &findBindlessSamplers;

						const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> jimara_BindlessBuffers =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
								m_viewport->Context()->Graphics()->Bindless().BufferBinding());
						auto findBindlessBuffers = [&](const auto&) { return jimara_BindlessBuffers; };
						desc.find.bindlessStructuredBuffers = &findBindlessBuffers;

						const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> jimara_LightDataBinding =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(
								m_viewport->Context()->Graphics()->Device()->CreateArrayBuffer(
									m_viewport->Context()->Graphics()->Configuration().ShaderLoader()->PerLightDataSize(), 1));
						auto findStructuredBuffers = [&](const auto&) { return jimara_LightDataBinding; };
						desc.find.structuredBuffer = &findStructuredBuffers;

						const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> jimara_DepthOnlyRenderer_ViewportBuffer =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(m_viewportBuffer);
						auto findConstantBuffers = [&](const auto&) { return jimara_DepthOnlyRenderer_ViewportBuffer; };
						desc.find.constantBuffer = &findConstantBuffers;

						while (m_environmentBindings.Size() < desc.pipeline->BindingSetCount()) {
							desc.bindingSetId = m_environmentBindings.Size();
							const Reference<Graphics::BindingSet> set = m_bindingPool->AllocateBindingSet(desc);
							if (set == nullptr)
								return fail("Failed to allocate environment binding set ", desc.bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							else m_environmentBindings.Push(set);
						}
					}

					return m_objectPipelines != nullptr;
				}

				inline bool UpdateRenderImages(RenderImages* images) {
					if (images == nullptr || images == m_lastImages && m_frameBuffer != nullptr && m_objectPipelines != nullptr)
						return true;
					m_frameBuffer = nullptr;
					m_lastImages = nullptr;

					RenderImages::Image* mainColor = images->GetImage(RenderImages::MainColor());
					Reference<Graphics::TextureView> colorAttachment = mainColor->Multisampled();

					if (mainColor->IsMultisampled()) {
						if (m_imageOverlayRenderer == nullptr)
							m_imageOverlayRenderer = ImageOverlayRenderer::Create(
								m_viewport->Context()->Graphics()->Device(),
								m_viewport->Context()->Graphics()->Configuration().ShaderLoader(),
								m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
						if (m_imageOverlayRenderer == nullptr) {
							m_viewport->Context()->Log()->Error(
								"CanvasRenderer::Helpers::Renderer::UpdateRenderImages - Failed to create image overlay renderer (rendering withous multisampling)!",
								" [File: ", __FILE__, "; Line: ", __LINE__, "]");
							colorAttachment = mainColor->Resolve();
						}
						else {
							const Reference<Graphics::TextureSampler> sampler = colorAttachment->CreateSampler();
							if (sampler == nullptr) {
								m_viewport->Context()->Log()->Error(
									"CanvasRenderer::Helpers::Renderer::UpdateRenderImages - Failed to create target image sampler (rendering withous multisampling)!",
									" [File: ", __FILE__, "; Line: ", __LINE__, "]");
								colorAttachment = mainColor->Resolve();
								m_imageOverlayRenderer = nullptr;
							}
							else {
								m_imageOverlayRenderer->SetSource(sampler);
								m_imageOverlayRenderer->SetTarget(mainColor->Resolve());
							}
						}
					}
					else m_imageOverlayRenderer = nullptr;

					if (!UpdatePipelines(colorAttachment->TargetTexture()->ImageFormat(), colorAttachment->TargetTexture()->SampleCount()))
						return false;

					m_frameBuffer = m_objectPipelines->RenderPass()->CreateFrameBuffer(&colorAttachment, nullptr, nullptr, nullptr);
					if (m_frameBuffer == nullptr) {
						m_viewport->Context()->Log()->Error(
							"CanvasRenderer::Helpers::Renderer::UpdateRenderImages - Failed to create new frame buffer!",
							" [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}

					m_lastImages = images;
					return true;
				}

				inline void OnCanvasObjectsFlushed() {
					m_viewport->Update();
					m_pipelines->Update();
				}

			public:
				inline Renderer(CanvasViewport* viewport, ComponentPipelines* pipelines)
					: m_viewport(viewport), m_pipelines(pipelines)
					, m_sortJob(Object::Instantiate<SortJob>(pipelines))
					, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
					m_pipelines->CanvasObjects()->OnFlushed() += Callback(&Renderer::OnCanvasObjectsFlushed, this);
					m_viewport->Context()->Graphics()->SynchPointJobs().Add(m_pipelines);

					Reference<RenderStack> stack = RenderStack::Main(viewport->Context());
					UpdateRenderImages(stack->Images());
				}

				inline virtual ~Renderer() {
					m_pipelines->CanvasObjects()->OnFlushed() -= Callback(&Renderer::OnCanvasObjectsFlushed, this);
					m_viewport->Context()->Graphics()->SynchPointJobs().Remove(m_pipelines);
				}

				inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
					// Refresh state:
					{
						Reference<GraphicsObjectPipelines> oldPipelines = m_objectPipelines;

						if (!UpdateRenderImages(images))
							return;

						if (oldPipelines != m_objectPipelines)
							return;
					}

					// Verify resolution:
					{
						Size2 size = images->Resolution();
						if (size.x <= 0 || size.y <= 0) return;
					}
					
					// Begin render pass:
					{
						const Vector4 clearColor = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
						m_objectPipelines->RenderPass()->BeginPass(
							commandBufferInfo.commandBuffer, m_frameBuffer, &clearColor, false);
					}

					// Set environment:
					{
						UpdateViewportBuffer(m_viewportBuffer, m_viewport);
						m_bindingPool->UpdateAllBindingSets(commandBufferInfo);
						for (size_t i = 0u; i < m_environmentBindings.Size(); i++)
							m_environmentBindings[i]->Bind(commandBufferInfo);
					}

					// Draw objects:
					{
						std::shared_lock<std::shared_mutex> lock(m_pipelines->GraphicsObjectLock());
						const GraphicsObjectPipelines::Reader pipelines(*m_objectPipelines);
						const ObjectSet<GraphicsObjectDescriptor, GraphicsObjectInfo>& graphicsObjects = m_pipelines->GraphicsObjects();
						const GraphicsObjectInfo* objectInfos = graphicsObjects.Data();
						const size_t objectCount = graphicsObjects.Size();
						const size_t pipelineCount = pipelines.Count();

						// If objectCount < pipelineCount, something has gone wrong...
						if (objectCount < pipelineCount)
							m_viewport->Context()->Log()->Error(
								"Internal Error: CanvasRenderer::Helpers::Renderer::Render - There are more pipelines than graphics objects!",
								" [File: ", __FILE__, "; Line: ", __LINE__, "]");

						// Validate/Fix indices:
						{
							static thread_local std::vector<std::optional<size_t>> objectIndices;
							if (objectIndices.size() < pipelineCount)
								objectIndices.resize(pipelineCount);
							size_t numEntriesFound = 0u;

							for (size_t i = 0u; i < pipelineCount; i++) {
								objectIndices[i] = std::optional<size_t>();
#ifndef NDUBUG
								assert(!objectIndices[i].has_value());
#endif
							}

;							for (size_t objectIndex = 0; objectIndex < objectCount; objectIndex++) {
								const GraphicsObjectInfo& objectInfo = objectInfos[objectIndex];
								size_t pipelineIndex = objectInfo.lastPipelineIndex;
								if (pipelineIndex >= pipelineCount) continue;
								const auto& pipeline = pipelines[pipelineIndex];
								if (pipeline.Descriptor() == objectInfo.descriptor) {
									objectIndices[pipelineIndex] = objectIndex;
									numEntriesFound++;
									if (pipeline.ViewData() != objectInfo.data)
										objectInfo.data = pipeline.ViewData();
								}
								else objectInfo.lastPipelineIndex = ~size_t(0u);
							}
							
							if (numEntriesFound < pipelineCount) 
								for (size_t pipelineIndex = 0u; pipelineIndex < pipelineCount; pipelineIndex++) {
									if (objectIndices[pipelineIndex].has_value()) continue;
									const auto& pipelineData = pipelines[pipelineIndex];
									const GraphicsObjectInfo* objectInfo = graphicsObjects.Find(pipelineData.Descriptor());
									if (objectInfo == nullptr) {
										m_viewport->Context()->Log()->Error(
											"Internal Error: CanvasRenderer::Helpers::Renderer::Render - Failed to find GraphicsObjectInfo for pipeline info!",
											" [File: ", __FILE__, "; Line: ", __LINE__, "]");
										continue;
									}
									objectInfo->lastPipelineIndex = pipelineIndex;
									if (objectInfo->data != pipelineData.ViewData())
										objectInfo->data = pipelineData.ViewData();
								}
						}

						const std::vector<size_t>& order = m_pipelines->PipelineOrder();
#ifndef NDUBUG
						if (order.size() != objectCount)
							m_viewport->Context()->Log()->Error(
								"Internal Error: CanvasRenderer::Helpers::Renderer::Render - Pipeline order size mismatch!",
								" [File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif
						for (size_t i = 0; i < objectCount; i++) {
							const GraphicsObjectInfo* objectInfo = objectInfos + order[i];
							const size_t pipelineIndex = objectInfo->lastPipelineIndex;
							if (pipelineIndex < pipelineCount)
								pipelines[pipelineIndex].ExecutePipeline(commandBufferInfo);
						}
					}

					// End pass:
					m_objectPipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);

					// Draw overlay if needed:
					if (m_imageOverlayRenderer != nullptr)
						m_imageOverlayRenderer->Execute(commandBufferInfo);
				}

				inline virtual void GetDependencies(Callback<JobSystem::Job*> report) { 
					report(m_sortJob);
					if (m_objectPipelines != nullptr)
						m_objectPipelines->GetUpdateTasks(report);
				}
			};
		};

		Reference<RenderStack::Renderer> CanvasRenderer::CreateFor(const Canvas* canvas) {
			if (canvas == nullptr) return nullptr;
			const Reference<Helpers::CanvasViewport> viewport = Object::Instantiate<Helpers::CanvasViewport>(canvas);
			const Reference<Helpers::ComponentPipelines> pipelines = Object::Instantiate<Helpers::ComponentPipelines>(viewport);
			return Object::Instantiate<Helpers::Renderer>(viewport, pipelines);
		}
	}
}
