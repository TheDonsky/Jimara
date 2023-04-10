#include "BloomKernel.h"
#include "../../TransientImage.h"


namespace Jimara {
	struct BloomKernel::Helpers {
		static const constexpr uint32_t BlockSize() { return 16u; }
		inline static constexpr uint32_t MinMipSize() { return 2; }



		struct FilterDescriptor 
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			const Reference<Graphics::Shader> filterShader;
			Reference<Graphics::TextureSampler> source;
			Reference<Graphics::TextureView> result;

			inline FilterDescriptor(Graphics::Shader* shader) : filterShader(shader) {}
			inline virtual ~FilterDescriptor() {}


			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return filterShader; }
			inline virtual Size3 NumBlocks()const override {
				const Size3 size = (result == nullptr) ? Size3(0u) : result->TargetTexture()->Size();
				auto blockCount = [](uint32_t width) { return (width + BlockSize() - 1) / BlockSize(); };
				return Size3(blockCount(size.x), blockCount(size.y), 1);
			}


			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1u; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return this; }


			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }
			
			inline virtual size_t ConstantBufferCount()const override { return 0u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return {}; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return nullptr; }
			
			inline virtual size_t StructuredBufferCount()const override { return 0u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return {}; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

			inline virtual size_t TextureSamplerCount()const override { return 1u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0u }; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return source; }

			inline virtual size_t TextureViewCount()const override { return 1u; }
			inline virtual BindingInfo TextureViewInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 1u }; }
			inline virtual Reference<Graphics::TextureView> View(size_t index)const override { return result; }
		};



		struct FilterWithSettings : public virtual FilterDescriptor {
			const Reference<Graphics::Buffer> settings;

			inline FilterWithSettings(Graphics::Shader* shader, Graphics::Buffer* settingsBuffer)
				: FilterDescriptor(shader), settings(settingsBuffer) {}
			inline virtual ~FilterWithSettings() {}

			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 2u }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return settings; }
		};

		struct MixFilterDescriptor : public virtual FilterWithSettings {
			const Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> dirt;

			inline MixFilterDescriptor(
				Graphics::Shader* shader, Graphics::Buffer* settingsBuffer,
				const Graphics::ShaderResourceBindings::TextureSamplerBinding* dirtBinding)
				: FilterDescriptor(shader), FilterWithSettings(shader, settingsBuffer), dirt(dirtBinding) {}
			inline virtual ~MixFilterDescriptor() {}

			inline virtual size_t TextureSamplerCount()const override { return 2u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { 
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) * 3u };
			}
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { 
				return (index == 0) ? source.operator->() : dirt->BoundObject();
			}
		};

		struct ThresholdSettings {
			alignas(4) float minIntensity = 0.75f;
			alignas(4) float invIntensityFade = 1.0f / std::numeric_limits<float>::epsilon();
		};

		struct UpsampleSettings {
			alignas(4) float baseWeight = 1.0f;
			alignas(4) float bloomWeight = 1.0f;
		};

		struct MixSettings {
			alignas(4) float bloomStrength = 1.0f;
			alignas(4) float dirtStrength = 1.0f;
			alignas(8) Vector2 dirtScale = Vector2(1.0f);
			alignas(8) Vector2 dirtOffset = Vector2(0.0f);
		};



		template<typename FilterType>
		struct FilterPipeline {
			Reference<FilterType> descriptor;
			Reference<Graphics::Pipeline> pipeline;
		};

		struct MipFilters {
			FilterPipeline<FilterDescriptor> downsample;
			FilterPipeline<FilterWithSettings> upsample;
		};

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> graphicsDevice;
			const Reference<Graphics::Shader> thresholdShader;
			const Reference<Graphics::Shader> downsampleShader;
			const Reference<Graphics::Shader> upsampleShader;
			const Reference<Graphics::Shader> mixShader;
			const size_t maxInFlightCommandBuffers;

			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> blackTexture;
			const Reference<Graphics::ShaderResourceBindings::TextureSamplerBinding> dirtBinding;

			struct {
				float strength = 0.5f;
				float size = 0.25f;
				float threshold = 1.0f;
				float thresholdSize = 0.1f;

				float dirtStrength = 1.0f;
				Vector2 dirtTiling = Vector2(1.0f);
				Vector2 dirtOffset = Vector2(0.0f);
			} settings;
			const Graphics::BufferReference<ThresholdSettings> thresholdSettings;
			const Graphics::BufferReference<UpsampleSettings> upscaleSettings;
			const Graphics::BufferReference<MixSettings> mixSettings;

			struct {
				Reference<Graphics::TextureSampler> sourceImage;
				Reference<Graphics::TextureView> resultView;
			} textures;

			struct {
				Reference<TransientImage> intermediateImage;
				std::vector<Reference<Graphics::TextureSampler>> intermediateImageSamplers;
				std::vector<MipFilters> filters;
			} kernels;

			inline Data(
				Graphics::GraphicsDevice* device,
				Graphics::Shader* threshold, Graphics::Shader* downsample, Graphics::Shader* upsample, Graphics::Shader* mix,
				size_t maxCommandBuffers)
				: graphicsDevice(device)
				, thresholdShader(threshold), downsampleShader(downsample), upsampleShader(upsample), mixShader(mix)
				, maxInFlightCommandBuffers(maxCommandBuffers)
				, blackTexture(Graphics::ShaderClass::SharedTextureSamplerBinding(Vector4(0.0f), device))
				, dirtBinding(Object::Instantiate<Graphics::ShaderResourceBindings::TextureSamplerBinding>())
				, thresholdSettings(device->CreateConstantBuffer<ThresholdSettings>())
				, upscaleSettings(device->CreateConstantBuffer<UpsampleSettings>())
				, mixSettings(device->CreateConstantBuffer<MixSettings>()) {
				dirtBinding->BoundObject() = blackTexture->BoundObject();
				ApplySettings(settings.strength, settings.size, settings.threshold, settings.thresholdSize);
			}

			inline void UpdateMixBuffer() {
				const Size2 targetSize = (kernels.intermediateImage == nullptr) ? Size2(0u) : Size2(kernels.intermediateImage->Texture()->Size());
				const Size2 dirtSize = dirtBinding->BoundObject()->TargetView()->TargetTexture()->Size();

				auto aspectRatio = [](const Size2& size) {
					return Math::Max(static_cast<float>(size.x), 1.0f) / Math::Max(static_cast<float>(size.y), 1.0f);
				};
				const float targetAspect = aspectRatio(targetSize);
				const float dirtAspect = aspectRatio(dirtSize);

				const Vector2 baseScale = (targetAspect > dirtAspect)
					? Vector2(1.0f, dirtAspect / targetAspect)
					: Vector2(targetAspect / dirtAspect, 1.0f);
				const Vector2 baseOffset = (1.0f - baseScale) * 0.5f;

				MixSettings mix = {};
				mix.bloomStrength = (settings.strength * 2.0f * settings.size);
				mix.dirtStrength = settings.dirtStrength * mix.bloomStrength;
				mix.dirtScale = baseScale * settings.dirtTiling;
				mix.dirtOffset = baseOffset + settings.dirtOffset;
				
				mixSettings.Map() = mix;
				mixSettings->Unmap(true);
			}

			inline void ApplySettings(float strength, float size, float threshold, float thresholdSize) {
				{
					settings.strength = strength;
					settings.size = size;
					settings.threshold = threshold;
					settings.thresholdSize = thresholdSize;
				}
				{
					ThresholdSettings settings = {};
					settings.minIntensity = threshold + Math::Min(thresholdSize, 0.0f);
					settings.invIntensityFade = 1.0f / Math::Max(std::abs(thresholdSize), std::numeric_limits<float>::epsilon());
					thresholdSettings.Map() = settings;
					thresholdSettings->Unmap(true);
				}
				{
					UpsampleSettings upscale = {};
					upscale.baseWeight = (1.0f - size);
					upscale.bloomWeight = size;
					upscaleSettings.Map() = upscale;
					upscaleSettings->Unmap(true);
				}
				UpdateMixBuffer();
			}

			inline void Clear() {
				// Clear textures:
				{
					textures.sourceImage = nullptr;
					textures.resultView = nullptr;
				}

				// Clear kernels
				{
					kernels.intermediateImage = nullptr;
					kernels.intermediateImageSamplers.clear();
					for (size_t i = 0; i < kernels.filters.size(); i++) {
						auto& entries = kernels.filters[i];
						{
							entries.downsample.descriptor->source = nullptr;
							entries.downsample.descriptor->result = nullptr;
						}
						{
							entries.upsample.descriptor->source = nullptr;
							entries.upsample.descriptor->result = nullptr;
						}
					}
				}
			}

			inline bool SetTextureSize(Size2 size) {
				if (kernels.intermediateImage != nullptr && kernels.intermediateImage->Texture()->Size() == Size3(size, 1u)) return true;

				auto fail = [&](const auto&... message) {
					kernels.intermediateImageSamplers.clear();
					kernels.intermediateImage = nullptr;
					graphicsDevice->Log()->Error("BloomKernel::Helpers::Data::SetTextureSize - ", message...);
					return false;
				};

				kernels.intermediateImageSamplers.clear();
				kernels.intermediateImage = TransientImage::Get(
					graphicsDevice,
					Graphics::Texture::TextureType::TEXTURE_2D,
					Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT, //result->TargetTexture()->ImageFormat(), 
					Size3(size, 1u), 1u, true);
				if (kernels.intermediateImage == nullptr)
					return fail("Failed to get transient image! [File:", __FILE__, "; Line: ", __LINE__, "]");
				
				Graphics::Texture* intemediateTexture = kernels.intermediateImage->Texture();
				if (intemediateTexture == nullptr)
					return fail("Failed to get texture from the transient image! [File:", __FILE__, "; Line: ", __LINE__, "]");

				uint32_t mipIndex = 0;
				while (Math::Min(size.x, size.y) >= MinMipSize() && mipIndex < intemediateTexture->MipLevels()) {
					const Reference<Graphics::TextureView> view = intemediateTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D, mipIndex, 1);
					if (view == nullptr) 
						return fail("Failed to create TextureView for mip ", mipIndex, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
					
					const Reference<Graphics::TextureSampler> sampler = view->CreateSampler(
						Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::CLAMP_TO_BORDER);
					if (sampler == nullptr)
						return fail("Failed to create TextureSampler for mip ", mipIndex, "! [File:", __FILE__, "; Line: ", __LINE__, "]");

					kernels.intermediateImageSamplers.push_back(sampler);
					mipIndex++;
					size /= 2u;
				}

				return true;
			}

			inline bool RefreshFilterKernels() {
				auto fail = [&](const auto&... message) {
					kernels.filters.clear();
					graphicsDevice->Log()->Error("BloomKernel::Helpers::Data::RefreshFilterKernels - ", message...);
					return false;
				};

				size_t mipIndex = 1u;
				while (mipIndex < kernels.intermediateImageSamplers.size()) {
					while (kernels.filters.size() < mipIndex) {
						MipFilters mipFilters = {};

						if (kernels.filters.empty()) {
							mipFilters.downsample.descriptor = Object::Instantiate<FilterWithSettings>(thresholdShader, thresholdSettings);
							mipFilters.upsample.descriptor = Object::Instantiate<MixFilterDescriptor>(mixShader, mixSettings, dirtBinding);
						}
						else {
							mipFilters.downsample.descriptor = Object::Instantiate<FilterDescriptor>(downsampleShader);
							mipFilters.upsample.descriptor = Object::Instantiate<FilterWithSettings>(upsampleShader, upscaleSettings);
						}

						mipFilters.downsample.pipeline = graphicsDevice->CreateComputePipeline(mipFilters.downsample.descriptor, maxInFlightCommandBuffers);
						if (mipFilters.downsample.pipeline == nullptr)
							return fail("Failed to downasmple pipeline for mip ", kernels.filters.size(), "![File:", __FILE__, "; Line: ", __LINE__, "]");

						mipFilters.upsample.pipeline = graphicsDevice->CreateComputePipeline(mipFilters.upsample.descriptor, maxInFlightCommandBuffers);
						if (mipFilters.upsample.pipeline == nullptr)
							return fail("Failed to upsample pipeline for mip ", kernels.filters.size(), "![File:", __FILE__, "; Line: ", __LINE__, "]");

						kernels.filters.push_back(mipFilters);
					}
					MipFilters& filters = kernels.filters[mipIndex - 1];
					Graphics::TextureSampler* bigMip = kernels.intermediateImageSamplers[mipIndex - 1];
					Graphics::TextureSampler* smallMip = kernels.intermediateImageSamplers[mipIndex];
					filters.downsample.descriptor->source = bigMip;
					filters.downsample.descriptor->result = smallMip->TargetView();
					filters.upsample.descriptor->source = smallMip;
					filters.upsample.descriptor->result = bigMip->TargetView();
					mipIndex++;
				}

				while (mipIndex < kernels.filters.size()) {
					MipFilters& filters = kernels.filters[mipIndex - 1];
					filters.downsample.descriptor->source = nullptr;
					filters.downsample.descriptor->result = nullptr;
					filters.upsample.descriptor->source = nullptr;
					filters.upsample.descriptor->result = nullptr;
					mipIndex++;
				}

				return true;
			}
		};
	};



	Reference<BloomKernel> BloomKernel::Create(
		Graphics::GraphicsDevice* device, 
		Graphics::ShaderLoader* shaderLoader, 
		size_t maxInFlightCommandBuffers) {
		if (device == nullptr) return nullptr;

		auto error = [&](const auto&... args) {
			device->Log()->Error("BloomKernel::Create - ", args...);
			return nullptr;
		};

		if (shaderLoader == nullptr)
			return error("Shader Loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		if (maxInFlightCommandBuffers <= 0u)
			return error("maxInFlightCommandBuffers must be greater than 0! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		if (shaderSet == nullptr)
			return error("Shader Loader failed to load shader sert for compute modules! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
		if (shaderCache == nullptr)
			return error("Shader cache does no exist for the given device! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		auto loadShader = [&](const Graphics::ShaderClass* shaderClass) -> Reference<Graphics::Shader> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr) 
				return error("Failed to load SPIRV binary for '", ((std::string)shaderClass->ShaderPath()), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
			if (shader == nullptr)
				return error("Failed get/create shader module for '", ((std::string)shaderClass->ShaderPath()), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return shader;
		};

		static const OS::Path basePath = "Jimara/Environment/Rendering/PostFX/Bloom";

		static const Graphics::ShaderClass THRESHOLD_SHADER_CLASS(basePath / OS::Path("BloomKernel_Threshold"));
		const Reference<Graphics::Shader> threshold = loadShader(&THRESHOLD_SHADER_CLASS);
		if (threshold == nullptr) return nullptr;

		static const Graphics::ShaderClass DOWNSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Downsample"));
		const Reference<Graphics::Shader> downsample = loadShader(&DOWNSAMPLE_SHADER_CLASS);
		if (downsample == nullptr) return nullptr;

		static const Graphics::ShaderClass UPSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Upsample"));
		const Reference<Graphics::Shader> upsample = loadShader(&UPSAMPLE_SHADER_CLASS);
		if (upsample == nullptr) return nullptr;

		static const Graphics::ShaderClass MIX_SHADER_CLASS(basePath / OS::Path("BloomKernel_Mix"));
		const Reference<Graphics::Shader> mix = loadShader(&MIX_SHADER_CLASS);
		if (mix == nullptr) return nullptr;

		Reference<BloomKernel> bloomKernel = new BloomKernel(device, threshold, downsample, upsample, mix, maxInFlightCommandBuffers);
		bloomKernel->ReleaseRef();
		return bloomKernel;
	}

	BloomKernel::BloomKernel(
		Graphics::GraphicsDevice* device,
		Graphics::Shader* threshold, Graphics::Shader* downsample, Graphics::Shader* upsample, Graphics::Shader* mix,
		size_t maxInFlightCommandBuffers) 
		: m_data(Object::Instantiate<Helpers::Data>(device, threshold, downsample, upsample, mix, maxInFlightCommandBuffers)) {}

	BloomKernel::~BloomKernel() {}

	void BloomKernel::Configure(float strength, float size, float threshold, float thresholdSize) {
		// Process input values:
		{
			size = Math::Min(Math::Max(0.0f, size), 1.0f);
			strength = Math::Max(0.0f, strength);
		}

		// Check if changed:
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (data->settings.strength == strength &&
			data->settings.size == size && 
			data->settings.threshold == threshold && 
			data->settings.thresholdSize == thresholdSize) return;

		// Update buffer:
		data->ApplySettings(strength, size, threshold, thresholdSize);
	}

	void BloomKernel::SetDirtTexture(Graphics::TextureSampler* image, float strength, const Vector2& tiling, const Vector2& offset) {
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (image == nullptr) image = data->blackTexture->BoundObject();

		// Check if changed:
		if (data->dirtBinding->BoundObject() == image &&
			data->settings.dirtStrength == strength &&
			data->settings.dirtTiling == tiling &&
			data->settings.dirtOffset == offset) return;

		// Set new values and apply:
		data->dirtBinding->BoundObject() = image;
		data->settings.dirtStrength = strength;
		data->settings.dirtTiling = tiling;
		data->settings.dirtOffset = offset;
		data->UpdateMixBuffer();
	}

	void BloomKernel::SetTarget(Graphics::TextureSampler* image) {
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (image == nullptr) {
			data->Clear();
			return;
		}
		if (data->textures.sourceImage == image)
			return;
		data->textures.sourceImage = image;
		data->textures.resultView = image->TargetView();
		data->SetTextureSize(data->textures.resultView->TargetTexture()->Size());
		data->RefreshFilterKernels();
		data->UpdateMixBuffer();
	}

	void BloomKernel::Execute(Graphics::InFlightBufferInfo commandBuffer) {
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		const auto& textures = data->textures;
		const auto& kernels = data->kernels;
		if (textures.resultView == nullptr ||
			kernels.intermediateImage == nullptr ||
			kernels.intermediateImageSamplers.size() <= 0 ||
			kernels.filters.size() <= 0u) return;
		
		// Update src & result textures:
		{
			const auto& filters = kernels.filters[0];
			filters.downsample.descriptor->source = textures.sourceImage;
			filters.upsample.descriptor->result = textures.resultView;
		}

		const size_t filterCount = kernels.intermediateImageSamplers.size() - 1u;

		// Downsample:
		for (size_t i = 0; i < filterCount; i++)
			kernels.filters[i].downsample.pipeline->Execute(commandBuffer);

		// Upsample:
		for (size_t i = filterCount; i-- > 0;)
			kernels.filters[i].upsample.pipeline->Execute(commandBuffer);
	}
}
