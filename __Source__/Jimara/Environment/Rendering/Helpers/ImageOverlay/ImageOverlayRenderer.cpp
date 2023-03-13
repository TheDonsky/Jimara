#include "ImageOverlayRenderer.h"
//#define USE_GRAPHICS_PIPELINE


namespace Jimara {
	struct ImageOverlayRenderer::Helpers {
		struct KernelSettings {
			alignas(8) Size2 targetSize = Size2(0u);
			alignas(8) Size2 targetOffset = Size2(0u);

			alignas(8) Vector2 sourcePixelScale = Vector2(0.0f);
			alignas(8) Vector2 sourceOffset = Vector2(0.0f);
			alignas(8) Size2 sourceSize = Size2(0u);

			alignas(4u) int srcSampleCount = 0;
			alignas(4u) int dstSampleCount = 0;
		};
		static_assert(sizeof(KernelSettings) == 48u);

		struct PipelineDescriptor 
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {

			// Configuration:
			const Reference<Graphics::Shader> shader;
			const Reference<Graphics::Buffer> settingsBuffer;
			const Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> sourceImage;
			const Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> targetImage;
			const std::shared_ptr<const Size2> blockCount;

			// Constructor & destructor:
			inline PipelineDescriptor(
				Graphics::Shader* kernelShader,
				Graphics::Buffer* settings,
				const Graphics::ShaderResourceBindings::TextureSamplerBinding* source,
				const Graphics::ShaderResourceBindings::TextureViewBinding* target,
				const std::shared_ptr<Size2>* numBlocks)
				: shader(kernelShader), settingsBuffer(settings)
				, sourceImage(source), targetImage(target), blockCount(*numBlocks) {
				assert(shader != nullptr);
				assert(settingsBuffer != nullptr);
				assert(sourceImage != nullptr);
				assert(targetImage != nullptr);
				assert(blockCount != nullptr);
			}
			inline virtual ~PipelineDescriptor() {}

			// Graphics::ComputePipeline::Descriptor:
			virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
			virtual Size3 NumBlocks() { return Size3(*blockCount, 1u); }

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1u; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t)const override { return this; }

			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }
			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo ConstantBufferInfo(size_t)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE, Graphics::PipelineStage::VERTEX), 0u };
			}
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return settingsBuffer; }
			inline virtual size_t StructuredBufferCount()const override { return 0u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo StructuredBufferInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }
			inline virtual size_t TextureSamplerCount()const override { return 1u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo TextureSamplerInfo(size_t)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 1u };
			}
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return sourceImage->BoundObject(); }
			inline virtual size_t TextureViewCount()const override { return 1u; }
			inline virtual Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo TextureViewInfo(size_t)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 2u };
			}
			inline virtual Reference<Graphics::TextureView> View(size_t)const override { return targetImage->BoundObject(); }
		};


		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> device;
			const Graphics::BufferReference<KernelSettings> settings;
			const Reference<Graphics::Shader> shader;
			const Reference<Graphics::Shader> shader_SRC_MS;
			const Reference<Graphics::Shader> shader_DST_MS;
			const Reference<Graphics::Shader> shader_SRC_DST_MS;
			const size_t maxInFlightCommandBuffers;

			inline Data(
				Graphics::GraphicsDevice* graphicsDevice,
				Graphics::Buffer* settingsBuffer,
				Graphics::Shader* kernel,
				Graphics::Shader* kernel_SRC_MS,
				Graphics::Shader* kernel_DST_MS,
				Graphics::Shader* kernel_SRC_DST_MS,
				size_t maxInFlightBuffers)
				: device(graphicsDevice)
				, settings(settingsBuffer)
				, shader(kernel), shader_SRC_MS(kernel_SRC_MS), shader_DST_MS(kernel_DST_MS), shader_SRC_DST_MS(kernel_SRC_DST_MS)
				, maxInFlightCommandBuffers(Math::Max(size_t(1u), maxInFlightBuffers)) {
				assert(device != nullptr);
				assert(settings != nullptr);
				assert(shader != nullptr);
				assert(shader_SRC_MS != nullptr);
				assert(shader_DST_MS != nullptr);
				assert(shader_SRC_DST_MS != nullptr);
			}

			SpinLock lock;
			const Reference<Graphics::ShaderResourceBindings::TextureViewBinding> targetTexture =
				Object::Instantiate<Graphics::ShaderResourceBindings::TextureViewBinding>();
			const std::shared_ptr<Size2> blockCount = std::make_shared<Size2>(Size2(0u));
			const Reference<Graphics::ShaderResourceBindings::TextureSamplerBinding> sourceTexture =
				Object::Instantiate<Graphics::ShaderResourceBindings::TextureSamplerBinding>();
			Rect sourceRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			Rect targetRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			bool settingsDirty = true;

			Reference<Graphics::Pipeline> pipeline;
		};

		inline static Data& State(const ImageOverlayRenderer* self) { return *dynamic_cast<Data*>(self->m_data.operator->()); }
	};

	Reference<ImageOverlayRenderer> ImageOverlayRenderer::Create(
		Graphics::GraphicsDevice* device, 
		Graphics::ShaderLoader* shaderLoader, 
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("ImageOverlayRenderer::Create - ", message...);
			return nullptr;
		};
		
		if (shaderLoader == nullptr)
			return fail("Shader loader was not provided! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<Helpers::KernelSettings> settings = device->CreateConstantBuffer<Helpers::KernelSettings>();
		if (settings == nullptr)
			return fail("Could not create settings buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		if (shaderSet == nullptr)
			return fail("Could not retrieve shader set! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
		if (shaderCache == nullptr)
			return fail("Could not retrieve shader cache! [File:", __FILE__, "; Line: ", __LINE__, "]");

		auto loadShader = [&](const Graphics::ShaderClass& shaderClass, Graphics::PipelineStage stage, const auto& name) -> Reference<Graphics::Shader> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, stage);
			if (binary == nullptr)
				return fail("Could not load ", name, " shader! [File:", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
			if (shader == nullptr)
				return fail("Could not create ", name, " shader! [File:", __FILE__, "; Line: ", __LINE__, "]");
			return shader;
		};

		static const OS::Path PROJECT_PATH = "Jimara/Environment/Rendering/Helpers/ImageOverlay";

		static const Graphics::ShaderClass KERNEL_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer");
		const Reference<Graphics::Shader> kernel = loadShader(KERNEL_CLASS, Graphics::PipelineStage::COMPUTE, "Single Sample");
		if (kernel == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_SRC_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_SRC_MS");
		const Reference<Graphics::Shader> kernel_SRC_MS = loadShader(KERNEL_SRC_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Multisampled source");
		if (kernel_SRC_MS == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_DST_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_DST_MS");
		const Reference<Graphics::Shader> kernel_DST_MS = loadShader(KERNEL_DST_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Multisampled target");
		if (kernel_DST_MS == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_SRC_DST_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_SRC_DST_MS");
		const Reference<Graphics::Shader> kernel_SRC_DST_MS = loadShader(KERNEL_SRC_DST_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Single Sample");
		if (kernel_SRC_DST_MS == nullptr) return nullptr;

		const Reference<Helpers::Data> data = Object::Instantiate<Helpers::Data>(
			device, settings, kernel, kernel_SRC_MS, kernel_DST_MS, kernel_SRC_DST_MS, maxInFlightCommandBuffers);

		const Reference<ImageOverlayRenderer> renderer = new ImageOverlayRenderer(data);
		renderer->ReleaseRef();
		return renderer;
	}

	ImageOverlayRenderer::~ImageOverlayRenderer() {}

	void ImageOverlayRenderer::SetSourceRegion(const Rect& region) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		if (data.sourceRegion.start == region.start && data.sourceRegion.end == region.end) return;
		data.sourceRegion = region;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::SetSource(Graphics::TextureSampler* sampler) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		data.settingsDirty = true;

		if (data.sourceTexture->BoundObject() == nullptr || sampler == nullptr || (
			(data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1) !=
			(sampler->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1)))
			data.pipeline = nullptr;

		data.sourceTexture->BoundObject() = sampler;
	}

	void ImageOverlayRenderer::SetTargetRegion(const Rect& region) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);
		if (data.targetRegion.start == region.start && data.targetRegion.end == region.end) return;
		data.targetRegion = region;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::SetTarget(Graphics::TextureView* target) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);

		if (data.targetTexture->BoundObject() == target) return;
		if (data.targetTexture->BoundObject() == nullptr || target == nullptr || (
			(data.targetTexture->BoundObject()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1) !=
			(target->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1)))
			data.pipeline = nullptr;

		data.targetTexture->BoundObject() = target;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::Execute(Graphics::Pipeline::CommandBufferInfo commandBuffer) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);

		// Make sure the images are valid (ei not null and with non-zero sizes)
		{
			auto pixelCount = [](Graphics::TextureView* view) -> size_t {
				if (view == nullptr) return 0u;
				Size2 size = view->TargetTexture()->Size();
				return size.x * size.y;
			};
			if (data.sourceTexture->BoundObject() == nullptr || 
				pixelCount(data.sourceTexture->BoundObject()->TargetView()) <= 0u || 
				data.targetTexture->BoundObject() == nullptr || 
				pixelCount(data.targetTexture->BoundObject()) <= 0u) return;
		}

		// (Re)Create compute pipeline:
		if (data.pipeline == nullptr) {
			const Reference<Helpers::PipelineDescriptor> descriptor = Object::Instantiate<Helpers::PipelineDescriptor>(
				data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1 
				? ((data.targetTexture->BoundObject()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1) 
					? data.shader : data.shader_DST_MS)
				: ((data.targetTexture->BoundObject()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1) 
					? data.shader_SRC_MS : data.shader_SRC_DST_MS)
				, data.settings, data.sourceTexture, data.targetTexture, &data.blockCount);
			data.pipeline = data.device->CreateComputePipeline(descriptor, data.maxInFlightCommandBuffers);
			if (data.pipeline == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to create compute pipeline!",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Update settings:
		if (data.settingsDirty) {
			const Size2 targetTextureSize = data.targetTexture->BoundObject()->TargetTexture()->Size();
			const Size2 sourceTextureSize = data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->Size();

			const Size2 targetStart = Size2(
				Vector2(Math::Max(data.targetRegion.start.x, 0.0f), Math::Max(data.targetRegion.start.y, 0.0f)) * Vector2(targetTextureSize));
			if (targetStart.x >= targetTextureSize.x || targetStart.y >= targetTextureSize.y) return;

			const Size2 rawTargetEnd = Size2(data.targetRegion.end * Vector2(targetTextureSize) + 0.5f);
			const Size2 targetEnd = Size2(
				Math::Max(Math::Min(rawTargetEnd.x, targetTextureSize.x), targetStart.x),
				Math::Max(Math::Min(rawTargetEnd.y, targetTextureSize.y), targetStart.y));
			const Size2 targetSize = targetEnd - targetStart;
			if (targetSize.x <= 0u || targetSize.y <= 0u) return;

			static const constexpr uint32_t BLOCK_SIZE = 16u;
			(*data.blockCount) = (targetSize + BLOCK_SIZE - 1u) / BLOCK_SIZE;

			Helpers::KernelSettings& settings = data.settings.Map();

			settings.targetSize = targetSize;
			settings.targetOffset = targetStart;

			settings.sourcePixelScale = data.sourceRegion.Size() / Vector2(
				Math::Max(targetTextureSize.x - 1, 1u), Math::Max(targetTextureSize.y - 1, 1u));
			settings.sourceOffset = data.sourceRegion.start;
			settings.sourceSize = sourceTextureSize;

			settings.srcSampleCount = (int)data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount();
			settings.dstSampleCount = (int)data.targetTexture->BoundObject()->TargetTexture()->SampleCount();

			data.settings->Unmap(true);
			data.settingsDirty = false;
		}

		// Execute compute pipeline:
		data.pipeline->Execute(commandBuffer);
	}
}
