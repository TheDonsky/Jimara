#include "BloomKernel.h"
#include "../../TransientImage.h"


namespace Jimara {
	struct BloomKernel::Helpers {
		static const constexpr uint32_t BlockSize() { return 16u; }
		inline static constexpr uint32_t MinMipSize() { return 8; }



		struct DownsampleFilter 
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			const Reference<Graphics::Shader> downsampleShader;
			Reference<Graphics::TextureSampler> source;
			Reference<Graphics::TextureView> result;

			inline DownsampleFilter(Graphics::Shader* shader) : downsampleShader(shader) {}
			inline virtual ~DownsampleFilter() {}


			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return downsampleShader; }
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



		struct UpsampleFilter
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			const Reference<Graphics::Shader> upsampleShader;
			const Graphics::BufferReference<float> radius;
			Reference<Graphics::TextureSampler> source;
			Reference<Graphics::TextureView> result;

			inline UpsampleFilter(Graphics::Shader* shader, Graphics::Buffer* radiusBuffer) 
				: upsampleShader(shader), radius(radiusBuffer) {}
			inline virtual ~UpsampleFilter() {}


			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return upsampleShader; }
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

			inline virtual size_t ConstantBufferCount()const override { return 1u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0u }; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return radius; }

			inline virtual size_t StructuredBufferCount()const override { return 0u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return {}; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }

			inline virtual size_t TextureSamplerCount()const override { return 1u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 1u }; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return source; }

			inline virtual size_t TextureViewCount()const override { return 1u; }
			inline virtual BindingInfo TextureViewInfo(size_t index)const override { return { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 2u }; }
			inline virtual Reference<Graphics::TextureView> View(size_t index)const override { return result; }
		};



		template<typename FilterType>
		struct FilterPipeline {
			Reference<FilterType> descriptor;
			Reference<Graphics::Pipeline> pipeline;

			inline void Clear() {
				descriptor->source = nullptr;
				descriptor->result = nullptr;
			}

			inline void SetTargets(Graphics::TextureSampler* source, Graphics::TextureSampler* result) {
				descriptor->source = source;
				descriptor->result = result;
			}
		};

		struct MipFilters {
			FilterPipeline<DownsampleFilter> downsample;
			FilterPipeline<UpsampleFilter> upsample;
		};

		struct Data : public virtual Object {
			const Reference<Graphics::GraphicsDevice> graphicsDevice;
			const Reference<Graphics::Shader> downsampleShader;
			const Reference<Graphics::Shader> upsampleShader;
			const size_t maxInFlightCommandBuffers;

			Reference<TransientImage> intermediateImage;
			std::vector<MipFilters> filters;

			inline Data(
				Graphics::GraphicsDevice* device,
				Graphics::Shader* downsample, Graphics::Shader* upsample,
				size_t maxCommandBuffers)
				: graphicsDevice(device)
				, downsampleShader(downsample), upsampleShader(upsample)
				, maxInFlightCommandBuffers(maxCommandBuffers) {}

			inline void Clear() {
				intermediateImage = nullptr;

				for (size_t i = 0; i < filters.size(); i++) {
					auto& entries = filters[i];
					entries.downsample.Clear();
					entries.upsample.Clear();
				}
			}

			inline void BindTargets(Graphics::TextureSampler* source, Graphics::TextureView* result) {
				Size3 size = result->TargetTexture()->Size();
				if (intermediateImage == nullptr || intermediateImage->Texture()->Size() != size)
					Clear();

				const bool intermediateImageIsNew = (intermediateImage == nullptr);
				if (intermediateImageIsNew) {
					intermediateImage = TransientImage::Get(
						graphicsDevice,
						Graphics::Texture::TextureType::TEXTURE_2D,
						Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT, //result->TargetTexture()->ImageFormat(), 
						size, 1u, true);
					if (intermediateImage == nullptr) return;
				}

				Graphics::Texture* intemediateTexture = intermediateImage->Texture();
				if (intemediateTexture == nullptr) return;

				uint32_t mipIndex = 1u;

				Reference<Graphics::TextureView> mipView = result;
				Reference<Graphics::TextureSampler> mipSampler = source;

				auto getMipFilters = [&](uint32_t mip) -> MipFilters& {
					auto fail = [&](const auto&... message) -> MipFilters& {
						graphicsDevice->Log()->Error("BloomKernel::Helpers::Data::BindTargets<getMipFilters> - ", message...);
						Clear();
						static MipFilters NO_FILTERS;
						return NO_FILTERS;
					};

					while (filters.size() < mip) {
						MipFilters mipFilters = {};

						Graphics::BufferReference<float> radiusBuffer = graphicsDevice->CreateConstantBuffer<float>();
						radiusBuffer.Map() = 1.0f;
						radiusBuffer->Unmap(true);
						
						mipFilters.downsample.descriptor = Object::Instantiate<DownsampleFilter>(downsampleShader);
						mipFilters.upsample.descriptor = Object::Instantiate<UpsampleFilter>(upsampleShader, radiusBuffer);

						mipFilters.downsample.pipeline = graphicsDevice->CreateComputePipeline(mipFilters.downsample.descriptor, maxInFlightCommandBuffers);
						if (mipFilters.downsample.pipeline == nullptr)
							return fail("Failed to downasmple pipeline for mip ", filters.size(), "![File:", __FILE__, "; Line: ", __LINE__, "]");

						mipFilters.upsample.pipeline = graphicsDevice->CreateComputePipeline(mipFilters.upsample.descriptor, maxInFlightCommandBuffers);
						if (mipFilters.upsample.pipeline == nullptr)
							return fail("Failed to upsample pipeline for mip ", filters.size(), "![File:", __FILE__, "; Line: ", __LINE__, "]");

						filters.push_back(mipFilters);
					}
					return filters[mip - 1];
				};

				auto setArgs = [&](Graphics::TextureSampler* srcLargeMip, Graphics::TextureSampler* smallMip, Graphics::TextureView* dstMip) {
					MipFilters& mipFilters = getMipFilters(mipIndex);
					if (mipFilters.downsample.descriptor == nullptr) return false;

					mipFilters.downsample.descriptor->source = srcLargeMip;
					mipFilters.downsample.descriptor->result = smallMip->TargetView();

					mipFilters.upsample.descriptor->source = smallMip;
					mipFilters.upsample.descriptor->result = dstMip;

					return true;
				};

				auto refreshMipSampler = [&]() {
					auto fail = [&](const auto&... message) -> bool {
						graphicsDevice->Log()->Error("BloomKernel::Helpers::Data::BindTargets<refreshMipSampler> - ", message...);
						Clear();
						return false;
					};

					mipView = intemediateTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D, mipIndex, 1u);
					if (mipView == nullptr)
						return fail("Failed to create a view for mip ", mipIndex, "![File:", __FILE__, "; Line: ", __LINE__, "]");

					mipSampler = mipView->CreateSampler(Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::CLAMP_TO_EDGE);
					if (mipSampler == nullptr)
						return fail("BloomKernel::Helpers::Data::BindTargets - Failed to create a sampler for mip ", mipIndex, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					
					return true;
				};

				auto nextMip = [&]() {
					size /= 2u;
					mipIndex++;
				};

				if ((!intermediateImageIsNew) && filters.size() > 0u) {
					MipFilters& mipFilters = getMipFilters(1u);
					setArgs(source, mipFilters.upsample.descriptor->source, result);
				}
				else while (Math::Min(size.x, size.y) >= MinMipSize() || mipIndex >= intemediateTexture->MipLevels()) {
					const Reference<Graphics::TextureSampler> higherMipSampler = mipSampler;
					const Reference<Graphics::TextureView> higherMipView = mipView;
					if (!refreshMipSampler()) break;
					if (!setArgs(higherMipSampler, mipSampler, higherMipView)) break;
					nextMip();
				}
			}

			inline void Execute(const Graphics::Pipeline::CommandBufferInfo& commandBuffer) {
				if (intermediateImage == nullptr) return;
				Graphics::Texture* intemediateTexture = intermediateImage->Texture();
				if (intemediateTexture == nullptr) return;

				uint32_t lastMip = intemediateTexture->MipLevels();
				if (lastMip <= 0) return;
				lastMip--;

				Size3 size = intemediateTexture->Size();
				uint32_t mipIndex = 0;

				while (Math::Min(size.x, size.y) >= MinMipSize() || mipIndex >= lastMip) {
					filters[mipIndex].downsample.pipeline->Execute(commandBuffer);
					size /= 2u;
					mipIndex++;
				}

				while (mipIndex > 0) {
					mipIndex--;
					filters[mipIndex].upsample.pipeline->Execute(commandBuffer);
				}
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

	void BloomKernel::Execute(
		Graphics::TextureSampler* source,
		Graphics::TextureView* destination,
		Graphics::Pipeline::CommandBufferInfo commandBuffer) {
		Helpers::Data* data = dynamic_cast<Helpers::Data*>(m_data.operator->());
		if (source == nullptr) {
			data->Clear();
			return;
		}
		if (destination == nullptr)
			destination = source->TargetView();
		data->BindTargets(source, destination);
		data->Execute(commandBuffer);
	}
}
