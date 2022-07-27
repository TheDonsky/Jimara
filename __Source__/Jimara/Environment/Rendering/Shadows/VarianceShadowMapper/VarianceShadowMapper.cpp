#include "VarianceShadowMapper.h"


namespace Jimara {
	struct VarianceShadowMapper::Helpers {
		struct Params {
			alignas(4) float closePlane = 0.01f;
			alignas(4) float farPlane = 1000.0f;
			alignas(4) uint32_t filterSize = 1;
		};

		struct PipelineDescriptor 
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			const Reference<Graphics::Shader> shader;
			Params params;
			float softness = 1.0f;
			const Graphics::BufferReference<Params> paramBuffer;
			Graphics::ArrayBufferReference<float> blurWeights;

			Reference<Graphics::TextureSampler> depthBuffer;
			Reference<Graphics::TextureView> varianceMap;
			Reference<Graphics::TextureSampler> varianceSampler;
			
			SpinLock lock;

			inline PipelineDescriptor(SceneContext* context)
				: shader([&]() -> Reference<Graphics::Shader> {
				Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
				if (shaderSet == nullptr) {
					context->Log()->Fatal(
						"VarianceShadowMapper::Helpers::PipelineDescriptor - Failed to get shader set for the compute module! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				static const Graphics::ShaderClass shaderClass("Jimara/Environment/Rendering/Shadows/VarianceShadowMapper/VarianceShadowMapper_Kernel");
				Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::COMPUTE);
				if (binary == nullptr) {
					context->Log()->Fatal(
						"VarianceShadowMapper::Helpers::PipelineDescriptor - Failed to get shader binary from shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				Reference<Graphics::ShaderCache> cache = Graphics::ShaderCache::ForDevice(context->Graphics()->Device());
				Reference<Graphics::Shader> rv = cache->GetShader(binary);
				if (binary == nullptr)
					context->Log()->Fatal(
						"VarianceShadowMapper::Helpers::PipelineDescriptor - Failed to get shader from cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return rv;
					}())
				, paramBuffer(context->Graphics()->Device()->CreateConstantBuffer<Params>()) {}
			inline virtual ~PipelineDescriptor() {}

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1; }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const override { return this; }

			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }

			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0u }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return paramBuffer; }

			inline virtual size_t StructuredBufferCount()const override { return 1u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 1u }; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return blurWeights; }

			inline virtual size_t TextureSamplerCount()const override { return 1u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 2u }; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return depthBuffer; }

			inline virtual size_t TextureViewCount()const override { return 1u; }
			inline virtual BindingInfo TextureViewInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 3u }; }
			inline virtual Reference<Graphics::TextureView> View(size_t)const override { return varianceMap; }

			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
			inline virtual Size3 NumBlocks() override {
				static const constexpr uint32_t BLOCK_SIZE = 256u;
				auto variance = varianceMap;
				const Size3 size = (variance == nullptr) ? Size3(0u) : variance->TargetTexture()->Size();
				const uint32_t pixelsPerGroup = BLOCK_SIZE - params.filterSize + 1;
				return Size3((size.x + (pixelsPerGroup - 1)) / pixelsPerGroup, size.y, 1);
			}
		};
	};

	VarianceShadowMapper::VarianceShadowMapper(SceneContext* context) 
		: m_context(context)
		, m_pipelineDescriptor(Object::Instantiate<Helpers::PipelineDescriptor>(context)) {
		m_computePipeline = context->Graphics()->Device()->CreateComputePipeline(m_pipelineDescriptor, m_context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (m_computePipeline == nullptr)
			m_context->Log()->Fatal("VarianceShadowMapper::VarianceShadowMapper - Failed to create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
	}

	VarianceShadowMapper::~VarianceShadowMapper() {}


	void VarianceShadowMapper::Configure(float closePlane, float farPlane, float softness, uint32_t filterSize) {
		Helpers::PipelineDescriptor* descriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
		{
			Helpers::Params& params = descriptor->params;
			params.closePlane = closePlane;
			params.farPlane = farPlane;
			params.filterSize = Math::Min(filterSize, 128u) | 1u;
			descriptor->softness = softness;
			//params.blurAmount = Math::Max(blurAmount / static_cast<float>(params.filterSize), 0.00001f);
			descriptor->paramBuffer.Map() = params;
			descriptor->paramBuffer->Unmap(true);
			descriptor->blurWeights = nullptr;
		}
	}

	Reference<Graphics::TextureSampler> VarianceShadowMapper::SetDepthTexture(Graphics::TextureSampler* clipSpaceDepth) {
		Helpers::PipelineDescriptor* descriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
		std::unique_lock<SpinLock> lock(descriptor->lock);
		
		descriptor->depthBuffer = clipSpaceDepth;

		if (clipSpaceDepth != nullptr && (descriptor->varianceMap == nullptr ||
			clipSpaceDepth->TargetView()->TargetTexture()->Size() != descriptor->varianceMap->TargetTexture()->Size())) {
			Reference<Graphics::Texture> texture = m_context->Graphics()->Device()->CreateMultisampledTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R32G32_SFLOAT,
				clipSpaceDepth->TargetView()->TargetTexture()->Size(), 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
			if (texture == nullptr) 
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			descriptor->varianceMap = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (descriptor->varianceMap == nullptr)
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			descriptor->varianceSampler = descriptor->varianceMap->CreateSampler(
				Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::CLAMP_TO_EDGE);
			if (descriptor->varianceSampler == nullptr)
				m_context->Log()->Fatal("VarianceShadowMapper::Configure - Failed to create a sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		Reference<Graphics::TextureSampler> rv = descriptor->varianceSampler;
		return rv;
	}

	Reference<Graphics::TextureSampler> VarianceShadowMapper::VarianceMap()const {
		Helpers::PipelineDescriptor* descriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
		std::unique_lock<SpinLock> lock(descriptor->lock);
		Reference<Graphics::TextureSampler> rv = descriptor->varianceSampler;
		return rv;
	}

	void VarianceShadowMapper::GenerateVarianceMap(Graphics::Pipeline::CommandBufferInfo commandBufferInfo) {
		if (commandBufferInfo.commandBuffer == nullptr) 
			commandBufferInfo = m_context->Graphics()->GetWorkerThreadCommandBuffer();
		Helpers::PipelineDescriptor* descriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
		std::unique_lock<SpinLock> lock(descriptor->lock);
		if (descriptor->blurWeights == nullptr) {
			descriptor->blurWeights = m_context->Graphics()->Device()->CreateArrayBuffer<float>(descriptor->params.filterSize);
			float* weights = descriptor->blurWeights.Map();
			const float filterOffset = static_cast<float>(descriptor->params.filterSize) * 0.5f - 0.5f;
			const float sigma = 1.0f / Math::Max(descriptor->softness, 0.00001f);
			float sum = 0.0f;
			for (uint32_t i = 0; i < descriptor->params.filterSize; i++) {
				const float offset = static_cast<float>(i) - filterOffset;
				const float a = (offset * sigma);
				const float val = std::exp(-0.5f * a * a);
				sum += val;
				weights[i] = val;
			}
			for (uint32_t i = 0; i < descriptor->params.filterSize; i++)
				weights[i] /= sum;
			descriptor->blurWeights->Unmap(true);
		}
		m_computePipeline->Execute(commandBufferInfo);
	}

	void VarianceShadowMapper::Execute() {
		GenerateVarianceMap({});
	}
}
