#include "SegmentTreeGenerationKernel.h"


namespace Jimara {
	struct SegmentTreeGenerationKernel::Helpers {
		struct BuildSettings {
			alignas(4) uint32_t layerSize = 0u;
			alignas(4) uint32_t layerStart = 0u;
		};
		static_assert(sizeof(BuildSettings) == 8u);

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet* baseBindings = nullptr;
			const std::string_view resultBufferBindingName;
			const std::string_view settingsBufferBindingName;
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> resultBufferBinding;
			const Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> settingsBufferBinding;

			inline BindingSet(
				const Graphics::ShaderResourceBindings::ShaderResourceBindingSet* base,
				const std::string_view& segmentTreeBufferBindingName, 
				const std::string_view& generationKernelSettingsName, Graphics::Buffer* settingsBuffer) 
				: baseBindings(base)
				, resultBufferBindingName(segmentTreeBufferBindingName)
				, settingsBufferBindingName(generationKernelSettingsName)
				, resultBufferBinding(Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>())
				, settingsBufferBinding(Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>(settingsBuffer)) {}
			inline virtual ~BindingSet() {}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == settingsBufferBindingName) return settingsBufferBinding;
				return baseBindings->FindConstantBufferBinding(name);
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == resultBufferBindingName) return resultBufferBinding;
				return baseBindings->FindStructuredBufferBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override { return baseBindings->FindTextureSamplerBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override { return baseBindings->FindTextureViewBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override { return baseBindings->FindBindlessStructuredBufferSetBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override { return baseBindings->FindBindlessTextureSamplerSetBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override { return baseBindings->FindBindlessTextureViewSetBinding(name); }
		};

		struct PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
			const Reference<Graphics::Shader> m_shader;
			const std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> m_bindingSets;
			Size3 m_kernelSize = Size3(0u);

			inline PipelineDescriptor(Graphics::Shader* shader, std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>>&& bindingSets)
				: m_shader(shader), m_bindingSets(std::move(bindingSets)) {}
			inline virtual ~PipelineDescriptor() {}

			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual size_t BindingSetCount()const override { return m_bindingSets.size(); }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const override { return m_bindingSets[index]; }
			inline virtual Size3 NumBlocks() override { return m_kernelSize; }
		};

		// __TODO__: Implement this crap!
	};

	SegmentTreeGenerationKernel::SegmentTreeGenerationKernel(
		Graphics::GraphicsDevice* device,
		Graphics::ShaderResourceBindings::StructuredBufferBinding* resultBufferBinding,
		Graphics::ComputePipeline::Descriptor* pipelineDescriptor,
		Graphics::Buffer* settingsBuffer,
		size_t workGroupSize) 
		: m_device(device)
		, m_resultBufferBinding(resultBufferBinding)
		, m_pipelineDescriptor(pipelineDescriptor)
		, m_settingsBuffer(settingsBuffer)
		, m_workGroupSize(workGroupSize) {}

	SegmentTreeGenerationKernel::~SegmentTreeGenerationKernel() {}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::Create(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
		const Graphics::ShaderClass* generationKernelShaderClass, size_t maxInFlightCommandBuffers, size_t workGroupSize,
		const std::string_view& segmentTreeBufferBindingName, const std::string_view& generationKernelSettingsName,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& additionalBindings) {
		if (device == nullptr) return nullptr;
		auto error = [&](const auto... message) { 
			device->Log()->Error("SegmentTreeGenerationKernel::Create - ", message...);
			return nullptr;
		};

		if (shaderLoader == nullptr) 
			return error("Shader Loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		if (generationKernelShaderClass == nullptr) 
			return error("Generation Kernel Shader Class not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		if (maxInFlightCommandBuffers <= 0u) maxInFlightCommandBuffers = 1u;
		{
			if (workGroupSize <= 0u) return error("Workgroup Size should be greater than 0! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			size_t groupSize = 1u;
			while (true) {
				if (groupSize == workGroupSize) break;
				else if (groupSize > workGroupSize) 
					return error("Workgroup Size has to be a power of two! (got ", workGroupSize, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
				else groupSize <<= 1u;
			}
		}

		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		if (shaderSet == nullptr) 
			return error("Failed to retrieve the shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::SPIRV_Binary> shaderBinary = shaderSet->GetShaderModule(generationKernelShaderClass, Graphics::PipelineStage::COMPUTE);
		if (shaderBinary == nullptr)
			return error("Failed to load shader binary for \"", generationKernelShaderClass->ShaderPath(), "\"! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
		if (shaderCache == nullptr)
			return error("Failed to retrieve the shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::Shader> shaderModule = shaderCache->GetShader(shaderBinary);
		if (shaderModule != nullptr)
			return error("Failed to create shader module for \"", generationKernelShaderClass->ShaderPath(), "\"! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Graphics::BufferReference<Helpers::BuildSettings> settingsBuffer = device->CreateConstantBuffer<Helpers::BuildSettings>();
		if (settingsBuffer == nullptr)
			return error("Failed to crete settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Helpers::BindingSet bindingSet(&additionalBindings, segmentTreeBufferBindingName, generationKernelSettingsName, settingsBuffer);
		const Reference<Helpers::PipelineDescriptor> pipelineDescriptor = [&]() -> Reference<Helpers::PipelineDescriptor> {
			std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> bindingSetDescriptors(shaderBinary->BindingSetCount());
			bool success = true;
			const Graphics::SPIRV_Binary* const binary = shaderBinary;
			success &= Graphics::ShaderResourceBindings::GenerateShaderBindings(&binary, 1, bindingSet, [&](const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo) {
				if (setInfo.setIndex >= bindingSetDescriptors.size()) {
					error("setIndex out of range! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					success = false;
				}
				else bindingSetDescriptors[setInfo.setIndex] = setInfo.set;
				}, device->Log());
			if (!success) return nullptr;
			Reference<Helpers::PipelineDescriptor> descriptor = new Helpers::PipelineDescriptor(shaderModule, std::move(bindingSetDescriptors));
			descriptor->ReleaseRef();
			return descriptor;
		}();
		if (pipelineDescriptor == nullptr)
			return error("Failed to create pipeline descriptor for \"", generationKernelShaderClass->ShaderPath(), "\"! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Reference<SegmentTreeGenerationKernel> kernelInstance = 
			new SegmentTreeGenerationKernel(device, bindingSet.resultBufferBinding, pipelineDescriptor, settingsBuffer, workGroupSize);
		kernelInstance->ReleaseRef();
		return kernelInstance;
	}

	size_t SegmentTreeGenerationKernel::SegmentTreeBufferSize(size_t inputBufferSize) {
		size_t result = inputBufferSize;
		size_t layerSize = inputBufferSize;
		while (layerSize > 1u) {
			result += layerSize;
			layerSize = (layerSize + 1u) >> 1u;
		}
		return result;
	}

	Reference<Graphics::ArrayBuffer> SegmentTreeGenerationKernel::Execute(
		const Graphics::Pipeline::CommandBufferInfo& commandBuffer, Graphics::ArrayBuffer* inputBuffer, size_t inputBufferSize, bool generateInPlace) {
		auto error = [&](const auto... message) {
			m_device->Log()->Error("SegmentTreeGenerationKernel::Execute - ", message...);
			return nullptr;
		};
		// __TODO__: Implement this crap!
		return error("Not yet implemented!");
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateUintSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateUintProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateIntSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateIntProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateFloatSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateFloatProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		// __TODO__: Implement this crap!
		return nullptr;
	}
}
