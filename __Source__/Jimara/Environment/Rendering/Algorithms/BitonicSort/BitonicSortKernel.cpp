#include "BitonicSortKernel.h"



namespace Jimara {
	Reference<BitonicSortKernel> BitonicSortKernel::Create(
		Graphics::GraphicsDevice* device,
		const Graphics::BindingSet::Descriptor::BindingSearchFunctions& bindings,
		size_t maxInFlightCommandBuffers, size_t workGroupSize,
		Graphics::SPIRV_Binary* singleStepShader, Graphics::SPIRV_Binary* groupsharedShader,
		const std::string_view& bitonicSortSettingsName) {

		if (device == nullptr) return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("BitonicSortKernel::Create - ", message...);
			return nullptr;
		};

		// Verify settings:
		if (maxInFlightCommandBuffers == 0) 
			maxInFlightCommandBuffers = 1u;
		if (workGroupSize <= 0u)
			return fail("0 workgroup size provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Verify shaders:
		if (singleStepShader == nullptr)
			singleStepShader = groupsharedShader;
		if (singleStepShader == nullptr)
			return fail("singleStepShader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if ((singleStepShader->ShaderStages() & Graphics::PipelineStage::COMPUTE) == Graphics::PipelineStage::NONE)
			return fail("singleStepShader expected to be a COMPUTE shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if (groupsharedShader != nullptr && (groupsharedShader->ShaderStages() & Graphics::PipelineStage::COMPUTE) == Graphics::PipelineStage::NONE)
			return fail("groupsharedShader expected to be a COMPUTE shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if (groupsharedShader != nullptr) {
			size_t groupSize = 1u;
			while (true) {
				if (groupSize == workGroupSize) break;
				else if (workGroupSize > workGroupSize)
					return fail("When groupsharedShader shader is provided, workGroupSize must be a power of 2! [File:", __FILE__, "; Line: ", __LINE__, "]");
				else groupSize <<= 1u;
			}
		}

		// Get/Create pipelines:
		const Reference<Graphics::Experimental::ComputePipeline> singleStepPipeline = device->GetComputePipeline(singleStepShader);
		if (singleStepPipeline == nullptr)
			return fail("Failed to get/create singleStepPipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::Experimental::ComputePipeline> groupsharedPipeline = [&]() -> Reference<Graphics::Experimental::ComputePipeline> {
			if (groupsharedShader == nullptr) return nullptr;
			else if (groupsharedShader == singleStepShader) return singleStepPipeline;
			else return device->GetComputePipeline(groupsharedShader);
		}();
		if (groupsharedShader != nullptr && groupsharedPipeline == nullptr)
			return fail("Failed to get/create groupsharedPipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create settings buffer:
		const Graphics::BufferReference<BitonicSortSettings> settingsBuffer = device->CreateConstantBuffer<BitonicSortSettings>();
		if (settingsBuffer == nullptr)
			return fail("Failed to generate settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBinding = 
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingsBuffer);

		// Create cached binding sets:
		auto findConstantBuffer = [&](const Graphics::BindingSet::BindingDescriptor& info) -> const Graphics::ResourceBinding<Graphics::Buffer>*{
			if (info.bindingName == bitonicSortSettingsName) return settingsBinding;
			else return bindings.constantBuffer(info);
		};
		Graphics::BindingSet::Descriptor::BindingSearchFunctions search = bindings;
		search.constantBuffer = &findConstantBuffer;
		const Reference<CachedGraphicsBindings> cachedSingleStepBindings = CachedGraphicsBindings::Create(singleStepShader, search, device->Log());
		if (cachedSingleStepBindings == nullptr)
			return fail("Failed to create cached bindings for single step shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<CachedGraphicsBindings> cachedGroupsharedBindings = [&]() -> Reference<CachedGraphicsBindings> {
			if (groupsharedShader == nullptr) return nullptr;
			else if (groupsharedShader == singleStepShader) return cachedSingleStepBindings;
			else return CachedGraphicsBindings::Create(groupsharedShader, search, device->Log());
		}();
		if (groupsharedShader != nullptr && cachedGroupsharedBindings == nullptr)
			return fail("Failed to create cached bindings for groupsharedPipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create binding pool:
		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr)
			return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create kernel:
		const Reference<BitonicSortKernel> kernelInstance = new BitonicSortKernel(
			device, singleStepPipeline, groupsharedPipeline,
			cachedSingleStepBindings, cachedGroupsharedBindings, bindingPool,
			settingsBuffer, maxInFlightCommandBuffers, workGroupSize);
		kernelInstance->ReleaseRef();
		return kernelInstance;
	}

	BitonicSortKernel::BitonicSortKernel(
		Graphics::GraphicsDevice* device,
		Graphics::Experimental::ComputePipeline* singleStepPipeline,
		Graphics::Experimental::ComputePipeline* groupsharedPipeline,
		CachedGraphicsBindings* singleStepBindings,
		CachedGraphicsBindings* groupsharedStepBindings,
		Graphics::BindingPool* bindingPool,
		const Graphics::BufferReference<BitonicSortSettings>& settingsBuffer,
		size_t maxInFlightCommandBuffers, size_t workGroupSize) 
		: m_device(device)
		, m_singleStepPipeline(singleStepPipeline)
		, m_groupsharedPipeline(groupsharedPipeline)
		, m_singleStepBindings(singleStepBindings)
		, m_groupsharedStepBindings(groupsharedStepBindings)
		, m_bindingPool(bindingPool)
		, m_settingsBuffer(settingsBuffer)
		, m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
		, m_workGroupSize(workGroupSize) {}

	BitonicSortKernel::~BitonicSortKernel() {}

	void BitonicSortKernel::Execute(const Graphics::InFlightBufferInfo& commandBuffer, size_t elemCount) {
		// Number of single steps to perform:
		uint32_t numSingleSteps = 0u;
		uint32_t numGroupsharedSteps = 0u;

		// Calculate listSizeBit for the last invokation:
		uint32_t listSizeBit = 0;
		while (((size_t)1u << listSizeBit) < elemCount) {
			uint32_t comparizonStepBit = listSizeBit;
			while (true) {
				if (m_groupsharedPipeline == nullptr || ((size_t)1u << comparizonStepBit) > m_workGroupSize) {
					numSingleSteps++;
					if (comparizonStepBit <= 0u) break;
					else comparizonStepBit--;
				}
				else {
					if (m_groupsharedPipeline == m_singleStepPipeline)
						numSingleSteps++;
					else numGroupsharedSteps++;
					break;
				}
			}
			listSizeBit++;
		}

		// (Re)Allocate binding sets or remove unused ones:
		auto allocateBindingSets = [&](
			auto& list, size_t bindingSetCount, size_t totalSets,
			const Graphics::Experimental::ComputePipeline* pipeline, 
			const CachedGraphicsBindings* cachedBindings) -> bool {
			if (list.Size() < totalSets) {
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = pipeline;
				while (list.Size() < totalSets) {
					for (size_t setId = 0u; setId < bindingSetCount; setId++) {
						desc.bindingSetId = setId;
						desc.find = cachedBindings->SearchFunctions(setId);
						const Reference<Graphics::BindingSet> set = m_bindingPool->AllocateBindingSet(desc);
						if (set == nullptr) {
							m_device->Log()->Error("BitonicSortKernel::Execute - Failed to allocate binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							list.Clear();
							return false;
						}
						else list.Push(set);
					}
				}
				assert(list.Size() == totalSets);
			}
			else list.Resize(totalSets);
			return true;
		};

		// Manage single step binding sets:
		const size_t singleStepsPerExecution = m_singleStepPipeline->BindingSetCount();
		const size_t singleStepsPerIteration = m_maxInFlightCommandBuffers * singleStepsPerExecution;
		const size_t singleStepsRequired = singleStepsPerIteration * numSingleSteps;
		if (!allocateBindingSets(m_singleStepBindingSets, singleStepsPerExecution, singleStepsRequired, m_singleStepPipeline, m_singleStepBindings)) return;

		// Manage groupshared binding sets:
		const size_t sharedStepsPerExecution = (m_groupsharedPipeline != nullptr) ? m_groupsharedPipeline->BindingSetCount() : size_t(0u);
		const size_t sharedStepsPerIteration = m_maxInFlightCommandBuffers * singleStepsPerIteration;
		const size_t sharedStepsRequired = sharedStepsPerIteration * numGroupsharedSteps;
		if (!allocateBindingSets(m_groupsharedStepBindingSets, sharedStepsPerExecution, sharedStepsRequired, m_groupsharedPipeline, m_groupsharedStepBindings)) return;

		// Early-return if the list size is no larger than 1:
		if (listSizeBit <= 0) 
			return;

		// Warn that elem count has to be a power of 2 for the algorithm to work properly:
		if (((size_t)1u << listSizeBit) != elemCount)
			m_device->Log()->Warning("BitonicSortKernel::Execute - Elem count should be a power of 2 for the algorithm to work correctly! ", elemCount, " provided!");

		// Set kernel size:
		const Size3 kernelSize = Size3(static_cast<uint32_t>((((size_t)1u << listSizeBit) + m_workGroupSize - 1u) / m_workGroupSize), 1u, 1u);

		// Execute pipelines:
		const Reference<Graphics::BindingSet>* singleStepPtr = m_singleStepBindingSets.Data();
		const Reference<Graphics::BindingSet>* groupsharedPtr = m_groupsharedStepBindingSets.Data();
		for (uint32_t sequenceSizeBit = 1; sequenceSizeBit <= listSizeBit; sequenceSizeBit++) {
			uint32_t comparizonStepBit = sequenceSizeBit - 1;
			while (true) {
				// Set settings:
				{
					BitonicSortSettings& settings = m_settingsBuffer.Map();
					settings.sequenceSizeBit = sequenceSizeBit;
					settings.comparizonStepBit = comparizonStepBit;
					m_settingsBuffer->Unmap(true);
				}
				
				// General helper for pipeline execution:
				auto execute = [&](
					Graphics::Experimental::ComputePipeline* pipeline, const Reference<Graphics::BindingSet>*& bindingPtr, 
					size_t bindingSetCount, size_t bindingsPerIteration) {
						const Reference<Graphics::BindingSet>* ptr = bindingPtr;
						const Reference<Graphics::BindingSet>* const end = ptr + bindingSetCount;
						while (ptr < end) {
							Graphics::BindingSet* set = (*ptr);
							ptr++;
							set->Update(commandBuffer);
							set->Bind(commandBuffer);
						}
						pipeline->Dispatch(commandBuffer, kernelSize);
						bindingPtr += bindingsPerIteration;
				};
				auto executeSingleStepPipeline = [&]() { 
					if (numSingleSteps <= 0u)
						m_device->Log()->Fatal("BitonicSortKernel::Execute - Internal error: not enough single step pipeline descriptors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					numSingleSteps--;
					execute(m_singleStepPipeline, singleStepPtr, singleStepsPerExecution, singleStepsPerIteration); 
				};

				// Execute pipelines:
				if (m_groupsharedPipeline == nullptr || ((size_t)1u << comparizonStepBit) > m_workGroupSize)
					executeSingleStepPipeline();
				else {
					if (m_groupsharedPipeline == m_singleStepPipeline)
						executeSingleStepPipeline();
					else {
						if (numGroupsharedSteps <= 0u)
							m_device->Log()->Fatal("BitonicSortKernel::Execute - Internal error: not enough groupshared pipeline descriptors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						numGroupsharedSteps--;
						execute(m_groupsharedPipeline, groupsharedPtr, sharedStepsPerExecution, sharedStepsPerIteration);
					}
					break;
				}

				// Decrement step size bit:
				if (comparizonStepBit <= 0u) break;
				else comparizonStepBit--;
			}
		}
	}

	Reference<BitonicSortKernel> BitonicSortKernel::CreateFloatSortingKernel(
		Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers,
		const Graphics::ResourceBinding<Graphics::ArrayBuffer>* binding) {
		static const constexpr uint32_t BLOCK_SIZE = 512u;
		static const constexpr std::string_view BASE_FOLDER = "Jimara/Environment/Rendering/Algorithms/BitonicSort/";
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_SingleStep");
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_GROUPSHARED(((std::string)BASE_FOLDER) + "BitonicSort_Floats_Groupshared");
		if (device == nullptr) return nullptr;
		if (shaderLoader == nullptr) {
			device->Log()->Error("BitonicSortKernel::CreateFloatSortingKernel - ShaderLoader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
		if (shaderSet == nullptr) {
			device->Log()->Error("BitonicSortKernel::CreateFloatSortingKernel - Failed to retrieve shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		auto getShader = [&](const Graphics::ShaderClass& shaderClass) -> Reference<Graphics::SPIRV_Binary> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr)
				device->Log()->Error(
					"BitonicSortKernel::CreateFloatSortingKernel - Failed to load shader binary for '", 
					shaderClass.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return binary;
		};
		const Reference<Graphics::SPIRV_Binary> singleStepShader = getShader(BITONIC_SORT_FLOATS_SINGLE_STEP);
		const Reference<Graphics::SPIRV_Binary> groupsharedShader = getShader(BITONIC_SORT_FLOATS_GROUPSHARED);
		if (singleStepShader == nullptr || groupsharedShader == nullptr) return nullptr;
		auto findBinding = [&](const Graphics::BindingSet::BindingDescriptor& info) 
			-> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
			static const constexpr std::string_view bindingName = "elements";
			if (info.bindingName == bindingName) return binding;
			else return nullptr;
		};
		Graphics::BindingSet::Descriptor::BindingSearchFunctions search = {};
		search.structuredBuffer = &findBinding;
		return Create(device, search, maxInFlightCommandBuffers, BLOCK_SIZE, singleStepShader, groupsharedShader);
	}
}
