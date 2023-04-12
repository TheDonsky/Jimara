#include "DepthOnlyRenderer.h"


namespace Jimara {
	struct DepthOnlyRenderer::Helpers {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Object, public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding> jimara_BindlessTextures =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding>("jimara_BindlessTextures");
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding> jimara_BindlessBuffers =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding>("jimara_BindlessBuffers");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_DepthOnlyRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_DepthOnlyRenderer_ViewportBuffer");

		public:
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string_view& name)const override {
				if (name == jimara_DepthOnlyRenderer_ViewportBuffer->BindingName()) return jimara_DepthOnlyRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string_view& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string_view&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string_view&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string_view& name)const override {
				if (name == jimara_BindlessBuffers->BindingName()) return jimara_BindlessBuffers;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string_view& name)const override {
				if (name == jimara_BindlessTextures->BindingName()) return jimara_BindlessTextures;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string_view&)const override {
				return nullptr;
			}

			static const EnvironmentShapeDescriptor& Instance() {
				static EnvironmentShapeDescriptor instance;
				return instance;
			}
		};

		/** CONCRETE ENVIRONMENT BINDINGS: */
		class EnvironmentDescriptor : public virtual EnvironmentShapeDescriptor {
		private:
			struct ViewportBuffer_t {
				alignas(16) Matrix4 view;
				alignas(16) Matrix4 projection;
			};

			const Reference<const ViewportDescriptor> m_viewport;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
				jimara_BindlessTextures->BoundObject() = m_viewport->Context()->Graphics()->Bindless().SamplerBinding();
				jimara_BindlessBuffers->BoundObject() = m_viewport->Context()->Graphics()->Bindless().BufferBinding();
				jimara_LightDataBinding->BoundObject() = viewport->Context()->Graphics()->Device()->CreateArrayBuffer(
					m_viewport->Context()->Graphics()->Configuration().ShaderLoader()->PerLightDataSize(), 1);
				jimara_DepthOnlyRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
			}

			inline void Update() {
				ViewportBuffer_t& buffer = m_viewportBuffer.Map();
				buffer.view = m_viewport->ViewMatrix();
				buffer.projection = m_viewport->ProjectionMatrix();
				m_viewportBuffer->Unmap(true);
			}
		};

		inline static Reference<Graphics::FrameBuffer> RefreshFrameBuffer(DepthOnlyRenderer* self) {
			const Reference<Graphics::TextureView> targetTexture = [&]() {
				std::unique_lock<SpinLock> bufferLock(self->m_textureLock);
				Reference<Graphics::TextureView> rv = self->m_targetTexture;
				return rv;
			}();

			// If texture has not changed, we can leve it be:
			if (targetTexture == self->m_frameBufferTexture)
				return self->m_frameBuffer;

			// Discard frame buffer and frameBufferTexture references:
			self->m_frameBuffer = nullptr;
			self->m_frameBufferTexture = nullptr;

			// If target texture is null, we can simply cleanup:
			if (targetTexture == nullptr) {
				self->m_pipelines = nullptr;
				return nullptr;
			}

			// Refresh pipeline set instance:
			{
				LightingModelPipelines::RenderPassDescriptor renderPassDesc = {};
				{
					renderPassDesc.sampleCount = targetTexture->TargetTexture()->SampleCount();
					renderPassDesc.depthFormat = targetTexture->TargetTexture()->ImageFormat();
					renderPassDesc.renderPassFlags = Graphics::RenderPass::Flags::CLEAR_DEPTH;
				}
				self->m_pipelines = self->m_lightingModelPipelines->GetInstance(renderPassDesc);
				if (self->m_pipelines == nullptr) {
					self->m_viewport->Context()->Log()->Error(
						"DepthOnlyRenderer::Helpers::RefreshFrameBuffer - Failed to create LightingModelPipelines::Instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
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

	DepthOnlyRenderer::DepthOnlyRenderer(const ViewportDescriptor* viewport, LayerMask layers, const ViewportDescriptor* graphicsObjectViewport)
		: m_viewport(viewport)
		, m_lightingModelPipelines([&]()->Reference<LightingModelPipelines> {
		LightingModelPipelines::Descriptor desc = {};
		{
			desc.descriptorSet = GraphicsObjectDescriptor::Set::GetInstance(viewport->Context());
			desc.viewport = graphicsObjectViewport;
			desc.layers = layers;
			desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DepthOnlyRenderer.jlm");
		}
		return LightingModelPipelines::Get(desc);
			}())
		, m_environmentDescriptor(Object::Instantiate<Helpers::EnvironmentDescriptor>(viewport)) {
		m_environmentPipeline = m_lightingModelPipelines->CreateEnvironmentPipeline(*dynamic_cast<Helpers::EnvironmentDescriptor*>(m_environmentDescriptor.operator->()));
	}

	DepthOnlyRenderer::~DepthOnlyRenderer() {
	}

	Graphics::Texture::PixelFormat DepthOnlyRenderer::TargetTextureFormat()const {
		return m_viewport->Context()->Graphics()->Device()->GetDepthFormat();
	}

	void DepthOnlyRenderer::SetTargetTexture(Graphics::TextureView* depthTexture) {
		std::unique_lock<SpinLock> bufferLock(m_textureLock);
		if (depthTexture == m_targetTexture) return;
		else if (depthTexture->TargetTexture()->ImageFormat() != TargetTextureFormat()) {
			m_viewport->Context()->Log()->Error("DepthOnlyRenderer::SetTargetTexture - Texture format (", (uint32_t)depthTexture->TargetTexture()->ImageFormat(), ") not supported!");
		}
		else m_targetTexture = depthTexture;
	}

	void DepthOnlyRenderer::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		// Get frame buffer:
		const Reference<Graphics::FrameBuffer> frameBuffer = Helpers::RefreshFrameBuffer(this);
		if (frameBuffer == nullptr) return;

		// Update environment:
		{
			const Size2 resolution = frameBuffer->Resolution();
			if (resolution.x <= 0 || resolution.y <= 0) return;
			dynamic_cast<Helpers::EnvironmentDescriptor*>(m_environmentDescriptor.operator->())->Update();
		}

		// Render:
		if (m_environmentPipeline != nullptr) {
			const LightingModelPipelines::Reader reader(m_pipelines);
			const Vector4 clearColor(0.0f);
			m_pipelines->RenderPass()->BeginPass(commandBufferInfo.commandBuffer, frameBuffer, &clearColor, false);
			m_environmentPipeline->Execute(commandBufferInfo);
			for (size_t i = 0; i < reader.PipelineCount(); i++)
				reader.Pipeline(i)->Execute(commandBufferInfo);
			m_pipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);
		}
	}

	void DepthOnlyRenderer::Execute() { Render(m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer()); }

	void DepthOnlyRenderer::CollectDependencies(Callback<JobSystem::Job*>) {}
}
