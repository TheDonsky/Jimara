#include "VarianceShadowMapper.h"


namespace Jimara {
	Reference<VarianceShadowMapper> VarianceShadowMapper::Create(SceneContext* context) {
		if (context == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			context->Log()->Error("VarianceShadowMapper::Create - ", message...);
			return nullptr;
		};
		
		Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
		if (shaderSet == nullptr) return fail("Failed to get shader set for the compute module! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		static const Graphics::ShaderClass shaderClass("Jimara/Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper_Kernel");
		Reference<Graphics::SPIRV_Binary> shader = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::COMPUTE);
		if (shader == nullptr) return fail("Failed to get shader binary from shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::Experimental::ComputePipeline> vsmPipeline = context->Graphics()->Device()->GetComputePipeline(shader);
		if (vsmPipeline == nullptr) return fail("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		else if (vsmPipeline->BindingSetCount() != 1u) return fail("Pipeline binding set count expected to be 1! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = context->Graphics()->Device()->CreateBindingPool(
			context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr) return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<Params> paramsBuffer = context->Graphics()->Device()->CreateConstantBuffer<Params>();
		if (paramsBuffer == nullptr) return fail("Failed to create parameter buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ResourceBinding<Graphics::Buffer>> params = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(paramsBuffer);
		auto searchParams = [&](const auto&) { return params; };

		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> blurFilter = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
		auto searchBlurFilter = [&](const auto&) { return blurFilter; };

		const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> depthBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
		auto searchDepthBuffer = [&](const auto&) { return depthBuffer; };

		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> varianceMap = Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		auto searchVarianceMap = [&](const auto&) { return varianceMap; };

		Graphics::BindingSet::Descriptor setDesc = {};
		{
			setDesc.pipeline = vsmPipeline;
			setDesc.bindingSetId = 0u;
			setDesc.findConstantBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::Buffer>::FromCall(&searchParams);
			setDesc.findStructuredBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::ArrayBuffer>::FromCall(&searchBlurFilter);
			setDesc.findTextureSampler = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureSampler>::FromCall(&searchDepthBuffer);
			setDesc.findTextureView = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureView>::FromCall(&searchVarianceMap);
		}
		const Reference<Graphics::BindingSet> bindingSet = bindingPool->AllocateBindingSet(setDesc);
		if (bindingSet == nullptr) return fail("Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<VarianceShadowMapper> result = new VarianceShadowMapper(
			context, vsmPipeline, bindingSet, paramsBuffer, blurFilter, depthBuffer, varianceMap);
		result->ReleaseRef();
		return result;
	}

	VarianceShadowMapper::VarianceShadowMapper(
		SceneContext* context,
		Graphics::Experimental::ComputePipeline* vsmPipeline,
		Graphics::BindingSet* bindingSet,
		Graphics::Buffer* params,
		Graphics::ResourceBinding<Graphics::ArrayBuffer>* blurFilter,
		Graphics::ResourceBinding<Graphics::TextureSampler>* depthBuffer,
		Graphics::ResourceBinding<Graphics::TextureView>* varianceMap) 
		: m_context(context)
		, m_vsmPipeline(vsmPipeline)
		, m_bindingSet(bindingSet)
		, m_paramsBuffer(params)
		, m_blurFilter(blurFilter)
		, m_depthBuffer(depthBuffer)
		, m_varianceMap(varianceMap) {
		assert(m_context != nullptr);
		assert(m_vsmPipeline != nullptr);
		assert(m_bindingSet != nullptr);
		assert(m_paramsBuffer != nullptr);
		assert(m_blurFilter != nullptr);
		assert(m_depthBuffer != nullptr);
		assert(m_varianceMap != nullptr);
	}

	VarianceShadowMapper::~VarianceShadowMapper() {}


	void VarianceShadowMapper::Configure(float closePlane, float farPlane, float softness, uint32_t filterSize, bool linearDepth) {
		std::unique_lock<SpinLock> lock(m_lock);

		// Make sure stuff behaves:
		{
			softness = Math::Max(softness, 0.00001f);
			filterSize = Math::Min(filterSize, 128u) | 1u;
		}

		// Early exit:
		if (m_params.closePlane == closePlane &&
			m_params.farPlane == farPlane &&
			m_params.filterSize == filterSize &&
			m_softness == softness &&
			(m_params.linearDepth != 0u) == linearDepth) return;
		
		// Update params:
		{
			m_params.closePlane = closePlane;
			m_params.farPlane = farPlane;
			m_params.filterSize = filterSize;
			m_softness = softness;
			m_params.linearDepth = (linearDepth ? 1u : 0u);
		}

		// Update buffer and discard weights:
		{
			m_paramsBuffer.Map() = m_params;
			m_paramsBuffer->Unmap(true);
			m_blurFilter->BoundObject() = nullptr;
		}
	}

	Reference<Graphics::TextureSampler> VarianceShadowMapper::SetDepthTexture(Graphics::TextureSampler* depthBuffer, bool fp32Variance) {
		const Graphics::Texture::PixelFormat format = fp32Variance ? Graphics::Texture::PixelFormat::R32G32_SFLOAT : Graphics::Texture::PixelFormat::R16G16_SFLOAT;
		std::unique_lock<SpinLock> lock(m_lock);

		// Early exit:
		if (m_depthBuffer->BoundObject() == depthBuffer &&
			m_varianceMap->BoundObject() != nullptr &&
			m_varianceMap->BoundObject()->TargetTexture()->ImageFormat() == format)
			return m_varianceSampler;

		// Update source:
		m_depthBuffer->BoundObject() = depthBuffer;

		// Update result:
		if (depthBuffer != nullptr && (m_varianceMap->BoundObject() == nullptr ||
			m_varianceMap->BoundObject()->TargetTexture()->ImageFormat() != format ||
			depthBuffer->TargetView()->TargetTexture()->Size() != m_varianceMap->BoundObject()->TargetTexture()->Size())) {
			Reference<Graphics::Texture> texture = m_context->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, format,
				depthBuffer->TargetView()->TargetTexture()->Size(), 1u, true);
			if (texture == nullptr) 
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			m_varianceMap->BoundObject() = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D, 0u, 1u);
			if (m_varianceMap->BoundObject() == nullptr)
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			Reference<Graphics::TextureView> fullViewWithMips = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (fullViewWithMips == nullptr)
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create the full texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			m_varianceSampler = fullViewWithMips->CreateSampler(
				Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::REPEAT);
			if (m_varianceSampler == nullptr)
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			m_blurFilter->BoundObject() = nullptr;
		}

		Reference<Graphics::TextureSampler> rv = m_varianceSampler;
		return rv;
	}

	Reference<Graphics::TextureSampler> VarianceShadowMapper::VarianceMap()const {
		//Helpers::PipelineDescriptor* descriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
		std::unique_lock<SpinLock> lock(m_lock);
		Reference<Graphics::TextureSampler> rv = m_varianceSampler;
		return rv;
	}

	void VarianceShadowMapper::GenerateVarianceMap(Graphics::InFlightBufferInfo commandBufferInfo) {
		if (commandBufferInfo.commandBuffer == nullptr) 
			commandBufferInfo = m_context->Graphics()->GetWorkerThreadCommandBuffer();
		auto fail = [&](const auto&... message) {
			m_context->Log()->Error("VarianceShadowMapper::GenerateVarianceMap - ", message...);
		};

		std::unique_lock<SpinLock> lock(m_lock);
		if (m_varianceMap->BoundObject() == nullptr) return;
		
		if (m_blurFilter->BoundObject() == nullptr) {
			m_blurFilter->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<float>(m_params.filterSize);
			if (m_blurFilter->BoundObject()) return fail("Failed to create blur filter! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			float* weights = reinterpret_cast<float*>(m_blurFilter->BoundObject()->Map());
			const float filterOffset = static_cast<float>(m_params.filterSize) * 0.5f - 0.5f;
			Vector3 size = m_varianceMap->BoundObject()->TargetTexture()->Size();
			const float sigma = 1.0f / (Math::Max(m_softness, 0.00001f) * 0.01f * std::sqrt(size.x * size.y));
			float sum = 0.0f;
			for (uint32_t i = 0; i < m_params.filterSize; i++) {
				const float offset = static_cast<float>(i) - filterOffset;
				const float a = (offset * sigma);
				const float val = std::exp(-0.5f * a * a);
				sum += val;
				weights[i] = val;
			}
			for (uint32_t i = 0; i < m_params.filterSize; i++)
				weights[i] /= sum;
			m_blurFilter->BoundObject()->Unmap(true);
		}

		const Size3 blockCount = [&]() {
			static const constexpr uint32_t BLOCK_SIZE = 256u;
			auto variance = m_varianceMap->BoundObject().operator->();
			const Size3 size = (variance == nullptr) ? Size3(0u) : variance->TargetTexture()->Size();
			const uint32_t pixelsPerGroup = BLOCK_SIZE - m_params.filterSize + 1;
			return Size3((size.x + (pixelsPerGroup - 1)) / pixelsPerGroup, size.y, 1);
		}();

		m_bindingSet->Update(commandBufferInfo.inFlightBufferId);
		m_vsmPipeline->Dispatch(commandBufferInfo.commandBuffer, blockCount);
		m_varianceMap->BoundObject()->TargetTexture()->GenerateMipmaps(commandBufferInfo.commandBuffer);
	}

	void VarianceShadowMapper::Execute() {
		GenerateVarianceMap({});
	}
}
