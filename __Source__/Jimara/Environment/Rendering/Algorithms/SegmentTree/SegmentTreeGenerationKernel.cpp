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
	};

	SegmentTreeGenerationKernel::SegmentTreeGenerationKernel(
		Graphics::GraphicsDevice* device,
		Graphics::ShaderResourceBindings::StructuredBufferBinding* resultBufferBinding,
		Graphics::ComputePipeline::Descriptor* pipelineDescriptor,
		Graphics::Buffer* settingsBuffer,
		size_t maxInFlightCommandBuffers,
		size_t workGroupSize) 
		: m_device(device)
		, m_resultBufferBinding(resultBufferBinding)
		, m_pipelineDescriptor(pipelineDescriptor)
		, m_settingsBuffer(settingsBuffer)
		, m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
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
		
		if (maxInFlightCommandBuffers <= 0u) 
			maxInFlightCommandBuffers = 1u;
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
		if (shaderModule == nullptr)
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
			new SegmentTreeGenerationKernel(device, bindingSet.resultBufferBinding, pipelineDescriptor, settingsBuffer, maxInFlightCommandBuffers, workGroupSize);
		kernelInstance->ReleaseRef();
		return kernelInstance;
	}

	size_t SegmentTreeGenerationKernel::SegmentTreeBufferSize(size_t inputBufferSize) {
		size_t result = 0u;
		size_t layerSize = inputBufferSize;
		while (true) {
			result += layerSize;
			if (layerSize <= 1u) break;
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

		// If we have zero input, just clean up and return:
		if (inputBuffer == nullptr || inputBufferSize <= 0u) {
			m_resultBufferBinding->BoundObject() = nullptr;
			return nullptr;
		}

		// If there is a mismatch in element size, discard old allocation:
		Graphics::ArrayBuffer* segmentBuffer = m_resultBufferBinding->BoundObject();
		if (segmentBuffer != nullptr && segmentBuffer->ObjectSize() != inputBuffer->ObjectSize()) {
			m_resultBufferBinding->BoundObject() = nullptr;
			segmentBuffer = nullptr;
		}

		// Calculate actual inputBufferSize and segmentBufferSize:
		if (inputBuffer->ObjectCount() < inputBufferSize)
			inputBufferSize = inputBuffer->ObjectCount();
		const size_t segmentBufferSize = SegmentTreeBufferSize(inputBufferSize);

		// If we have generateInPlace flag set, make sure element count is suffitient: 
		if (generateInPlace) {
			if (inputBuffer->ObjectCount() < segmentBufferSize)
				return error("generateInPlace flag set, but the input buffer is not big enough: ",
					"required SegmentTreeBufferSize(", inputBufferSize,") = ", segmentBufferSize,
					"; got inputBuffer->ObjectCount() = ", inputBuffer->ObjectCount(), ")! ",
					"[File:", __FILE__, "; Line: ", __LINE__, "]");
			m_resultBufferBinding->BoundObject() = inputBuffer;
			segmentBuffer = inputBuffer;
		}

		// Determine number of kernel iterations:
		const uint32_t groupLayerSize = static_cast<uint32_t>(m_workGroupSize << 1u);
		const size_t numIterations = [&]() {
			size_t itCount = 0u;
			size_t layerSize = inputBufferSize;
			while (layerSize > 1u) {
				layerSize = (layerSize + groupLayerSize - 1u) / groupLayerSize;
				itCount++;
			}
			return itCount;
		}();

		// (Re)Create kernel if needed:
		if (m_maxPipelineIterations < numIterations) {
			m_pipeline = m_device->CreateComputePipeline(m_pipelineDescriptor, m_maxInFlightCommandBuffers * numIterations);
			if (m_pipeline == nullptr) {
				m_maxPipelineIterations = 0u;
				return error("Failed to create Compute Pipeline! [File:", __FILE__, "; Line: ", __LINE__, "]");
			}
			m_maxPipelineIterations = numIterations;
		}
		const size_t baseInFlightId = commandBuffer.inFlightBufferId * m_maxPipelineIterations;
		Helpers::PipelineDescriptor* pipelineDescriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());

		// Clean pipeline descriptors that will not be used:
		{
			pipelineDescriptor->m_kernelSize = Size3(0u);
			for (size_t i = numIterations; i < m_maxPipelineIterations; i++)
				m_pipeline->Execute(commandBuffer.commandBuffer, baseInFlightId + i);
		}

		// (Re)Allocate result buffer if needed:
		if (segmentBuffer == nullptr || segmentBuffer->ObjectCount() < segmentBufferSize) {
			m_resultBufferBinding->BoundObject() = m_device->CreateArrayBuffer(
				inputBuffer->ObjectCount(), Math::Max((segmentBuffer != nullptr) ? segmentBuffer->ObjectCount() : 0u, segmentBufferSize));
			segmentBuffer = m_resultBufferBinding->BoundObject();
			if (segmentBuffer == nullptr)
				return error("Failed to allocate result buffer! [File:", __FILE__, "; Line: ", __LINE__, "]");
		}

		// Copy fisrt inputBufferSize elements if we are not generating in-place
		if (segmentBuffer != inputBuffer)
			segmentBuffer->Copy(commandBuffer.commandBuffer, inputBuffer, inputBuffer->ObjectCount() * inputBufferSize);

		// Run kernel as many times as needed:
		{
			Helpers::BuildSettings buildSettings = {};
			buildSettings.layerSize = static_cast<uint32_t>(inputBufferSize);
			buildSettings.layerStart = 0u;
			for (size_t i = 0u; i < numIterations; i++) {
				// Update buildSettings:
				if (i > 0u) {
					uint32_t groupLayerDimm = Math::Min(buildSettings.layerSize, groupLayerSize);
					while (groupLayerDimm > 1u) {
						groupLayerDimm = (groupLayerDimm + 1u) >> 1u;
						buildSettings.layerStart += buildSettings.layerSize;
						buildSettings.layerSize = (buildSettings.layerSize + 1u) >> 1u;
					}
				}

				// Execute pipeline:
				{
					(*reinterpret_cast<Helpers::BuildSettings*>(m_settingsBuffer->Map())) = buildSettings;
					m_settingsBuffer->Unmap(true);
					pipelineDescriptor->m_kernelSize = Size3((((buildSettings.layerSize + 1u) >> 1u) + m_workGroupSize - 1u) / m_workGroupSize, 1u, 1u);
					m_pipeline->Execute(commandBuffer.commandBuffer, baseInFlightId + i);
				}
			}
		}

		// Return result:
		return segmentBuffer;
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateUintSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_UintSumGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateUintProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_UintProductGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateIntSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_IntSumGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateIntProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_IntProductGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateFloatSumKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_FloatSumGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::CreateFloatProductKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers) {
		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Algorithms/SegmentTree/SegmentTree_FloatProductGenerator");
		return Create(device, shaderLoader, &SHADER_CLASS, maxInFlightCommandBuffers, 256u);
	}
}
