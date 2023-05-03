#include "ForwardLightingModel.h"
#include "../GraphicsObjectPipelines.h"
#include "../../SceneObjects/Lights/LightmapperJobs.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"
#include "../../SceneObjects/Lights/SceneLightGrid.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	namespace {
		/** FORWARD RENDERER: */
		class ForwardRenderer : public virtual RenderStack::Renderer {
		private:
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<SceneLightGrid> m_lightGrid;
			const Reference<GraphicsSimulation::JobDependencies> m_graphicsSimulation;
			const Reference<const LightmapperJobs> m_lightmapperJobs;
			const LayerMask m_layerMask;
			const Graphics::RenderPass::Flags m_clearAndResolveFlags;

			struct Bindings {
				struct ViewportBuffer_t {
					alignas(16) Matrix4 view;
					alignas(16) Matrix4 projection;
				};
				const Reference<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> jimara_BindlessTextures =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>();
				const Reference<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> jimara_BindlessBuffers =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>();
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> jimara_LightDataBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> jimara_ForwardRenderer_LightTypeIds =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Reference<Graphics::ResourceBinding<Graphics::Buffer>> jimara_ForwardRenderer_ViewportBuffer =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>();

				const Reference<LightDataBuffer> m_lightDataBuffer;
				const Reference<LightTypeIdBuffer> m_lightTypeIdBuffer;
				const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

				inline Bindings(const ViewportDescriptor* viewport)
					: m_lightDataBuffer(LightDataBuffer::Instance(viewport))
					, m_lightTypeIdBuffer(LightTypeIdBuffer::Instance(viewport))
					, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
					if (m_viewportBuffer == nullptr) viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
					jimara_BindlessTextures->BoundObject() = viewport->Context()->Graphics()->Bindless().SamplerBinding();
					jimara_BindlessBuffers->BoundObject() = viewport->Context()->Graphics()->Bindless().BufferBinding();
					jimara_ForwardRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
				}

				inline void Update(const ViewportDescriptor* viewport) {
					jimara_LightDataBinding->BoundObject() = m_lightDataBuffer->Buffer();
					jimara_ForwardRenderer_LightTypeIds->BoundObject() = m_lightTypeIdBuffer->Buffer();
					ViewportBuffer_t& buffer = m_viewportBuffer.Map();
					buffer.view = viewport->ViewMatrix();
					buffer.projection = viewport->ProjectionMatrix();
					m_viewportBuffer->Unmap(true);
				}

				inline void GetDependencies(Callback<JobSystem::Job*> report) {
					report(m_lightDataBuffer);
					report(m_lightTypeIdBuffer);
				}
			} m_bindings;

			Reference<Graphics::BindingPool> m_bindingPool;
			Reference<GraphicsObjectPipelines> m_graphicsObjectPipelines;
			Stacktor<Reference<Graphics::BindingSet>, 4u> m_environmentBindingSets;

			
			struct {
				Graphics::Texture::PixelFormat pixelFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::MAX_AVAILABLE;
			} m_renderPass;


			struct {
				Reference<RenderImages> renderImages;
				Reference<Graphics::FrameBuffer> frameBuffer;
			} m_lastFrameBuffer;

			inline bool RefreshRenderPass(
				Graphics::Texture::PixelFormat pixelFormat, 
				Graphics::Texture::PixelFormat depthFormat,
				Graphics::Texture::Multisampling sampleCount) {
				
				if (m_graphicsObjectPipelines != nullptr &&				
					m_renderPass.pixelFormat == pixelFormat && 
					m_renderPass.sampleCount == sampleCount) return true;

				auto fail = [&](const auto&... message) {
					m_graphicsObjectPipelines = nullptr;
					m_environmentBindingSets.Clear();
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - ", message...);
					return false;
				};

				// Make sure we have a binding pool:
				if (m_bindingPool == nullptr) {
					m_bindingPool = m_viewport->Context()->Graphics()->Device()->CreateBindingPool(
						m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
					if (m_bindingPool == nullptr)
						return fail("Failed to create a binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Get GraphicsObjectPipelines: 
				{
					const Reference<Graphics::RenderPass> renderPass = m_viewport->Context()->Graphics()->Device()->GetRenderPass(
						sampleCount, 1u, &pixelFormat, depthFormat, m_clearAndResolveFlags);
					if (renderPass == nullptr)
						return fail("Failed to create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					GraphicsObjectPipelines::Descriptor desc = {};
					{
						desc.descriptorSet = GraphicsObjectDescriptor::Set::GetInstance(m_viewport->Context());
						desc.viewportDescriptor = m_viewport;
						desc.renderPass = renderPass;
						desc.layers = m_layerMask;
						desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm");
					}
					m_graphicsObjectPipelines = GraphicsObjectPipelines::Get(desc);
					if (m_graphicsObjectPipelines == nullptr)
						return fail("Failed to create/get GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Create binding sets:
				{
					m_environmentBindingSets.Clear();
					const Graphics::BindingSet::BindingSearchFunctions lightGridSearchFunctions = m_lightGrid->BindingDescriptor();

					Graphics::BindingSet::Descriptor desc = {};
					desc.find = lightGridSearchFunctions;
					
					desc.pipeline = m_graphicsObjectPipelines->EnvironmentPipeline();

					auto findConstantBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::Buffer>> {
						return (info.name == "jimara_ForwardRenderer_ViewportBuffer")
							? m_bindings.jimara_ForwardRenderer_ViewportBuffer.operator->()
							: lightGridSearchFunctions.constantBuffer(info);
					};
					desc.find.constantBuffer = &findConstantBuffer;

					auto findStructuredBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
						return
							(info.name == "jimara_LightDataBinding") ? m_bindings.jimara_LightDataBinding.operator->() :
							(info.name == "jimara_ForwardRenderer_LightTypeIds") ? m_bindings.jimara_ForwardRenderer_LightTypeIds.operator->() :
							lightGridSearchFunctions.structuredBuffer(info);
					};
					desc.find.structuredBuffer = &findStructuredBuffer;

					auto findBindlessArrays = [&](const auto&) { return m_bindings.jimara_BindlessBuffers; };
					desc.find.bindlessStructuredBuffers = &findBindlessArrays;

					auto findBindlessTextures = [&](const auto&) { return m_bindings.jimara_BindlessTextures; };
					desc.find.bindlessTextureSamplers = &findBindlessTextures;

					for (size_t i = 0u; i < desc.pipeline->BindingSetCount(); i++) {
						desc.bindingSetId = i;
						const Reference<Graphics::BindingSet> bindingSet = m_bindingPool->AllocateBindingSet(desc);
						if (bindingSet == nullptr)
							return fail("Failed to allocate binding set ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_environmentBindingSets.Push(bindingSet);
					}
				}

				m_renderPass.pixelFormat = pixelFormat;
				m_renderPass.depthFormat = depthFormat;
				m_renderPass.sampleCount = sampleCount;

				return true;
			}
			
			inline Reference<Graphics::FrameBuffer> RefreshFrameBuffer(RenderImages* images) {
				if (m_lastFrameBuffer.renderImages == images)
					return m_lastFrameBuffer.frameBuffer;

				RenderImages::Image* mainColor = images->GetImage(RenderImages::MainColor());
				RenderImages::Image* depthBuffer = images->GetImage(RenderImages::DepthBuffer());

				Reference<Graphics::TextureView> depthAttachment = depthBuffer->Multisampled();
				Reference<Graphics::TextureView> depthResolve = depthBuffer->Resolve();
				Reference<Graphics::TextureView> colorAttachment = mainColor->Multisampled();
				Reference<Graphics::TextureView> resolveAttachment = mainColor->Resolve();

				if (!RefreshRenderPass(
					colorAttachment->TargetTexture()->ImageFormat(),
					depthAttachment->TargetTexture()->ImageFormat(),
					images->SampleCount())) return nullptr;

				m_lastFrameBuffer.frameBuffer = m_graphicsObjectPipelines->RenderPass()
					->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveAttachment, depthResolve);
				if (m_lastFrameBuffer.frameBuffer == nullptr)
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - Failed to create the frame buffer!");
				else m_lastFrameBuffer.renderImages = images;

				return m_lastFrameBuffer.frameBuffer;
			}

		public:
			inline ForwardRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)
				: m_viewport(viewport)
				, m_lightGrid(SceneLightGrid::GetFor(viewport))
				, m_graphicsSimulation(GraphicsSimulation::JobDependencies::For(viewport->Context()))
				, m_lightmapperJobs(LightmapperJobs::GetInstance(viewport->Context()))
				, m_layerMask(layers)
				, m_clearAndResolveFlags(flags)
				, m_bindings(viewport) {}

			inline virtual ~ForwardRenderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				if (images == nullptr) return;

				// Get frame buffer:
				Reference<Graphics::FrameBuffer> oldFrameBufer = m_lastFrameBuffer.frameBuffer;
				Reference<Graphics::FrameBuffer> frameBuffer = RefreshFrameBuffer(images);
				if (frameBuffer == nullptr || m_graphicsObjectPipelines == nullptr || oldFrameBufer != frameBuffer) 
					return;

				// Verify resolution:
				{
					Size2 size = images->Resolution();
					if (size.x <= 0 || size.y <= 0) return;
				}

				// Begin render pass:
				{
					const Vector4 clearColor = m_viewport->ClearColor();
					const Vector4* CLEAR_VALUE = &clearColor;
					m_graphicsObjectPipelines->RenderPass()->BeginPass(commandBufferInfo.commandBuffer, frameBuffer, CLEAR_VALUE, false);
				}

				// Set environment:
				{
					m_bindings.Update(m_viewport);
					m_bindingPool->UpdateAllBindingSets(commandBufferInfo);
					for (size_t i = 0u; i < m_environmentBindingSets.Size(); i++)
						m_environmentBindingSets[i]->Bind(commandBufferInfo);
				}

				// Draw objects:
				{
					// __TODO__: we need some sorting here, as well as separate opaque and transparent treatment...
					const GraphicsObjectPipelines::Reader reader(*m_graphicsObjectPipelines);
					for (size_t i = 0; i < reader.Count(); i++)
						reader[i].ExecutePipeline(commandBufferInfo);
				}

				// End pass:
				m_graphicsObjectPipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				if (m_graphicsObjectPipelines != nullptr)
					m_graphicsObjectPipelines->GetUpdateTasks(report);
				m_bindings.GetDependencies(report);
				m_lightmapperJobs->GetAll(report);
				report(m_lightGrid->UpdateJob());
				m_graphicsSimulation->CollectDependencies(report);
			}
		};
	}

	Reference<ForwardLightingModel> ForwardLightingModel::Instance() {
		static ForwardLightingModel model;
		return &model;
	}

	Reference<RenderStack::Renderer> ForwardLightingModel::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags) {
		if (viewport == nullptr) return nullptr;
		else return Object::Instantiate<ForwardRenderer>(viewport, layers, flags);
	}
}
