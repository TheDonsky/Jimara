#include "ForwardLightingModel_Opaque_Pass.h"
#include "../GraphicsObjectPipelines.h"
#include "../../SceneObjects/Lights/LightmapperJobs.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"
#include "../../SceneObjects/Lights/SceneLightGrid.h"
#include "../../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	struct ForwardLightingModel_Opaque_Pass::Helpers {
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
					alignas(16) Matrix4 viewPose;
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
					buffer.viewPose = Math::Inverse(buffer.view);
					m_viewportBuffer->Unmap(true);
				}

				inline void GetDependencies(Callback<JobSystem::Job*> report) {
					report(m_lightDataBuffer);
					report(m_lightTypeIdBuffer);
				}
			} m_bindings;

			Reference<Graphics::BindingPool> m_bindingPool;
			Reference<GraphicsObjectPipelines> m_depthOnlyPrePassPipelines;
			Reference<GraphicsObjectPipelines> m_graphicsObjectPipelines;
			Stacktor<Reference<Graphics::BindingSet>, 4u> m_environmentBindingSets;
			Stacktor<Reference<Graphics::BindingSet>, 4u> m_depthOnlyEnvironmentBindingSets;

			
			struct {
				Graphics::Texture::PixelFormat pixelFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::MAX_AVAILABLE;
			} m_renderPass;


			struct {
				Reference<RenderImages> renderImages;
				Reference<Graphics::FrameBuffer> frameBuffer;
				Reference<Graphics::FrameBuffer> depthOnlyFrameBuffer;
			} m_lastFrameBuffer;

			inline bool RefreshRenderPass(
				Graphics::Texture::PixelFormat pixelFormat,
				Graphics::Texture::PixelFormat depthFormat,
				Graphics::Texture::Multisampling sampleCount) {

				if (m_graphicsObjectPipelines != nullptr &&
					m_renderPass.pixelFormat == pixelFormat &&
					m_renderPass.sampleCount == sampleCount)
					return true;

				auto fail = [&](const auto&... message) {
					m_depthOnlyPrePassPipelines = nullptr;
					m_graphicsObjectPipelines = nullptr;
					m_environmentBindingSets.Clear();
					m_depthOnlyEnvironmentBindingSets.Clear();
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
				auto getPipelines = [&](
					const OS::Path& lightingModelPass,
					Graphics::RenderPass::Flags clearAndResolveFlags,
					Graphics::GraphicsPipeline::Flags pipelineFlags,
					uint32_t colorAttachmentCount) 
					-> Reference<GraphicsObjectPipelines> {
					const Reference<Graphics::RenderPass> renderPass = m_viewport->Context()->Graphics()->Device()->GetRenderPass(
						sampleCount, colorAttachmentCount, &pixelFormat, depthFormat, clearAndResolveFlags);
					if (renderPass == nullptr) {
						fail("Failed to create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}

					GraphicsObjectPipelines::Descriptor desc = {};
					{
						desc.descriptorSet = GraphicsObjectDescriptor::Set::GetInstance(m_viewport->Context());
						desc.viewportDescriptor = m_viewport;
						desc.renderPass = renderPass;
						desc.flags = GraphicsObjectPipelines::Flags::EXCLUDE_NON_OPAQUE_OBJECTS;
						desc.pipelineFlags = pipelineFlags;
						desc.layers = m_layerMask;
						desc.lightingModel = lightingModelPass;
					}
					Reference<GraphicsObjectPipelines> graphicsObjectPipelines = GraphicsObjectPipelines::Get(desc);
					if (graphicsObjectPipelines == nullptr)
						fail("Failed to create/get GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return graphicsObjectPipelines;
					};

				m_graphicsObjectPipelines = getPipelines(
					OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm"),
					m_clearAndResolveFlags & (~(Graphics::RenderPass::Flags::CLEAR_DEPTH)), Graphics::GraphicsPipeline::Flags::NONE, 1u);
				m_depthOnlyPrePassPipelines = getPipelines(
					OS::Path("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DepthOnlyRenderer.jlm"),
					m_clearAndResolveFlags & (~(Graphics::RenderPass::Flags::RESOLVE_DEPTH)), Graphics::GraphicsPipeline::Flags::WRITE_DEPTH, 0u);
				if (m_graphicsObjectPipelines == nullptr || m_depthOnlyPrePassPipelines == nullptr)
					return false;

				// Create binding sets:
				auto allocateBindingSets = [&](Stacktor<Reference<Graphics::BindingSet>, 4u>& bindingSets, GraphicsObjectPipelines* pipelines) {
					bindingSets.Clear();
					const Graphics::BindingSet::BindingSearchFunctions lightGridSearchFunctions = m_lightGrid->BindingDescriptor();

					Graphics::BindingSet::Descriptor desc = {};
					desc.find = lightGridSearchFunctions;

					desc.pipeline = pipelines->EnvironmentPipeline();

					auto findConstantBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::Buffer>> {
						return (
							info.name == "jimara_ForwardRenderer_ViewportBuffer" ||
							info.name == "jimara_DepthOnlyRenderer_ViewportBuffer")
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
						bindingSets.Push(bindingSet);
					}

					return true;
					};
				if ((!allocateBindingSets(m_environmentBindingSets, m_graphicsObjectPipelines)) ||
					(!allocateBindingSets(m_depthOnlyEnvironmentBindingSets, m_depthOnlyPrePassPipelines)))
					return false;

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
					images->SampleCount())) 
					return nullptr;

				m_lastFrameBuffer.frameBuffer = m_graphicsObjectPipelines->RenderPass()
					->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveAttachment, depthResolve);
				m_lastFrameBuffer.depthOnlyFrameBuffer = m_depthOnlyPrePassPipelines->RenderPass()
					->CreateFrameBuffer(nullptr, depthAttachment, nullptr, nullptr);
				if (m_lastFrameBuffer.frameBuffer == nullptr || m_lastFrameBuffer.depthOnlyFrameBuffer == nullptr) {
					m_lastFrameBuffer.frameBuffer = m_lastFrameBuffer.depthOnlyFrameBuffer = nullptr;
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - Failed to create the frame buffers!");
				}
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
					if (size.x <= 0 || size.y <= 0) 
						return;
				}

				// Update environment bindings:
				{
					m_bindings.Update(m_viewport);
					m_bindingPool->UpdateAllBindingSets(commandBufferInfo);
				}

				// Run pipelines
				auto runPipelines = [&](
					GraphicsObjectPipelines* pipelines, const Stacktor<Reference<Graphics::BindingSet>, 4u>& bindingSets,
					Graphics::FrameBuffer* frameBuffer, const Vector4* clearColor) {
					pipelines->RenderPass()->BeginPass(commandBufferInfo, frameBuffer, clearColor, false);
					for (size_t i = 0u; i < bindingSets.Size(); i++)
						bindingSets[i]->Bind(commandBufferInfo);

					const GraphicsObjectPipelines::Reader reader(*pipelines);
					for (size_t i = 0; i < reader.Count(); i++)
						reader[i].ExecutePipeline(commandBufferInfo);

					pipelines->RenderPass()->EndPass(commandBufferInfo);
				};
				{
					runPipelines(m_depthOnlyPrePassPipelines, m_depthOnlyEnvironmentBindingSets, m_lastFrameBuffer.depthOnlyFrameBuffer, nullptr);
					const Vector4 clearColor = m_viewport->ClearColor();
					const Vector4* CLEAR_VALUE = &clearColor;
					runPipelines(m_graphicsObjectPipelines, m_environmentBindingSets, frameBuffer, CLEAR_VALUE);
				}
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
	};

	ForwardLightingModel_Opaque_Pass* ForwardLightingModel_Opaque_Pass::Instance() {
		static ForwardLightingModel_Opaque_Pass model;
		return &model;
	}

	Reference<RenderStack::Renderer> ForwardLightingModel_Opaque_Pass::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		if (viewport == nullptr) return nullptr;
		else return Object::Instantiate<Helpers::ForwardRenderer>(viewport, layers, flags);
	}
}
