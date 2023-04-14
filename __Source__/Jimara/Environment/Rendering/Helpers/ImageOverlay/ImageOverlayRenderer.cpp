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

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> device;
			const Graphics::BufferReference<KernelSettings> settings;
			const Reference<Graphics::SPIRV_Binary> shader;
			const Reference<Graphics::SPIRV_Binary> shader_SRC_MS;
			const Reference<Graphics::SPIRV_Binary> shader_DST_MS;
			const Reference<Graphics::SPIRV_Binary> shader_SRC_DST_MS;
			const Reference<Graphics::BindingPool> bindingPool;

			inline Data(
				Graphics::GraphicsDevice* graphicsDevice,
				Graphics::Buffer* settingsBuffer,
				Graphics::SPIRV_Binary* kernel,
				Graphics::SPIRV_Binary* kernel_SRC_MS,
				Graphics::SPIRV_Binary* kernel_DST_MS,
				Graphics::SPIRV_Binary* kernel_SRC_DST_MS,
				Graphics::BindingPool* bindingSetPool,
				size_t maxInFlightBuffers)
				: device(graphicsDevice)
				, settings(settingsBuffer)
				, shader(kernel), shader_SRC_MS(kernel_SRC_MS), shader_DST_MS(kernel_DST_MS), shader_SRC_DST_MS(kernel_SRC_DST_MS)
				, bindingPool(bindingSetPool)
				, settingsBufferBinding(Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingsBuffer)) {
				assert(device != nullptr);
				assert(settings != nullptr);
				assert(shader != nullptr);
				assert(shader_SRC_MS != nullptr);
				assert(shader_DST_MS != nullptr);
				assert(shader_SRC_DST_MS != nullptr);
			}

			SpinLock lock;
			const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBufferBinding;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> targetTexture =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> sourceTexture =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
			Rect sourceRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			Rect targetRegion = Rect(Vector2(0.0f), Vector2(1.0f));
			bool settingsDirty = true;

			Reference<Graphics::ComputePipeline> pipeline;
			Reference<Graphics::BindingSet> bindingSet;
			Size3 blockCount = Size3(0u);
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

		auto loadShader = [&](const Graphics::ShaderClass& shaderClass, Graphics::PipelineStage stage, const auto& name) 
			-> Reference<Graphics::SPIRV_Binary> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, stage);
			if (binary == nullptr)
				return fail("Could not load ", name, " shader! [File:", __FILE__, "; Line: ", __LINE__, "]");
			if (binary->BindingSetCount() != 1u)
				return fail("", name, " shader expected to have exactly 1 binding set! [File:", __FILE__, "; Line: ", __LINE__, "]");
			return binary;
		};

		static const OS::Path PROJECT_PATH = "Jimara/Environment/Rendering/Helpers/ImageOverlay";

		static const Graphics::ShaderClass KERNEL_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer");
		const Reference<Graphics::SPIRV_Binary> kernel = loadShader(KERNEL_CLASS, Graphics::PipelineStage::COMPUTE, "Single Sample");
		if (kernel == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_SRC_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_SRC_MS");
		const Reference<Graphics::SPIRV_Binary> kernel_SRC_MS = loadShader(KERNEL_SRC_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Multisampled source");
		if (kernel_SRC_MS == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_DST_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_DST_MS");
		const Reference<Graphics::SPIRV_Binary> kernel_DST_MS = loadShader(KERNEL_DST_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Multisampled target");
		if (kernel_DST_MS == nullptr) return nullptr;

		static const Graphics::ShaderClass KERNEL_SRC_DST_MS_CLASS(PROJECT_PATH / "Jimara_ImageOverlayRenderer_SRC_DST_MS");
		const Reference<Graphics::SPIRV_Binary> kernel_SRC_DST_MS = loadShader(KERNEL_SRC_DST_MS_CLASS, Graphics::PipelineStage::COMPUTE, "Single Sample");
		if (kernel_SRC_DST_MS == nullptr) return nullptr;

		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr)
			return fail("Could not create binding pool! [File:", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Helpers::Data> data = Object::Instantiate<Helpers::Data>(
			device, settings, kernel, kernel_SRC_MS, kernel_DST_MS, kernel_SRC_DST_MS, bindingPool, maxInFlightCommandBuffers);

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
			(sampler->TargetView()->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1))) {
			data.pipeline = nullptr;
			data.bindingSet = nullptr;
		}

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
			(target->TargetTexture()->SampleCount() != Graphics::Texture::Multisampling::SAMPLE_COUNT_1))) {
			data.pipeline = nullptr;
			data.bindingSet = nullptr;
		}

		data.targetTexture->BoundObject() = target;
		data.settingsDirty = true;
	}

	void ImageOverlayRenderer::Execute(const Graphics::InFlightBufferInfo& commandBuffer) {
		Helpers::Data& data = Helpers::State(this);
		std::unique_lock<SpinLock> lock(data.lock);

		// Make sure the images are valid (ei not null and with non-zero sizes)
		{
			auto pixelCount = [](Graphics::TextureView* view) -> size_t {
				if (view == nullptr) return 0u;
				Size2 size = view->TargetTexture()->Size();
				return size_t(size.x) * size.y;
			};
			if (data.sourceTexture->BoundObject() == nullptr || 
				pixelCount(data.sourceTexture->BoundObject()->TargetView()) <= 0u || 
				data.targetTexture->BoundObject() == nullptr || 
				pixelCount(data.targetTexture->BoundObject()) <= 0u) return;
		}

		// (Re)Create compute pipeline:
		if (data.pipeline == nullptr) {
			const Graphics::SPIRV_Binary* shader =
				data.sourceTexture->BoundObject()->TargetView()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1
				? ((data.targetTexture->BoundObject()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1)
					? data.shader : data.shader_DST_MS)
				: ((data.targetTexture->BoundObject()->TargetTexture()->SampleCount() == Graphics::Texture::Multisampling::SAMPLE_COUNT_1)
					? data.shader_SRC_MS : data.shader_SRC_DST_MS);
			data.pipeline = data.device->GetComputePipeline(shader);
			if (data.pipeline == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to get/create compute pipeline! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			auto findBuffer = [&](const auto&) { return data.settingsBufferBinding; };
			auto findSampler = [&](const auto&) { return data.sourceTexture; };
			auto findView = [&](const auto&) { return data.targetTexture; };
			Graphics::BindingSet::Descriptor desc = {};
			{
				desc.pipeline = data.pipeline;
				desc.bindingSetId = 0u;
				desc.find.constantBuffer = &findBuffer;
				desc.find.textureSampler = &findSampler;
				desc.find.textureView = &findView;
			}
			data.bindingSet = data.bindingPool->AllocateBindingSet(desc);
			if (data.bindingSet == nullptr) {
				data.device->Log()->Error(
					"ImageOverlayRenderer::Execute - Failed to allocate binding set! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				data.pipeline = nullptr;
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
			data.blockCount = Size3((targetSize + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u);

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
		{
			data.bindingSet->Update(commandBuffer);
			data.bindingSet->Bind(commandBuffer);
			data.pipeline->Dispatch(commandBuffer, data.blockCount);
		}
	}
}
