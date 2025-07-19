#include "RayTracedRenderer_Tools.h"


namespace Jimara {
	struct RayTracedRenderer::Tools::RasterPass::Helpers {
		inline static bool SetSampleCount(RasterPass* self, Graphics::Texture::Multisampling sampleCount) {
			// If sample count is unchanged, do nothing:
			if (self->m_renderPass != nullptr &&
				self->m_pipelines != nullptr &&
				self->m_renderPass->SampleCount() == sampleCount)
				return true;

			// Cleanup:
			self->m_renderPass = nullptr;
			self->m_pipelines = nullptr;
			self->m_frameBuffer = nullptr;

			// Obtain render pass:
			self->m_renderPass = self->m_viewport->Context()->Graphics()->Device()->GetRenderPass(
				sampleCount, 1u, &PRIMITIVE_RECORD_ID_FORMAT, RenderImages::DepthBuffer()->Format(),
				Graphics::RenderPass::Flags::CLEAR_COLOR | (self->m_flags & Graphics::RenderPass::Flags::CLEAR_DEPTH));
			if (self->m_renderPass == nullptr) {
				self->m_viewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::SetSampleCount - ",
					"Failed to create render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			// Obtain pipeline descriptor:
			GraphicsObjectPipelines::Descriptor pipelineDesc = {};
			{
				pipelineDesc.descriptorSet = self->m_graphicsObjects;
				pipelineDesc.frustrumDescriptor = self->m_viewport;
				pipelineDesc.customViewportDataProvider = self->m_objectDescProvider;
				pipelineDesc.renderPass = self->m_renderPass;
				pipelineDesc.layers = self->m_layers;
				pipelineDesc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
				pipelineDesc.lightingModel = OS::Path(LIGHTING_MODEL_PATH);
				pipelineDesc.lightingModelStage = "RasterPass";
			}
			self->m_pipelines = GraphicsObjectPipelines::Get(pipelineDesc);
			if (self->m_pipelines == nullptr) {
				self->m_viewport->Context()->Log()->Error(
					"RayTracedRenderer::Tools::RasterPass::Helpers::SetSampleCount - ",
					"Failed to obtain graphics object pipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

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
				self->m_viewport->Context()->Log()->Error(
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

	RayTracedRenderer::Tools::RasterPass::RasterPass(const ViewportDescriptor* viewport,
		GraphicsObjectDescriptor::Set* graphicsObjects,
		IndexedGraphicsObjectDataProvider* objectDescProvider,
		const LayerMask& layers, Graphics::RenderPass::Flags flags)
		: m_viewport(viewport)
		, m_graphicsObjects(graphicsObjects)
		, m_objectDescProvider(objectDescProvider)
		, m_layers(layers)
		, m_flags(flags) {
		assert(m_viewport != nullptr);
		assert(m_graphicsObjects != nullptr);
	}

	RayTracedRenderer::Tools::RasterPass::~RasterPass() {}

	Reference<RayTracedRenderer::Tools::RasterPass> RayTracedRenderer::Tools::RasterPass::Create(
		const RayTracedRenderer* renderer,
		const ViewportDescriptor* viewport,
		LayerMask layers, Graphics::RenderPass::Flags flags) {

		// Get graphics object set:
		const Reference<GraphicsObjectDescriptor::Set> graphicsObjects = GraphicsObjectDescriptor::Set::GetInstance(viewport->Context());
		if (graphicsObjects == nullptr) {
			viewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::RasterPass::Create - ",
				"Failed to get GraphicsObjectDescriptor::Set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		// Get IndexedGraphicsObjectDataProvider:
		IndexedGraphicsObjectDataProvider::Descriptor graphicsObjectDescs = {};
		{
			graphicsObjectDescs.graphicsObjects = graphicsObjects;
			graphicsObjectDescs.frustrumDescriptor = viewport;
			graphicsObjectDescs.customIndexBindingName = "jm_IndexedGraphicsObjectDataProvider_ID";
		}
		const Reference<IndexedGraphicsObjectDataProvider> viewportObjectDescProvider = IndexedGraphicsObjectDataProvider::GetFor(graphicsObjectDescs);
		if (viewportObjectDescProvider == nullptr) {
			viewport->Context()->Log()->Error(
				"RayTracedRenderer::Tools::RasterPass::Create - ",
				"Could not obtain IndexedGraphicsObjectDataProvider! [File: ", __FILE__, ": Line: ", __LINE__, "]");
			return nullptr;
		}

		// Create raster-pass instance:
		const Reference<RasterPass> rasterPass = new RasterPass(viewport, graphicsObjects, viewportObjectDescProvider, layers, flags);
		rasterPass->ReleaseRef();

		// To start-off, set initial sample count to avoid blank frame when possible:
		const Reference<const RenderStack> renderStack = USE_HARDWARE_MULTISAMPLING
			? RenderStack::Main(viewport->Context())
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

	bool RayTracedRenderer::Tools::RasterPass::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		// __TODO__: Implement this crap!
		return true;
	}

	void RayTracedRenderer::Tools::RasterPass::GetDependencies(Callback<JobSystem::Job*> report) {
		// __TODO__: Implement this crap!
	}
}
