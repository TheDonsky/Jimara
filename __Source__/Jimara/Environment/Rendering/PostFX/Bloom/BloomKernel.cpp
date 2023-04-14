#include "BloomKernel.h"
#include "../../TransientImage.h"


namespace Jimara {
	struct BloomKernel::Helpers {
		static const constexpr uint32_t BlockSize() { return 16u; }
		inline static constexpr uint32_t MinMipSize() { return 2; }

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


		struct PipelineWithSet {
			Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBuffer;
			Reference<Graphics::BindingSet> bindingSet;
			Reference<Graphics::ComputePipeline> pipeline;
			Size3 numBlocks = Size3(0u);

			inline void Dispatch(Graphics::InFlightBufferInfo bufferInfo)const {
				bindingSet->Update(bufferInfo);
				bindingSet->Bind(bufferInfo);
				pipeline->Dispatch(bufferInfo, numBlocks);
			}
		};

		struct MipFilters {
			PipelineWithSet downsample;
			PipelineWithSet upsample;
		};

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> graphicsDevice;
			const Reference<Graphics::BindingPool> bindingPool;
			const Reference<Graphics::ComputePipeline> thresholdPipeline;
			const Reference<Graphics::ComputePipeline> downsamplePipeline;
			const Reference<Graphics::ComputePipeline> upsamplePipeline;
			const Reference<Graphics::ComputePipeline> mixPipeline;

			const Reference<const Graphics::ShaderClass::TextureSamplerBinding> blackTexture;
			const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> dirtBinding;

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
				const Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> sourceImage = 
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>();
				const Reference<Graphics::ResourceBinding<Graphics::TextureView>> resultView =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
			} textures;

			struct {
				Reference<TransientImage> intermediateImage;
				std::vector<Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>>> intermediateImageSamplers;
				std::vector<MipFilters> filters;
			} kernels;

			inline Data(
				Graphics::GraphicsDevice* device,
				Graphics::BindingPool* bindingSetPool, 
				Graphics::ComputePipeline* threshold,
				Graphics::ComputePipeline* downsample,
				Graphics::ComputePipeline* upsample,
				Graphics::ComputePipeline* mix)
				: graphicsDevice(device)
				, bindingPool(bindingSetPool)
				, thresholdPipeline(threshold)
				, downsamplePipeline(downsample)
				, upsamplePipeline(upsample)
				, mixPipeline(mix)
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
					textures.sourceImage->BoundObject() = nullptr;
					textures.resultView->BoundObject() = nullptr;
				}

				// Clear kernels
				{
					kernels.intermediateImage = nullptr;
					kernels.intermediateImageSamplers.clear();
					for (size_t i = 0; i < kernels.filters.size(); i++) {
						auto& entries = kernels.filters[i];
						entries.downsample.bindingSet = nullptr;
						entries.upsample.bindingSet = nullptr;
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

					kernels.intermediateImageSamplers.push_back(Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(sampler));
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
							mipFilters.downsample.settingsBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(thresholdSettings);
							mipFilters.downsample.pipeline = thresholdPipeline;

							mipFilters.upsample.settingsBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(mixSettings);
							mipFilters.upsample.pipeline = mixPipeline;
						}
						else {
							mipFilters.downsample.pipeline = downsamplePipeline;

							if (kernels.filters.size() <= 1u)
								mipFilters.upsample.settingsBuffer = Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(upscaleSettings);
							else mipFilters.upsample.settingsBuffer = kernels.filters.back().upsample.settingsBuffer;
							mipFilters.upsample.pipeline = upsamplePipeline;
						}

						kernels.filters.push_back(mipFilters);
					}
					const size_t prevMipIndex = mipIndex - 1u;
					MipFilters& filters = kernels.filters[prevMipIndex];

					const Graphics::ResourceBinding<Graphics::TextureSampler>* bigMip = (prevMipIndex > 0u)
						? kernels.intermediateImageSamplers[prevMipIndex].operator->() : textures.sourceImage.operator->();
					const Reference<const Graphics::ResourceBinding<Graphics::TextureView>> bigMipView = [&]() 
						-> Reference<const Graphics::ResourceBinding<Graphics::TextureView>> {
						if (prevMipIndex > 0u)
							return Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>(bigMip->BoundObject()->TargetView());
						else return textures.resultView;
					}();
					const Graphics::ResourceBinding<Graphics::TextureSampler>* smallMip = kernels.intermediateImageSamplers[mipIndex];
					const Reference<const Graphics::ResourceBinding<Graphics::TextureView>> smallMipView =
						Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>(smallMip->BoundObject()->TargetView());

					auto createDescriptorSet = [&](
						PipelineWithSet& set, 
						const Graphics::ResourceBinding<Graphics::TextureSampler>* source,
						const Graphics::ResourceBinding<Graphics::TextureView>* result) -> bool {
						auto searchSettingsBuffer = [&](const auto&) { return set.settingsBuffer; };
						auto searchTextureSampler = [&](const Graphics::BindingSet::BindingDescriptor& desc) 
							-> const Graphics::ResourceBinding<Graphics::TextureSampler>* {
							if (desc.setBindingIndex == 0u) return source;
							else if (desc.setBindingIndex == 3u) return dirtBinding;
							else return nullptr;
						};
						auto searchTextureView = [&](const auto& desc) { return result; };
						Graphics::BindingSet::Descriptor setDescriptor = {};
						{
							setDescriptor.pipeline = set.pipeline;
							setDescriptor.bindingSetId = 0u;
							setDescriptor.find.constantBuffer = &searchSettingsBuffer;
							setDescriptor.find.textureSampler = &searchTextureSampler;
							setDescriptor.find.textureView = &searchTextureView;
						}
						set.bindingSet = bindingPool->AllocateBindingSet(setDescriptor);
						if (set.bindingSet == nullptr)
							return fail("Failed to create binding set! [File:", __FILE__, "; Line: ", __LINE__, "]");
						const Size3 size = (result == nullptr) ? Size3(0u) : result->BoundObject()->TargetTexture()->Size();
						auto blockCount = [](uint32_t width) { return (width + BlockSize() - 1) / BlockSize(); };
						set.numBlocks = Size3(blockCount(size.x), blockCount(size.y), 1);
						return true;
					};
					if (!createDescriptorSet(filters.downsample, bigMip, smallMipView)) return false;
					if (!createDescriptorSet(filters.upsample, smallMip, bigMipView)) return false;
					mipIndex++;
				}

				while (mipIndex <= kernels.filters.size()) {
					MipFilters& filters = kernels.filters[mipIndex - 1];
					filters.downsample.bindingSet = nullptr;
					filters.upsample.bindingSet = nullptr;
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

		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr) 
			return error("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		auto loadShader = [&](const Graphics::ShaderClass* shaderClass) -> Reference<Graphics::ComputePipeline> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr)
				return error("Failed to load SPIRV binary for '", ((std::string)shaderClass->ShaderPath()), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(binary);
			if (pipeline == nullptr)
				return error("Failed get/create graphics pipeline for '", ((std::string)shaderClass->ShaderPath()), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			if (pipeline->BindingSetCount() != 1u)
				return error("Pipeline for '", ((std::string)shaderClass->ShaderPath()), 
					"' expected to require exactly 1 binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return pipeline;
		};

		static const OS::Path basePath = "Jimara/Environment/Rendering/PostFX/Bloom";

		static const Graphics::ShaderClass THRESHOLD_SHADER_CLASS(basePath / OS::Path("BloomKernel_Threshold"));
		const auto threshold = loadShader(&THRESHOLD_SHADER_CLASS);
		if (threshold == nullptr) return nullptr;

		static const Graphics::ShaderClass DOWNSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Downsample"));
		const auto downsample = loadShader(&DOWNSAMPLE_SHADER_CLASS);
		if (downsample == nullptr) return nullptr;

		static const Graphics::ShaderClass UPSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Upsample"));
		const auto upsample = loadShader(&UPSAMPLE_SHADER_CLASS);
		if (upsample == nullptr) return nullptr;

		static const Graphics::ShaderClass MIX_SHADER_CLASS(basePath / OS::Path("BloomKernel_Mix"));
		const auto mix = loadShader(&MIX_SHADER_CLASS);
		if (mix == nullptr) return nullptr;

		Reference<BloomKernel> bloomKernel = new BloomKernel(device, bindingPool, threshold, downsample, upsample, mix);
		bloomKernel->ReleaseRef();
		return bloomKernel;
	}

	BloomKernel::BloomKernel(
		Graphics::GraphicsDevice* device,
		Graphics::BindingPool* bindingPool,
		Graphics::ComputePipeline* threshold,
		Graphics::ComputePipeline* downsample,
		Graphics::ComputePipeline* upsample,
		Graphics::ComputePipeline* mix) 
		: m_data(Object::Instantiate<Helpers::Data>(device, bindingPool, threshold, downsample, upsample, mix)) {}

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
		if (data->textures.sourceImage->BoundObject() == image)
			return;
		data->textures.sourceImage->BoundObject() = image;
		data->textures.resultView->BoundObject() = image->TargetView();
		data->SetTextureSize(data->textures.resultView->BoundObject()->TargetTexture()->Size());
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
		
		const size_t filterCount = kernels.intermediateImageSamplers.size() - 1u;

		// Downsample:
		for (size_t i = 0; i < filterCount; i++)
			kernels.filters[i].downsample.Dispatch(commandBuffer);

		// Upsample:
		for (size_t i = filterCount; i-- > 0;)
			kernels.filters[i].upsample.Dispatch(commandBuffer);
	}
}
