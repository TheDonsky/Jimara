#include "DualParaboloidDepthRenderer.h"


namespace Jimara {
	struct DualParaboloidDepthRenderer::Helpers {
		struct ConstantBuffer {
			alignas(16) Vector3 viewOffset = Vector3(0.0f);
			alignas(4) float closePlane = 0.001f;
			alignas(4) float farPlane = 1000.0f;
			alignas(4) float forward = 1.0f;
 		};

		inline static void UpdatePipelines(DualParaboloidDepthRenderer* self) {
			// If texture has not changed, we can leve it be:
			const Reference<Graphics::TextureView> targetTexture = self->m_targetTexture;
			if (targetTexture == self->m_frameBufferTexture) return;

			// If we fail, we do a cleanup and report:
			auto fail = [&](const auto&... message) {
				self->m_pipelines = nullptr;
				self->m_bindingSetsFront.Clear();
				self->m_bindingSetsBack.Clear();
				self->m_frameBufferTexture = nullptr;
				self->m_frameBuffer = nullptr;
				self->m_context->Log()->Error("DualParaboloidDepthRenderer::Helpers::UpdatePipelines - ", message...);
			};

			// Discard frame buffer and frameBufferTexture references:
			self->m_frameBuffer = nullptr;
			self->m_frameBufferTexture = nullptr;

			// If target texture is null, we can simply cleanup:
			if (targetTexture == nullptr) {
				self->m_pipelines = nullptr;
				return;
			}

			// Make sure we have a binding pool:
			if (self->m_bindingPool == nullptr) {
				self->m_bindingPool = self->m_context->Graphics()->Device()->CreateBindingPool(
					self->m_context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (self->m_bindingPool == nullptr)
					return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			// Refresh pipeline set instance:
			{
				const Reference<Graphics::RenderPass> renderPass = self->m_context->Graphics()->Device()->GetRenderPass(
					targetTexture->TargetTexture()->SampleCount(), 0u, nullptr, targetTexture->TargetTexture()->ImageFormat(), Graphics::RenderPass::Flags::CLEAR_DEPTH);
				if (renderPass == nullptr)
					return fail("Failed to create/get render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				GraphicsObjectPipelines::Descriptor desc = {};
				desc.descriptorSet = self->m_graphicsObjectDescriptors;
				desc.frustrumDescriptor = self->m_settings;
				desc.renderPass = renderPass;
				desc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
				desc.layers = self->m_layers;
				desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DualParabolidDepthRenderer.jlm");
				self->m_pipelines = GraphicsObjectPipelines::Get(desc);
				if (self->m_pipelines == nullptr)
					return fail("Failed to get/create GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			// (Re)Create binding sets if needed:
			if (self->m_bindingSetsFront.Size() < self->m_pipelines->EnvironmentPipeline()->BindingSetCount()) {
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = self->m_pipelines->EnvironmentPipeline();

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> jimara_BindlessTextures =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
						self->m_context->Graphics()->Bindless().SamplerBinding());
				auto findBindlessTextures = [&](const auto&) { return jimara_BindlessTextures; };
				desc.find.bindlessTextureSamplers = &findBindlessTextures;

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> jimara_BindlessBuffers =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
						self->m_context->Graphics()->Bindless().BufferBinding());
				auto findBindlessArrays = [&](const auto&) { return jimara_BindlessBuffers; };
				desc.find.bindlessStructuredBuffers = &findBindlessArrays;

				const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> jimara_LightDataBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(
						self->m_context->Graphics()->Device()->CreateArrayBuffer(
							self->m_context->Graphics()->Configuration().ShaderLibrary()->PerLightDataSize(), 1));
				auto findStructuredBuffer = [&](const auto&) { return jimara_LightDataBinding; };
				desc.find.structuredBuffer = &findStructuredBuffer;

				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> jimara_ParaboloidDepthRenderer_ViewportBuffer_Front =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(self->m_constantBufferFront);
				auto findFrontBuffer = [&](const auto&) { return jimara_ParaboloidDepthRenderer_ViewportBuffer_Front; };

				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> jimara_ParaboloidDepthRenderer_ViewportBuffer_Back =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(self->m_constantBufferBack);
				auto findBackBuffer = [&](const auto&) { return jimara_ParaboloidDepthRenderer_ViewportBuffer_Back; };

				while (self->m_bindingSetsFront.Size() < desc.pipeline->BindingSetCount()) {
					assert(self->m_bindingSetsFront.Size() == self->m_bindingSetsBack.Size());
					desc.bindingSetId = self->m_bindingSetsFront.Size();
					{
						desc.find.constantBuffer = &findFrontBuffer;
						const Reference<Graphics::BindingSet> set = self->m_bindingPool->AllocateBindingSet(desc);
						if (set == nullptr)
							return fail("Failed to create environment front binding set for set ", desc.bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						self->m_bindingSetsFront.Push(set);
					}
					{
						desc.find.constantBuffer = &findBackBuffer;
						const Reference<Graphics::BindingSet> set = self->m_bindingPool->AllocateBindingSet(desc);
						if (set == nullptr)
							return fail("Failed to create environment back binding set for set ", desc.bindingSetId, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						self->m_bindingSetsBack.Push(set);
					}
					assert(self->m_bindingSetsFront.Size() == self->m_bindingSetsBack.Size());
				}
			}
			assert(self->m_bindingSetsFront.Size() == self->m_bindingSetsBack.Size());

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

	DualParaboloidDepthRenderer::DualParaboloidDepthRenderer(
		Scene::LogicContext* context, LayerMask layers, const RendererFrustrumDescriptor* rendererFrustrum, RendererFrustrumFlags frustrumFlags)
		: m_context(context)
		, m_layers(layers)
		, m_graphicsObjectDescriptors(GraphicsObjectDescriptor::Set::GetInstance(context))
		, m_constantBufferFront(context->Graphics()->Device()->CreateConstantBuffer<Helpers::ConstantBuffer>())
		, m_constantBufferBack(context->Graphics()->Device()->CreateConstantBuffer<Helpers::ConstantBuffer>())
		, m_settings(Object::Instantiate<FrustrumSettings>(rendererFrustrum, frustrumFlags))
		, m_graphicsSimulation(GraphicsSimulation::JobDependencies::For(context)) {}

	DualParaboloidDepthRenderer::~DualParaboloidDepthRenderer() {}

	void DualParaboloidDepthRenderer::Configure(const Vector3& position, float closePlane, float farPlane) {
		std::unique_lock<SpinLock> lock(m_settings->lock);
		m_settings->position = position;
		m_settings->closePlane = Math::Max(closePlane, std::numeric_limits<float>::epsilon());
		m_settings->farPlane = Math::Max(farPlane, m_settings->closePlane + std::numeric_limits<float>::epsilon() * 32.0f);
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
		else {
			m_targetTexture = depthTexture;
			Helpers::UpdatePipelines(this);
		}
	}

	void DualParaboloidDepthRenderer::Render(Graphics::InFlightBufferInfo commandBufferInfo) {
		// Update stuff if needed:
		if (m_frameBuffer == nullptr || m_pipelines == nullptr) return;

		// Update bindings:
		{
			Helpers::ConstantBuffer& front = *reinterpret_cast<Helpers::ConstantBuffer*>(m_constantBufferFront->Map());
			Helpers::ConstantBuffer& back = *reinterpret_cast<Helpers::ConstantBuffer*>(m_constantBufferBack->Map());
			{
				std::unique_lock<SpinLock> lock(m_settings->lock);
				front.viewOffset = back.viewOffset = -m_settings->position;
				front.closePlane = back.closePlane = m_settings->closePlane;
				front.farPlane = back.farPlane = m_settings->farPlane;
				front.forward = 1.0f;
				back.forward = -1.0f;
			}
			m_constantBufferFront->Unmap(true);
			m_constantBufferBack->Unmap(true);
		}

		// Render:
		{
			const GraphicsObjectPipelines::Reader reader(*m_pipelines);
			auto executeGraphicsPipelines = [&](const auto& bindingSets) {
				const Reference<Graphics::BindingSet>* ptr = bindingSets.Data();
				const Reference<Graphics::BindingSet>* const end = ptr + bindingSets.Size();
				while (ptr < end) {
					Graphics::BindingSet* set = *ptr;
					ptr++;
					set->Update(commandBufferInfo);
					set->Bind(commandBufferInfo);
				}
				for (size_t i = 0; i < reader.Count(); i++)
					reader[i].ExecutePipeline(commandBufferInfo);
			};
			const Vector4 clearColor(0.0f);
			m_pipelines->RenderPass()->BeginPass(commandBufferInfo.commandBuffer, m_frameBuffer, &clearColor, false);
			executeGraphicsPipelines(m_bindingSetsFront);
			executeGraphicsPipelines(m_bindingSetsBack);
			m_pipelines->RenderPass()->EndPass(commandBufferInfo.commandBuffer);
		}
	}

	void DualParaboloidDepthRenderer::Execute() { Render(m_context->Graphics()->GetWorkerThreadCommandBuffer()); }

	void DualParaboloidDepthRenderer::GetDependencies(const Callback<JobSystem::Job*>& addDependency) {
		const Reference<GraphicsObjectPipelines> pipelines = [&]() {
			std::unique_lock<SpinLock> lock(m_textureLock);
			const Reference<GraphicsObjectPipelines> rv = m_pipelines;
			return rv;
		}();
		if (pipelines != nullptr)
			pipelines->GetUpdateTasks(addDependency);
		m_graphicsSimulation->CollectDependencies(addDependency);
	}

	void DualParaboloidDepthRenderer::CollectDependencies(Callback<JobSystem::Job*> addDependency) {
		GetDependencies(addDependency);
	}


	DualParaboloidDepthRenderer::FrustrumSettings::FrustrumSettings(const RendererFrustrumDescriptor* viewportFrustrum, RendererFrustrumFlags frustrumFlags)
		: RendererFrustrumDescriptor(frustrumFlags), m_viewportFrustrum(viewportFrustrum) {}
	DualParaboloidDepthRenderer::FrustrumSettings::~FrustrumSettings() {}
	Matrix4 DualParaboloidDepthRenderer::FrustrumSettings::FrustrumTransform()const {
		std::unique_lock<SpinLock> changeLock(lock);
		const Matrix4 projectionMatrix = Math::Orthographic(farPlane, 1.0f, -farPlane, farPlane);
		Matrix4 viewMatrix = Math::Identity();
		viewMatrix[3] = Vector4(-position, 1.0f);
		return projectionMatrix * viewMatrix;
	}
	Vector3 DualParaboloidDepthRenderer::FrustrumSettings::EyePosition()const {
		std::unique_lock<SpinLock> changeLock(lock);
		return position; 
	}
	const RendererFrustrumDescriptor* DualParaboloidDepthRenderer::FrustrumSettings::ViewportFrustrumDescriptor()const {
		return (m_viewportFrustrum == nullptr) ? ((const RendererFrustrumDescriptor*)this) : m_viewportFrustrum.operator->();
	}
}
