#include "ForwardLightingModel_OIT_Pass.h"
#include "../GraphicsObjectPipelines.h"


namespace Jimara {
	struct ForwardLightingModel_IOT_Pass::Helpers {
		static const constexpr Size3 WORKGROUP_SIZE = Size3(16u, 16u, 1u);

		typedef Stacktor<Reference<Graphics::BindingSet>, 4u> PassBindingSets;

		class Renderer : public virtual RenderStack::Renderer {
		private:
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<Graphics::RenderPass> m_renderPass;
			const Reference<Graphics::BindingPool> m_bindingPool;

			const Reference<Graphics::ComputePipeline> m_clearPipeline;
			const Reference<Graphics::BindingSet> m_clearShaderInput;

			const Reference<const GraphicsObjectPipelines> m_alphaBlendedPipelines;
			const PassBindingSets m_alphaBlendedPassSets;
			const Reference<const GraphicsObjectPipelines> m_additivePipelines;
			const PassBindingSets m_additivePassSets;

			const Reference<Graphics::ComputePipeline> m_blitPipeline;
			const Reference<Graphics::BindingSet> m_blitShaderInput;

			Reference<RenderImages> m_images;
			Reference<Graphics::FrameBuffer> m_frameBuffer;

			bool UpdatePerPixelSamples(RenderImages* images) {
				// __TODO__: Implement this crap!
				return false;
			}

			bool UpdateFrameBuffer(RenderImages* images) {
				if (m_images == images && m_frameBuffer != nullptr)
					return true;
				if (images == nullptr) {
					m_images = nullptr;
					m_frameBuffer = nullptr;
					return false;
				}
				m_frameBuffer = m_renderPass->CreateFrameBuffer(nullptr, images->GetImage(RenderImages::DepthBuffer())->Resolve(), nullptr, nullptr);
				if (m_frameBuffer == nullptr)
					m_viewport->Context()->Log()->Error(
						"ForwardLightingModel_IOT_Pass::Helpers::Renderer::UpdateFrameBuffer - Failed to create frame buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else m_images = images;
				return m_frameBuffer != nullptr;
			}

			void UpdateSettingsBuffers() {
				// __TODO__: Implement this crap!
			}

		public:
			inline Renderer() {
				// __TODO__: Implement this crap!
			}

			inline virtual ~Renderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				if (!UpdatePerPixelSamples(images))
					return;
				
				if (!UpdateFrameBuffer(images)) 
					return;

				UpdateSettingsBuffers();
				m_bindingPool->UpdateAllBindingSets(commandBufferInfo);

				const Size3 workgroupCount = (Size3(images->Resolution(), 1u) + WORKGROUP_SIZE - 1u) / WORKGROUP_SIZE;

				// Execute clear pipeline:
				{
					m_clearShaderInput->Bind(commandBufferInfo);
					m_clearPipeline->Dispatch(commandBufferInfo, workgroupCount);
				}

				// Draw stuff:
				{
					m_renderPass->BeginPass(commandBufferInfo, m_frameBuffer, nullptr);
					auto drawObjects = [&](const GraphicsObjectPipelines* pipelines, const PassBindingSets& sets) {
						const GraphicsObjectPipelines::Reader reader(pipelines);
						for (size_t i = 0u; i < sets.Size(); i++)
							sets[i]->Bind(commandBufferInfo);
						for (size_t i = 0u; i < reader.Count(); i++)
							reader[i].ExecutePipeline(commandBufferInfo);
					};
					drawObjects(m_alphaBlendedPipelines, m_alphaBlendedPassSets);
					drawObjects(m_additivePipelines, m_additivePassSets);
					m_renderPass->EndPass(commandBufferInfo);
				}

				// Execute blit pipeline:
				{
					m_blitShaderInput->Bind(commandBufferInfo);
					m_blitPipeline->Dispatch(commandBufferInfo, workgroupCount);
				}
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				m_alphaBlendedPipelines->GetUpdateTasks(report);
				m_additivePipelines->GetUpdateTasks(report);
			}
		};
	};

	const ForwardLightingModel_IOT_Pass* ForwardLightingModel_IOT_Pass::Instance() {
		static const ForwardLightingModel_IOT_Pass instance;
		return &instance;
	}

	Reference<RenderStack::Renderer> ForwardLightingModel_IOT_Pass::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		if (viewport == nullptr || viewport->Context() == nullptr) return nullptr;
		auto fail = [&](const auto&... message) { 
			viewport->Context()->Log()->Error("ForwardLightingModel_IOT_Pass::CreateRenderer - ", message...); 
			return nullptr; 
		};

		const Reference<GraphicsObjectDescriptor::Set> graphicsObjects = GraphicsObjectDescriptor::Set::GetInstance(viewport->Context());
		if (graphicsObjects == nullptr)
			return fail("Failed to retrieve GraphicsObjectDescriptor::Set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::RenderPass> renderPass = viewport->Context()->Graphics()->Device()->GetRenderPass(
			Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 0u, nullptr, RenderImages::DepthBuffer()->Format(), flags);
		if (renderPass == nullptr)
			return fail("Could not create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		auto getPipelines = [&](GraphicsObjectPipelines::Flags flags) {
			GraphicsObjectPipelines::Descriptor desc = {};
			desc.descriptorSet = graphicsObjects;
			desc.viewportDescriptor = viewport;
			desc.renderPass = renderPass;
			desc.layers = layers;
			desc.flags = flags;
			desc.pipelineFlags = Graphics::GraphicsPipeline::Flags::NONE;
			desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer_OIT_Pass.jlm");
			return GraphicsObjectPipelines::Get(desc);
		};
		const Reference<const GraphicsObjectPipelines> alphaBlendedPipelines = getPipelines(
			GraphicsObjectPipelines::Flags::EXCLUDE_OPAQUE_OBJECTS | GraphicsObjectPipelines::Flags::EXCLUDE_NON_OPAQUE_OBJECTS ^ 
			GraphicsObjectPipelines::Flags::EXCLUDE_ALPHA_BLENDED_OBJECTS);
		if (alphaBlendedPipelines == nullptr)
			return fail("Failed to get alpha-blended graphics object pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<const GraphicsObjectPipelines> additivePipelines = getPipelines(
			GraphicsObjectPipelines::Flags::EXCLUDE_OPAQUE_OBJECTS | GraphicsObjectPipelines::Flags::EXCLUDE_NON_OPAQUE_OBJECTS ^
			GraphicsObjectPipelines::Flags::EXCLUDE_ADDITIVELY_BLENDED_OBJECTS);
		if (additivePipelines == nullptr)
			return fail("Failed to get additive graphics object pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(
			viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return fail("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
	}
}
