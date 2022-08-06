#include "DualParaboloidDepthRenderer.h"


namespace Jimara {
	struct DualParaboloidDepthRenderer::Helpers {
		struct ConstantBuffer {
			alignas(16) Vector3 viewOffset = Vector3(0.0f);
			alignas(4) float closePlane = 0.001f;
			alignas(4) float farPlane = 1000.0f;
			alignas(4) float forward = 1.0f;
 		};

		inline static Reference<LightingModelPipelines> GetLightingModelPipelines(Scene::LogicContext* context, LayerMask layers) {
			LightingModelPipelines::Descriptor descriptor = {};
			descriptor.context = context;
			descriptor.layers = layers;
			descriptor.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DualParabolidDepthRenderer.jlm");
			return LightingModelPipelines::Get(descriptor);
		}

		inline static void CreateEnvironmentPipelines(
			LightingModelPipelines* pipelines, Scene::LogicContext* context, 
			Graphics::Buffer* frontBuffer, Graphics::Buffer* backBuffer,
			Reference<Graphics::Pipeline>& front, Reference<Graphics::Pipeline>& back) {
			Graphics::ShaderResourceBindings::ShaderBindingDescription description = {};

			const Reference<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding> jimara_BindlessTextures =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding>(
					"jimara_BindlessTextures", context->Graphics()->Bindless().SamplerBinding());
			const Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding* bindlessTextures[] = { jimara_BindlessTextures };
			description.bindlessTextureSamplerBindings = bindlessTextures;
			description.bindlessTextureSamplerBindingCount = 1u;

			const Reference<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding> jimara_BindlessBuffers =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding>(
					"jimara_BindlessBuffers", context->Graphics()->Bindless().BufferBinding());
			const Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding* bindlessBuffers[] = { jimara_BindlessBuffers };
			description.bindlessStructuredBufferBindings = bindlessBuffers;
			description.bindlessStructuredBufferBindingCount = 1u;

			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			jimara_LightDataBinding->BoundObject() = context->Graphics()->Device()->CreateArrayBuffer(
				context->Graphics()->Configuration().ShaderLoader()->PerLightDataSize(), 1);
			const Graphics::ShaderResourceBindings::NamedStructuredBufferBinding* structuredBuffers[] = { jimara_LightDataBinding };
			description.structuredBufferBindings = structuredBuffers;
			description.structuredBufferBindingCount = 1u;

			auto createPipeline = [&](Graphics::Buffer* buffer, Reference<Graphics::Pipeline>& pipeline, const char* pipelineName) {
				const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ParaboloidDepthRenderer_ViewportBuffer =
					Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_DualParaboloidDepthRenderer_ViewportBuffer", buffer);
				const Graphics::ShaderResourceBindings::NamedConstantBufferBinding* constantBuffers[] = { jimara_ParaboloidDepthRenderer_ViewportBuffer };
				description.constantBufferBindings = constantBuffers;
				description.constantBufferBindingCount = 1u;
				pipeline = pipelines->CreateEnvironmentPipeline(description);
				if (pipeline == nullptr)
					context->Log()->Error("DualParaboloidDepthRenderer::Helpers::CreateEnvironmentPipelines - Failed to create the ", pipelineName, " environment pipeline!");
			};
			createPipeline(frontBuffer, front, "front");
			createPipeline(backBuffer, back, "back");
		}

		inline static void UpdatePipelines(DualParaboloidDepthRenderer* self) {
			const Reference<Graphics::TextureView> targetTexture = [&]() {
				std::unique_lock<SpinLock> bufferLock(self->m_textureLock);
				Reference<Graphics::TextureView> rv = self->m_targetTexture;
				return rv;
			}();

			// If texture has not changed, we can leve it be:
			if (targetTexture == self->m_frameBufferTexture) return;

			// Discard frame buffer and frameBufferTexture references:
			self->m_frameBuffer = nullptr;
			self->m_frameBufferTexture = nullptr;

			// If target texture is null, we can simply cleanup:
			if (targetTexture == nullptr) {
				self->m_pipelines = nullptr;
				return;
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
					self->m_context->Log()->Error(
						"DepthOnlyRenderer::Helpers::RefreshFrameBuffer - Failed to create LightingModelPipelines::Instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
			}

			// Create frame buffer:
			{
				self->m_frameBuffer = self->m_pipelines->RenderPass()->CreateFrameBuffer(nullptr, targetTexture, nullptr, nullptr);
				if (self->m_frameBuffer == nullptr)
					self->m_context->Log()->Error(
						"DepthOnlyRenderer::Helpers::RefreshFrameBuffer - Failed to create frame buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else self->m_frameBufferTexture = targetTexture;
			}
		}
	};

	DualParaboloidDepthRenderer::DualParaboloidDepthRenderer(Scene::LogicContext* context, LayerMask layers) 
		: m_context(context)
		, m_lightingModelPipelines(Helpers::GetLightingModelPipelines(context, layers))
		, m_constantBufferFront(context->Graphics()->Device()->CreateConstantBuffer<Helpers::ConstantBuffer>())
		, m_constantBufferBack(context->Graphics()->Device()->CreateConstantBuffer<Helpers::ConstantBuffer>()) {
		Helpers::CreateEnvironmentPipelines(
			m_lightingModelPipelines, m_context,
			m_constantBufferFront, m_constantBufferBack,
			m_environmentPipelineFront, m_environmentPipelineBack);
	}

	DualParaboloidDepthRenderer::~DualParaboloidDepthRenderer() {
	}

	void DualParaboloidDepthRenderer::Configure(const Vector3& position, float closePlane, float farPlane) {
		std::unique_lock<SpinLock> lock(m_settings.lock);
		m_settings.position = position;
		m_settings.closePlane = Math::Max(closePlane, std::numeric_limits<float>::epsilon());
		m_settings.farPlane = Math::Max(farPlane, m_settings.closePlane + std::numeric_limits<float>::epsilon() * 32.0f);
	}

	Graphics::Texture::PixelFormat DualParaboloidDepthRenderer::TargetTextureFormat()const {
		return m_context->Graphics()->Device()->GetDepthFormat();
	}

	void DualParaboloidDepthRenderer::SetTargetTexture(Graphics::TextureView* depthTexture) {
		std::unique_lock<SpinLock> bufferLock(m_textureLock);
		if (depthTexture == m_targetTexture) return;
		else if (depthTexture->TargetTexture()->ImageFormat() != TargetTextureFormat()) {
			m_context->Log()->Error("DualParaboloidDepthRenderer::SetTargetTexture - Texture format (", (uint32_t)depthTexture->TargetTexture()->ImageFormat(), ") not supported!");
		}
		else m_targetTexture = depthTexture;
	}

	void DualParaboloidDepthRenderer::Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo) {
		// Update stuff if needed:
		{
			Helpers::UpdatePipelines(this);
			if (m_frameBuffer == nullptr) return;
		}

		// Update bindings:
		{
			Helpers::ConstantBuffer& front = *reinterpret_cast<Helpers::ConstantBuffer*>(m_constantBufferFront->Map());
			Helpers::ConstantBuffer& back = *reinterpret_cast<Helpers::ConstantBuffer*>(m_constantBufferBack->Map());
			{
				std::unique_lock<SpinLock> lock(m_settings.lock);
				front.viewOffset = back.viewOffset = -m_settings.position;
				front.closePlane = back.closePlane = m_settings.closePlane;
				front.farPlane = back.farPlane = m_settings.farPlane;
				front.forward = 1.0f;
				back.forward = -1.0f;
			}
			m_constantBufferFront->Unmap(true);
			m_constantBufferBack->Unmap(true);
		}

		// Render:
		{
			const LightingModelPipelines::Reader reader(m_pipelines);
			const Vector4 clearColor(0.0f);
			m_pipelines->RenderPass()->BeginPass(commandBufferInfo.commandBuffer, m_frameBuffer, &clearColor, false);
			if (m_environmentPipelineFront != nullptr) {
				m_environmentPipelineFront->Execute(commandBufferInfo);
				for (size_t i = 0; i < reader.PipelineCount(); i++)
					reader.Pipeline(i)->Execute(commandBufferInfo);
			}
			if (m_environmentPipelineBack != nullptr) {
				m_environmentPipelineBack->Execute(commandBufferInfo);
				for (size_t i = 0; i < reader.PipelineCount(); i++)
					reader.Pipeline(i)->Execute(commandBufferInfo);
			}
			m_pipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);
		}
	}

	void DualParaboloidDepthRenderer::Execute() { Render(m_context->Graphics()->GetWorkerThreadCommandBuffer()); }

	void DualParaboloidDepthRenderer::CollectDependencies(Callback<JobSystem::Job*> addDependency) {}
}
