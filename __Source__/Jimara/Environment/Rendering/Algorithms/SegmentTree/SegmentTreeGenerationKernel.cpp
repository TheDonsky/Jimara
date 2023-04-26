#include "SegmentTreeGenerationKernel.h"


namespace Jimara {
	struct SegmentTreeGenerationKernel::Helpers {
		struct BuildSettings {
			alignas(4) uint32_t layerSize = 0u;
			alignas(4) uint32_t layerStart = 0u;
		};
		static_assert(sizeof(BuildSettings) == 8u);
	};

	SegmentTreeGenerationKernel::SegmentTreeGenerationKernel(
		Graphics::GraphicsDevice* device,
		Graphics::ResourceBinding<Graphics::ArrayBuffer>* resultBufferBinding,
		Graphics::BindingPool* bindingPool,
		Graphics::ComputePipeline* pipeline,
		const CachedGraphicsBindings* cachedBindings,
		Graphics::Buffer* settingsBuffer,
		size_t maxInFlightCommandBuffers,
		size_t workGroupSize) 
		: m_device(device)
		, m_resultBufferBinding(resultBufferBinding)
		, m_bindingPool(bindingPool)
		, m_pipeline(pipeline)
		, m_cachedBindings(cachedBindings)
		, m_settingsBuffer(settingsBuffer)
		, m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
		, m_workGroupSize(workGroupSize) {}

	SegmentTreeGenerationKernel::~SegmentTreeGenerationKernel() {}

	Reference<SegmentTreeGenerationKernel> SegmentTreeGenerationKernel::Create(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
		const Graphics::ShaderClass* generationKernelShaderClass, size_t maxInFlightCommandBuffers, size_t workGroupSize,
		const std::string_view& segmentTreeBufferBindingName, const std::string_view& generationKernelSettingsName,
		const Graphics::BindingSet::BindingSearchFunctions& additionalBindings) {
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

		const Graphics::BufferReference<Helpers::BuildSettings> settingsBuffer = device->CreateConstantBuffer<Helpers::BuildSettings>();
		if (settingsBuffer == nullptr)
			return error("Failed to crete settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBufferBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingsBuffer);
		auto findConstantBuffer = [&](const Graphics::BindingSet::BindingDescriptor& desc) {
			if (desc.name == generationKernelSettingsName) return settingsBufferBinding;
			else return additionalBindings.constantBuffer(desc);
		};

		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> segmentTreeBufferBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
		auto findStructuredBuffer = [&](const Graphics::BindingSet::BindingDescriptor& desc) 
			-> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
			if (desc.name == segmentTreeBufferBindingName) return segmentTreeBufferBinding;
			return additionalBindings.structuredBuffer(desc);
		};

		Graphics::BindingSet::BindingSearchFunctions searchFunctions = additionalBindings;
		searchFunctions.constantBuffer = &findConstantBuffer;
		searchFunctions.structuredBuffer = &findStructuredBuffer;

		const Reference<CachedGraphicsBindings> setBindings = CachedGraphicsBindings::Create(shaderBinary, searchFunctions, device->Log());
		if (setBindings == nullptr)
			return error("Failed to define bindings! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr)
			return error("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(shaderBinary);
		if (pipeline == nullptr)
			return error("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Reference<SegmentTreeGenerationKernel> kernelInstance = new SegmentTreeGenerationKernel(
			device, segmentTreeBufferBinding, bindingPool, pipeline, setBindings, settingsBuffer, maxInFlightCommandBuffers, workGroupSize);
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
		const Graphics::InFlightBufferInfo& commandBuffer, Graphics::ArrayBuffer* inputBuffer, size_t inputBufferSize, bool generateInPlace) {
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

		// (Re)Create pipeline binding sets if needed:
		const size_t bindingSetsPerExecution = m_pipeline->BindingSetCount();
		const size_t bindingSetsPerIteration = m_maxInFlightCommandBuffers * bindingSetsPerExecution;
		const size_t requiredBindingSetCount = bindingSetsPerIteration * numIterations;
		if (m_bindingSets.Size() < requiredBindingSetCount) {
			Graphics::BindingSet::Descriptor desc = {};
			desc.pipeline = m_pipeline;
			while (m_bindingSets.Size() < requiredBindingSetCount)
				for (size_t setId = 0u; setId < bindingSetsPerExecution; setId++) {
					desc.bindingSetId = setId;
					desc.find = m_cachedBindings->SearchFunctions(setId);
					const Reference<Graphics::BindingSet> bindingSet = m_bindingPool->AllocateBindingSet(desc);
					if (bindingSet == nullptr) {
						m_bindingSets.Clear();
						return error("Failed to create binding set! [File:", __FILE__, "; Line: ", __LINE__, "]");
					}
					else m_bindingSets.Push(bindingSet);
				}
			assert(m_bindingSets.Size() == requiredBindingSetCount);
		}
		else m_bindingSets.Resize(requiredBindingSetCount);

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
			const Reference<Graphics::BindingSet>* bindingSetPtr = m_bindingSets.Data();
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

				// Set build settings buffer content:
				{
					(*reinterpret_cast<Helpers::BuildSettings*>(m_settingsBuffer->Map())) = buildSettings;
					m_settingsBuffer->Unmap(true);
				}

				// Update and bind binding sets:
				{
					const Reference<Graphics::BindingSet>* ptr = bindingSetPtr;
					const Reference<Graphics::BindingSet>* const end = ptr + bindingSetsPerExecution;
					while (ptr < end) {
						Graphics::BindingSet* set = (*ptr);
						ptr++;
						set->Update(commandBuffer.inFlightBufferId);
						set->Bind(commandBuffer);
					}
					bindingSetPtr += bindingSetsPerIteration;
				}

				// Execute pipeline:
				{
					const Size3 numBlocks = Size3((((buildSettings.layerSize + 1u) >> 1u) + m_workGroupSize - 1u) / m_workGroupSize, 1u, 1u);
					//m_pipeline->Execute(commandBuffer.commandBuffer, baseInFlightId + i);
					m_pipeline->Dispatch(commandBuffer, numBlocks);
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
