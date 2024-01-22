#include "DepthOnlyRenderer.h"


namespace Jimara {
	struct DepthOnlyRenderer::Helpers {
		inline static Reference<Graphics::FrameBuffer> RefreshFrameBuffer(DepthOnlyRenderer* self) {
			// If texture has not changed, we can leve it be:
			const Reference<Graphics::TextureView> targetTexture = self->m_targetTexture;
			if (targetTexture == self->m_frameBufferTexture)
				return self->m_frameBuffer;

			// If we fail, we do a cleanup and report:
			auto fail = [&](const auto&... message) {
				self->m_pipelines = nullptr;
				self->m_bindingSets.Clear();
				self->m_frameBufferTexture = nullptr;
				self->m_frameBuffer = nullptr;
				self->m_viewport->Context()->Log()->Error("DepthOnlyRenderer::Helpers::RefreshFrameBuffer - ", message...);
				return nullptr;
			};

			// Discard frame buffer and frameBufferTexture references:
			self->m_frameBuffer = nullptr;
			self->m_frameBufferTexture = nullptr;

			// If target texture is null, we can simply cleanup:
			if (targetTexture == nullptr) {
				self->m_pipelines = nullptr;
				return nullptr;
			}

			// Make sure we have a binding pool:
			if (self->m_bindingPool == nullptr) {
				self->m_bindingPool = self->m_viewport->Context()->Graphics()->Device()->CreateBindingPool(
					self->m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (self->m_bindingPool == nullptr)
					return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			// Refresh pipeline set instance:
			{
				const Reference<Graphics::RenderPass> renderPass = self->m_viewport->Context()->Graphics()->Device()->GetRenderPass(
					targetTexture->TargetTexture()->SampleCount(), 0u, nullptr, targetTexture->TargetTexture()->ImageFormat(), Graphics::RenderPass::Flags::CLEAR_DEPTH);
				if (renderPass == nullptr)
					return fail("Failed to create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				GraphicsObjectPipelines::Descriptor desc = {};
				desc.descriptorSet = self->m_graphicsObjectDescriptors;
				desc.frustrumDescriptor = self->m_graphicsObjectViewport;
				desc.renderPass = renderPass;
				desc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
				desc.layers = self->m_layers;
				desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DepthOnlyRenderer.jlm");
				self->m_pipelines = GraphicsObjectPipelines::Get(desc);
				if (self->m_pipelines == nullptr)
					return fail("Failed to get/create GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			// (Re)Create binding sets if needed:
			if (self->m_bindingSets.Size() < self->m_pipelines->EnvironmentPipeline()->BindingSetCount()) {
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = self->m_pipelines->EnvironmentPipeline();
				
				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> jimara_BindlessTextures =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
						self->m_viewport->Context()->Graphics()->Bindless().SamplerBinding());
				auto findBindlessTextures = [&](const auto&) { return jimara_BindlessTextures; };
				desc.find.bindlessTextureSamplers = &findBindlessTextures;

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> jimara_BindlessBuffers =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
						self->m_viewport->Context()->Graphics()->Bindless().BufferBinding());
				auto findBindlessArrays = [&](const auto&) { return jimara_BindlessBuffers; };
				desc.find.bindlessStructuredBuffers = &findBindlessArrays;

				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> jimara_LightDataBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(
						self->m_viewport->Context()->Graphics()->Device()->CreateArrayBuffer(
							self->m_viewport->Context()->Graphics()->Configuration().ShaderLoader()->PerLightDataSize(), 1));
				auto findStructuredBuffer = [&](const auto&) { return jimara_LightDataBinding; };
				desc.find.structuredBuffer = &findStructuredBuffer;

				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> jimara_DepthOnlyRenderer_ViewportBuffer =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(self->m_viewportBuffer);
				auto findConstantBuffer = [&](const auto&) { return jimara_DepthOnlyRenderer_ViewportBuffer; };
				desc.find.constantBuffer = &findConstantBuffer;

				while (self->m_bindingSets.Size() < desc.pipeline->BindingSetCount()) {
					desc.bindingSetId = self->m_bindingSets.Size();
					const Reference<Graphics::BindingSet> set = self->m_bindingPool->AllocateBindingSet(desc);
					if (set == nullptr)
						return fail("Failed to create environment binding set for set ", desc.bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					self->m_bindingSets.Push(set);
				}
			}

			// Create frame buffer:
			{
				self->m_frameBuffer = self->m_pipelines->RenderPass()->CreateFrameBuffer(nullptr, targetTexture, nullptr, nullptr);
				if (self->m_frameBuffer == nullptr)
					self->m_viewport->Context()->Log()->Error(
						"DepthOnlyRenderer::Helpers::RefreshFrameBuffer - Failed to create frame buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else self->m_frameBufferTexture = targetTexture;
			}

			return self->m_frameBuffer;
		}
	};

	DepthOnlyRenderer::DepthOnlyRenderer(const ViewportDescriptor* viewport, LayerMask layers, const RendererFrustrumDescriptor* graphicsObjectFrustrum)
		: m_viewport(viewport)
		, m_graphicsObjectViewport(graphicsObjectFrustrum)
		, m_graphicsObjectDescriptors(GraphicsObjectDescriptor::Set::GetInstance(viewport->Context()))
		, m_layers(layers)
		, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {}

	DepthOnlyRenderer::~DepthOnlyRenderer() {}

	Graphics::Texture::PixelFormat DepthOnlyRenderer::TargetTextureFormat()const {
		return m_viewport->Context()->Graphics()->Device()->GetDepthFormat();
	}

	void DepthOnlyRenderer::SetTargetTexture(Graphics::TextureView* depthTexture) {
		std::unique_lock<SpinLock> bufferLock(m_textureLock);
		if (depthTexture == m_targetTexture) return;
		else if (depthTexture->TargetTexture()->ImageFormat() != TargetTextureFormat()) {
			m_viewport->Context()->Log()->Error("DepthOnlyRenderer::SetTargetTexture - Texture format (", (uint32_t)depthTexture->TargetTexture()->ImageFormat(), ") not supported!");
		}
		else {
			m_targetTexture = depthTexture;
			Helpers::RefreshFrameBuffer(this);
		}
	}

	void DepthOnlyRenderer::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		std::unique_lock<SpinLock> bufferLock(m_textureLock);

		// Get frame buffer:
		const Reference<Graphics::FrameBuffer> frameBuffer = m_frameBuffer;
		if (frameBuffer == nullptr || m_pipelines == nullptr) 
			return;

		// Do nothing if resolution is 0:
		{
			const Size2 resolution = frameBuffer->Resolution();
			if (resolution.x <= 0 || resolution.y <= 0) return;
		}

		// Update environment:
		{
			if (m_viewportBuffer == nullptr) 
				return;
			ViewportBuffer_t& buffer = m_viewportBuffer.Map();
			buffer.view = m_viewport->ViewMatrix();
			buffer.projection = m_viewport->ProjectionMatrix();
			buffer.viewPose = Math::Inverse(buffer.view);
			m_viewportBuffer->Unmap(true);
		}

		// Bind environment pipelines:
		{
			const Reference<Graphics::BindingSet>* ptr = m_bindingSets.Data();
			const Reference<Graphics::BindingSet>* const end = ptr + m_bindingSets.Size();
			while (ptr < end) {
				Graphics::BindingSet* set = *ptr;
				ptr++;
				set->Update(commandBufferInfo);
				set->Bind(commandBufferInfo);
			}
		}

		// Render:
		{
			const Vector4 clearColor(0.0f);
			m_pipelines->RenderPass()->BeginPass(commandBufferInfo, frameBuffer, &clearColor, false);
			GraphicsObjectPipelines::Reader reader(*m_pipelines);
			for (size_t i = 0; i < reader.Count(); i++)
				reader[i].ExecutePipeline(commandBufferInfo);
			m_pipelines->RenderPass()->EndPass(commandBufferInfo);
		}
	}

	void DepthOnlyRenderer::Execute() { Render(m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer()); }

	void DepthOnlyRenderer::GetDependencies(const Callback<JobSystem::Job*>& addDependency) {
		const Reference<GraphicsObjectPipelines> pipelines = [&]() {
			std::unique_lock<SpinLock> lock(m_textureLock);
			const Reference<GraphicsObjectPipelines> rv = m_pipelines;
			return rv;
		}();
		if (pipelines != nullptr)
			pipelines->GetUpdateTasks(addDependency);
	}

	void DepthOnlyRenderer::CollectDependencies(Callback<JobSystem::Job*> addDependency) {
		GetDependencies(addDependency);
	}
}
