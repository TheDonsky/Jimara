#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::RasterPass::Helpers {
		inline static bool ObtainRenderPass(RasterPass* self, Graphics::Texture::Multisampling sampleCount) {
			self->m_renderPass = self->m_sharedBindings->tlasViewport->Context()->Graphics()->Device()->GetRenderPass(
				sampleCount, 1u, &PRIMITIVE_RECORD_ID_FORMAT, RenderImages::DepthBuffer()->Format(),
				Graphics::RenderPass::Flags::CLEAR_COLOR | (self->m_flags & Graphics::RenderPass::Flags::CLEAR_DEPTH));
			if (self->m_renderPass == nullptr) {
				self->m_sharedBindings->tlasViewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::ObtainRenderPass - ",
					"Failed to create render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			else return true;
		}

		inline static bool ObtainPipelines(RasterPass* self) {
			GraphicsObjectPipelines::Descriptor pipelineDesc = {};
			{
				pipelineDesc.descriptorSet = self->m_graphicsObjects;
				pipelineDesc.frustrumDescriptor = self->m_sharedBindings->tlasViewport->BaseViewport();
				pipelineDesc.customViewportDataProvider = self->m_objectDescProvider;
				pipelineDesc.renderPass = self->m_renderPass;
				pipelineDesc.layers = self->m_layers;
				pipelineDesc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
				pipelineDesc.lightingModel = OS::Path(LIGHTING_MODEL_PATH);
				pipelineDesc.lightingModelStage = RASTER_PASS_STAGE_NAME;
			}
			self->m_pipelines = GraphicsObjectPipelines::Get(pipelineDesc);
			if (self->m_pipelines == nullptr) {
				self->m_sharedBindings->tlasViewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::ObtainPipelines - ",
					"Failed to obtain graphics object pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			else return true;
		}

		inline static bool CreateEnvironmentBindings(RasterPass* self) {
			Graphics::Pipeline* const environmentPipeline = self->m_pipelines->EnvironmentPipeline();
			if (environmentPipeline == nullptr) {
				self->m_sharedBindings->tlasViewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::CreateEnvironmentBindings - ",
					"Environment Pipeline Missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			self->m_environmentBindings.Clear();
			for (size_t i = 0u; i < environmentPipeline->BindingSetCount(); i++) {
				const Reference<Graphics::BindingSet> set = self->m_sharedBindings->CreateBindingSet(environmentPipeline, i);
				if (set == nullptr) {
					self->m_sharedBindings->tlasViewport->Context()->Log()->Error(
						"RayTracedRenderer::Tools::RasterPass::Helpers::CreateEnvironmentBindings - ",
						"Failed to create binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				self->m_environmentBindings.Push(set);
			}

			return true;
		}

		inline static bool SetSampleCount(RasterPass* self, Graphics::Texture::Multisampling sampleCount) {
			// If sample count is unchanged, do nothing:
			if (self->m_renderPass != nullptr &&
				self->m_pipelines != nullptr &&
				self->m_renderPass->SampleCount() == sampleCount)
				return true;

			// Cleanup:
			auto cleanup = [&]() {
				self->m_renderPass = nullptr;
				self->m_pipelines = nullptr;
				self->m_frameBuffer = nullptr;
				self->m_environmentBindings.Clear();
				return false;
			};
			cleanup();

			// Obtain render pass:
			if (!ObtainRenderPass(self, sampleCount))
				return cleanup();

			// Obtain pipeline descriptor:
			if (!ObtainPipelines(self))
				return cleanup();

			// Re-create binding sets:
			if (!CreateEnvironmentBindings(self))
				return cleanup();

			// Done:
			return true;
		}

		inline static bool SetFrameBufferImages(RasterPass* self, const FrameBuffers& images) {
			// If textures are unchanged, do nothing:
			if (self->m_primitiveRecordBuffer == images.primitiveRecordId &&
				self->m_depthBuffer == images.depthBuffer &&
				self->m_frameBuffer != nullptr)
				return true;

			// Cleanup:
			self->m_primitiveRecordBuffer = nullptr;
			self->m_depthBuffer = nullptr;
			self->m_frameBuffer = nullptr;

			// Create new frame buffer:
			self->m_frameBuffer = self->m_renderPass->CreateFrameBuffer(&images.primitiveRecordId, images.depthBuffer, nullptr, nullptr);
			if (self->m_frameBuffer == nullptr) {
				self->m_sharedBindings->tlasViewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::SetFrameBufferImages - ",
					"Failed to create frame buffer object! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			// Save the texture references:
			self->m_primitiveRecordBuffer = images.primitiveRecordId;
			self->m_depthBuffer = images.depthBuffer;

			// Done:
			return true;
		}
	};

	RayTracedRenderer::Tools::RasterPass::RasterPass(
		GraphicsObjectDescriptor::Set* graphicsObjects,
		IndexedGraphicsObjectDataProvider* objectDescProvider,
		const SharedBindings* sharedBindings,
		const LayerMask& layers, Graphics::RenderPass::Flags flags)
		: m_graphicsObjects(graphicsObjects)
		, m_objectDescProvider(objectDescProvider)
		, m_sharedBindings(sharedBindings)
		, m_layers(layers)
		, m_flags(flags) {
		assert(m_graphicsObjects != nullptr);
		assert(m_objectDescProvider != nullptr);
		assert(m_sharedBindings != nullptr);
	}

	RayTracedRenderer::Tools::RasterPass::~RasterPass() {}

	Reference<RayTracedRenderer::Tools::RasterPass> RayTracedRenderer::Tools::RasterPass::Create(
		const RayTracedRenderer* renderer,
		const SharedBindings* sharedBindings,
		LayerMask layers, Graphics::RenderPass::Flags flags) {

		// Get graphics object set:
		const Reference<GraphicsObjectDescriptor::Set> graphicsObjects = GraphicsObjectDescriptor::Set::GetInstance(sharedBindings->tlasViewport->Context());
		if (graphicsObjects == nullptr) {
			sharedBindings->tlasViewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::RasterPass::Create - ",
				"Failed to get GraphicsObjectDescriptor::Set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		// Get IndexedGraphicsObjectDataProvider:
		IndexedGraphicsObjectDataProvider::Descriptor graphicsObjectDescs = {};
		{
			graphicsObjectDescs.graphicsObjects = graphicsObjects;
			graphicsObjectDescs.frustrumDescriptor = sharedBindings->tlasViewport;
			graphicsObjectDescs.customIndexBindingName = "jm_IndexedGraphicsObjectDataProvider_ID";
		}
		const Reference<IndexedGraphicsObjectDataProvider> viewportObjectDescProvider = IndexedGraphicsObjectDataProvider::GetFor(graphicsObjectDescs);
		if (viewportObjectDescProvider == nullptr) {
			sharedBindings->tlasViewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::RasterPass::Create - ",
				"Could not obtain IndexedGraphicsObjectDataProvider! [File: ", __FILE__, ": Line: ", __LINE__, "]");
			return nullptr;
		}

		// Create raster-pass instance:
		const Reference<RasterPass> rasterPass = new RasterPass(
			graphicsObjects, viewportObjectDescProvider, 
			sharedBindings, layers, flags);
		rasterPass->ReleaseRef();

		// To start-off, set initial sample count to avoid blank frame when possible:
		const Reference<const RenderStack> renderStack = USE_HARDWARE_MULTISAMPLING
			? RenderStack::Main(sharedBindings->tlasViewport->Context())
			: Reference<RenderStack>(nullptr);
		Helpers::SetSampleCount(rasterPass, (renderStack != nullptr) ? renderStack->SampleCount() : Graphics::Texture::Multisampling::SAMPLE_COUNT_1);

		// Done:
		return rasterPass;
	}

	bool RayTracedRenderer::Tools::RasterPass::SetFrameBuffers(const FrameBuffers& frameBuffers) {
		if (!Helpers::SetSampleCount(this, frameBuffers.colorTexture->TargetTexture()->SampleCount()))
			return false;
		else return Helpers::SetFrameBufferImages(this, frameBuffers);
	}

	RayTracedRenderer::Tools::RasterPass::State::State(const RasterPass* pass) 
		: m_pass(pass), m_pipelines(*pass->m_pipelines) {}

	RayTracedRenderer::Tools::RasterPass::State::~State() {}

	bool RayTracedRenderer::Tools::RasterPass::State::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		// If we failed to obtain pipelines earlier, we can't render:
		if (m_pass->m_pipelines == nullptr)
			return false;
		assert(m_pass->m_renderPass != nullptr);

		// If there's no frame buffer, we can't draw:
		if (m_pass->m_frameBuffer == nullptr)
			return false;

		// Obtain pipeline list (TODO: This list has to be shared and has to have some associated data):
		const size_t pipelineCount = m_pipelines.Count();

		// Update environment bindings:
		for (size_t i = 0u; i < m_pass->m_environmentBindings.Size(); i++)
			m_pass->m_environmentBindings[i]->Update(commandBufferInfo);

		// Begin pass:
		{
			auto uintAsFloatBytes = [](uint32_t value) { return *reinterpret_cast<float*>(&value); };
			const Vector4 clearColor = Vector4(uintAsFloatBytes(~uint32_t(0u)));
			m_pass->m_renderPass->BeginPass(commandBufferInfo, m_pass->m_frameBuffer, &clearColor);
		}

		// Set environment:
		for (size_t i = 0u; i < m_pass->m_environmentBindings.Size(); i++)
			m_pass->m_environmentBindings[i]->Bind(commandBufferInfo);

		// Draw to primitiveRecordId buffer:
		for (size_t i = 0u; i < pipelineCount; i++)
			m_pipelines[i].ExecutePipeline(commandBufferInfo);

		// Done:
		m_pass->m_renderPass->EndPass(commandBufferInfo);
		return true;
	}

	void RayTracedRenderer::Tools::RasterPass::GetDependencies(Callback<JobSystem::Job*> report) {
		if (m_pipelines != nullptr)
			m_pipelines->GetUpdateTasks(report);
	}
}
