#include "ForwardLightingModel.h"
#include "../LightingModelPipelines.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"
#include "../../../../Graphics/Data/GraphicsPipelineSet.h"


namespace Jimara {
	namespace {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding> jimara_BindlessTextures =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding>("jimara_BindlessTextures");
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding> jimara_BindlessBuffers =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding>("jimara_BindlessBuffers");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_ForwardRenderer_LightTypeIds =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_ForwardRenderer_LightTypeIds");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ForwardRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_ForwardRenderer_ViewportBuffer");

		public:
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_ForwardRenderer_ViewportBuffer->BindingName()) return jimara_ForwardRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else if (name == jimara_ForwardRenderer_LightTypeIds->BindingName()) return jimara_ForwardRenderer_LightTypeIds;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override {
				return nullptr;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override {
				return nullptr;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override {
				if (name == jimara_BindlessBuffers->BindingName()) return jimara_BindlessBuffers;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override {
				if (name == jimara_BindlessTextures->BindingName()) return jimara_BindlessTextures;
				else return nullptr;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override {
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
			const Reference<LightDataBuffer> m_lightDataBuffer;
			const Reference<LightTypeIdBuffer> m_lightTypeIdBuffer;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_lightDataBuffer(LightDataBuffer::Instance(viewport))
				, m_lightTypeIdBuffer(LightTypeIdBuffer::Instance(viewport))
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
				jimara_BindlessTextures->BoundObject() = m_viewport->Context()->Graphics()->Bindless().SamplerBinding();
				jimara_BindlessBuffers->BoundObject() = m_viewport->Context()->Graphics()->Bindless().BufferBinding();
				jimara_ForwardRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
			}

			inline void Update() {
				jimara_LightDataBinding->BoundObject() = m_lightDataBuffer->Buffer();
				jimara_ForwardRenderer_LightTypeIds->BoundObject() = m_lightTypeIdBuffer->Buffer();
				ViewportBuffer_t& buffer = m_viewportBuffer.Map();
				buffer.view = m_viewport->ViewMatrix();
				buffer.projection = m_viewport->ProjectionMatrix();
				m_viewportBuffer->Unmap(true);
			}

			inline void GetDependencies(Callback<JobSystem::Job*> report) {
				report(m_lightDataBuffer);
				report(m_lightTypeIdBuffer);
			}
		};





		/** FORWARD RENDERER: */

		class ForwardRenderer : public virtual RenderStack::Renderer {
		private:
			const Reference<const ViewportDescriptor> m_viewport;
			const LayerMask m_layerMask;
			const Graphics::RenderPass::Flags m_clearAndResolveFlags;
			const Reference<LightingModelPipelines> m_pipelineObjects;
			EnvironmentDescriptor m_environmentDescriptor;

			
			struct {
				Graphics::Texture::PixelFormat pixelFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::MAX_AVAILABLE;
			} m_renderPass;

			struct {
				Reference<Graphics::Pipeline> environmentPipeline;
				Reference<LightingModelPipelines::Instance> objectPipelines;
			} m_pipelines;

			struct {
				Reference<RenderImages> renderImages;
				Reference<Graphics::FrameBuffer> frameBuffer;
			} m_lastFrameBuffer;

			inline bool RefreshRenderPass(
				Graphics::Texture::PixelFormat pixelFormat, 
				Graphics::Texture::PixelFormat depthFormat,
				Graphics::Texture::Multisampling sampleCount) {
				
				if (m_pipelines.objectPipelines != nullptr && 
					m_renderPass.pixelFormat == pixelFormat && 
					m_renderPass.sampleCount == sampleCount) return true;
				
				m_renderPass.pixelFormat = pixelFormat;
				m_renderPass.depthFormat = depthFormat;
				m_renderPass.sampleCount = sampleCount;

				LightingModelPipelines::RenderPassDescriptor renderPassDesc = {};
				{
					renderPassDesc.sampleCount = sampleCount;
					renderPassDesc.colorAttachmentFormats.Push(pixelFormat);
					renderPassDesc.depthFormat = depthFormat;
					renderPassDesc.renderPassFlags = m_clearAndResolveFlags;
				}
				
				m_pipelines.objectPipelines = m_pipelineObjects->GetInstance(renderPassDesc);
				if (m_pipelines.objectPipelines == nullptr) {
					m_viewport->Context()->Log()->Error(
						"ForwardRenderer::RefreshRenderPass - Failed to create LightingModelPipelines::Instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

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

				m_lastFrameBuffer.frameBuffer = m_pipelines.objectPipelines->RenderPass()->CreateFrameBuffer(
					&colorAttachment, depthAttachment, &resolveAttachment, depthResolve);
				if (m_lastFrameBuffer.frameBuffer == nullptr)
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - Failed to create the frame buffer!");
				else m_lastFrameBuffer.renderImages = images;

				return m_lastFrameBuffer.frameBuffer;
			}

		public:
			inline ForwardRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)
				: m_viewport(viewport)
				, m_layerMask(layers)
				, m_clearAndResolveFlags(flags)
				, m_pipelineObjects([&]()->Reference<LightingModelPipelines> {
				LightingModelPipelines::Descriptor desc = {};
				{
					desc.viewport = viewport;
					desc.layers = layers;
					desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm");
				}
				return LightingModelPipelines::Get(desc);
					}())
				, m_environmentDescriptor(viewport) {
				m_pipelines.environmentPipeline = m_pipelineObjects->CreateEnvironmentPipeline(m_environmentDescriptor);
				if (m_pipelines.environmentPipeline == nullptr) 
					m_viewport->Context()->Log()->Error("ForwardRenderer::ForwardRenderer - Failed to create environment pipeline!");
			}

			inline virtual ~ForwardRenderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				if (images == nullptr || m_pipelines.environmentPipeline == nullptr) return;

				// Get frame buffer:
				Reference<Graphics::FrameBuffer> frameBuffer = RefreshFrameBuffer(images);
				if (frameBuffer == nullptr || m_pipelines.objectPipelines == nullptr) return;

				// Verify resolution:
				{
					Size2 size = images->Resolution();
					if (size.x <= 0 || size.y <= 0) return;
				}

				// Begin render pass:
				{
					const Vector4 clearColor = m_viewport->ClearColor();
					const Vector4* CLEAR_VALUE = &clearColor;
					m_pipelines.objectPipelines->RenderPass()->BeginPass(commandBufferInfo.commandBuffer, frameBuffer, CLEAR_VALUE, false);
				}

				// Set environment:
				{
					m_environmentDescriptor.Update();
					m_pipelines.environmentPipeline->Execute(commandBufferInfo);
				}

				// Draw objects:
				{
					// __TODO__: we need some sorting here, as well as separate opaque and transparent treatment...
					const LightingModelPipelines::Reader reader(m_pipelines.objectPipelines);
					for (size_t i = 0; i < reader.PipelineCount(); i++)
						reader.Pipeline(i)->Execute(commandBufferInfo);
				}

				// End pass:
				m_pipelines.objectPipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				m_environmentDescriptor.GetDependencies(report);
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
