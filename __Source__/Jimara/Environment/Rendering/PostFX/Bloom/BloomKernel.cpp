#include "BloomKernel.h"
#include "../../TransientImage.h"


namespace Jimara {
	struct BloomKernel::Helpers {
		static const constexpr uint32_t BlockSize() { return 16u; }
		inline static constexpr uint32_t MinMipSize() { return 1; }



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
			inline virtual Size3 NumBlocks() override {
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



		struct UpsampleFilter : public virtual FilterDescriptor {
			struct Settings {
				alignas(4) float spread = 1.0f;
				alignas(4) float strength = 0.5f;
			};

			const Graphics::BufferReference<Settings> radius;
			Reference<Graphics::TextureView> bigMip;

			inline UpsampleFilter(Graphics::Shader* shader, Graphics::Buffer* radiusBuffer) 
				: FilterDescriptor(shader), radius(radiusBuffer) {}
			inline virtual ~UpsampleFilter() {}

			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 3u }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return radius; }

			inline virtual size_t TextureViewCount()const override { return 2u; }
			inline virtual BindingInfo TextureViewInfo(size_t index)const override {
				return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) + 1u };
			}
			inline virtual Reference<Graphics::TextureView> View(size_t index)const override { 
				return (index == 0) ? result : bigMip;
			}
		};



		template<typename FilterType>
		struct FilterPipeline {
			Reference<FilterType> descriptor;
			Reference<Graphics::Pipeline> pipeline;
		};

		struct MipFilters {
			FilterPipeline<FilterDescriptor> downsample;
			FilterPipeline<UpsampleFilter> upsample;
		};

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> graphicsDevice;
			const Reference<Graphics::Shader> downsampleShader;
			const Reference<Graphics::Shader> upsampleShader;
			const size_t maxInFlightCommandBuffers;

			UpsampleFilter::Settings upscaleSettings;
			const Graphics::BufferReference<UpsampleFilter::Settings> upscaleSettingsBuffer;

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
				Graphics::Shader* downsample, Graphics::Shader* upsample,
				size_t maxCommandBuffers)
				: graphicsDevice(device)
				, downsampleShader(downsample), upsampleShader(upsample)
				, maxInFlightCommandBuffers(maxCommandBuffers)
				, upscaleSettingsBuffer(device->CreateConstantBuffer<UpsampleFilter::Settings>()) {
				upscaleSettingsBuffer.Map() = upscaleSettings;
				upscaleSettingsBuffer->Unmap(true);
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
							entries.upsample.descriptor->bigMip = nullptr;
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
				while (Math::Max(size.x, size.y) >= MinMipSize() && mipIndex < intemediateTexture->MipLevels()) {
					const Reference<Graphics::TextureView> view = intemediateTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D, mipIndex, 1);
					if (view == nullptr) 
						return fail("Failed to create TextureView for mip ", mipIndex, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
					
					const Reference<Graphics::TextureSampler> sampler = view->CreateSampler(
						Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::CLAMP_TO_EDGE);
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

						mipFilters.downsample.descriptor = Object::Instantiate<FilterDescriptor>(downsampleShader);
						mipFilters.upsample.descriptor = Object::Instantiate<UpsampleFilter>(upsampleShader, upscaleSettingsBuffer);

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
					filters.upsample.descriptor->bigMip = bigMip->TargetView();
					mipIndex++;
				}

				while (mipIndex < kernels.filters.size()) {
					MipFilters& filters = kernels.filters[mipIndex - 1];
					filters.downsample.descriptor->source = nullptr;
					filters.downsample.descriptor->result = nullptr;
					filters.upsample.descriptor->source = nullptr;
					filters.upsample.descriptor->result = nullptr;
					filters.upsample.descriptor->bigMip = nullptr;
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

		static const Graphics::ShaderClass DOWNSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Downsample"));
		const Reference<Graphics::Shader> downsample = loadShader(&DOWNSAMPLE_SHADER_CLASS);
		if (downsample == nullptr) return nullptr;

		static const Graphics::ShaderClass UPSAMPLE_SHADER_CLASS(basePath / OS::Path("BloomKernel_Upsample"));
		const Reference<Graphics::Shader> upsample = loadShader(&UPSAMPLE_SHADER_CLASS);
		if (upsample == nullptr) return nullptr;

		Reference<BloomKernel> bloomKernel = new BloomKernel(device, downsample, upsample, maxInFlightCommandBuffers);
		bloomKernel->ReleaseRef();
		return bloomKernel;
	}

	BloomKernel::BloomKernel(
		Graphics::GraphicsDevice* device,
		Graphics::Shader* downsample, Graphics::Shader* upsample,
		size_t maxInFlightCommandBuffers) 
		: m_data(Object::Instantiate<Helpers::Data>(device, downsample, upsample, maxInFlightCommandBuffers)) {}

	BloomKernel::~BloomKernel() {}

	void BloomKernel::Configure(float spread, float strength) {
		// Process input values:
		{
			spread = Math::Max(0.0f, spread);
			strength = Math::Min(Math::Max(0.0f, strength), 1.0f);
		}

		// Check if changed:
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (data->upscaleSettings.spread == spread &&
			data->upscaleSettings.strength == strength) return;

		// Update settings:
		{
			data->upscaleSettings.spread = spread;
			data->upscaleSettings.strength = strength;
		}

		// Update buffer:
		{
			data->upscaleSettingsBuffer.Map() = data->upscaleSettings;
			data->upscaleSettingsBuffer->Unmap(true);
		}
	}

	void BloomKernel::SetTextures(Graphics::TextureSampler* source, Graphics::TextureView* destination) {
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (source == nullptr) {
			data->Clear();
			return;
		}
		if (destination == nullptr)
			destination = source->TargetView();
		if (data->textures.sourceImage == source && data->textures.resultView == destination)
			return;
		data->textures.sourceImage = source;
		data->textures.resultView = destination;
		data->SetTextureSize(destination->TargetTexture()->Size());
		data->RefreshFilterKernels();
	}

	void BloomKernel::Execute(Graphics::Pipeline::CommandBufferInfo commandBuffer) {
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
			filters.upsample.descriptor->bigMip = textures.sourceImage->TargetView();
		}

		const size_t filterCount = kernels.intermediateImageSamplers.size() - 1u;

		// Downsample:
		for (size_t i = 0; i < filterCount; i++)
			kernels.filters[i].downsample.pipeline->Execute(commandBuffer);

		// Upsample:
		for (size_t i = (filterCount - 1); i-- > 0;)
			kernels.filters[i].upsample.pipeline->Execute(commandBuffer);
	}
}
