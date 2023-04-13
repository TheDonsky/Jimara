#include "SegmentTreeGenerationKernel.h"


namespace Jimara {
	struct SegmentTreeGenerationKernel::Helpers {
		struct BuildSettings {
			alignas(4) uint32_t layerSize = 0u;
			alignas(4) uint32_t layerStart = 0u;
		};
		static_assert(sizeof(BuildSettings) == 8u);

		template<typename ResourceType>
		using BindingReference = Reference<const Graphics::ResourceBinding<ResourceType>>;

		template<typename ResourceType>
		using ResourceMappings = std::unordered_map<size_t, BindingReference<ResourceType>>;

		struct SetBindings {
			ResourceMappings<Graphics::Buffer> constantBuffers;
			ResourceMappings<Graphics::ArrayBuffer> structuredBuffers;
			ResourceMappings<Graphics::TextureSampler> textureSamplers;
			ResourceMappings<Graphics::TextureView> textureViews;
			ResourceMappings<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> bindlessBuffers;
			ResourceMappings<Graphics::BindlessSet<Graphics::TextureSampler>::Instance> bindlessSamplers;
		
			bool Build(const Graphics::SPIRV_Binary::BindingSetInfo& setInfo, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log) {
				static const auto tryFind = [](const Graphics::SPIRV_Binary::BindingInfo& info, auto& bindings, const auto& searchFn) {
					{
						const auto it = bindings.find(info.binding);
						if (it != bindings.end()) return;
					}
					{
						Graphics::BindingSet::BindingDescriptor desc = {};
						desc.setBindingIndex = info.binding;
						desc.bindingName = info.name;
						const auto binding = searchFn(desc);
						if (binding == nullptr) return;
						bindings.insert(std::make_pair(desc.setBindingIndex, binding));
					}
				};
				static const auto hasEntry = [](const Graphics::SPIRV_Binary::BindingInfo& info, const auto& bindings) {
					return bindings.find(info.binding) != bindings.end();
				};
				
				typedef void(*TryFindBinding)(const Graphics::SPIRV_Binary::BindingInfo&, const Graphics::BindingSet::Descriptor::BindingSearchFunctions&, SetBindings*);
				typedef bool(*Verify)(const Graphics::SPIRV_Binary::BindingInfo&, SetBindings*);
				struct TryFindAndVerify {
					TryFindBinding tryFind = nullptr;
					Verify verify = nullptr;
				};

				static const TryFindAndVerify* findByType = []() -> const TryFindAndVerify* {
					static TryFindAndVerify findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)];

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->constantBuffers, search.constantBuffer);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->constantBuffers); }
					};

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->structuredBuffers, search.structuredBuffer);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->structuredBuffers); }
					};

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->textureSamplers, search.textureSampler);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->textureSamplers); }
					};

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->textureViews, search.textureView);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->textureViews); }
					};

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->bindlessBuffers, search.bindlessStructuredBuffers);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->bindlessBuffers); }
					};

					findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] = {
						[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
							tryFind(info, self->bindlessSamplers, search.bindlessTextureSamplers);
						}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->bindlessSamplers); }
					};

					return findFunctions;
				}();

				for (size_t bindingIndex = 0u; bindingIndex < setInfo.BindingCount(); bindingIndex++) {
					const Graphics::SPIRV_Binary::BindingInfo& info = setInfo.Binding(bindingIndex);
					if (info.type >= Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT) continue;
					const TryFindAndVerify findFn = findByType[static_cast<size_t>(info.type)];
					if (findFn.tryFind != nullptr)
						findFn.tryFind(info, search, this);
				}

				for (size_t bindingIndex = 0u; bindingIndex < setInfo.BindingCount(); bindingIndex++) {
					const Graphics::SPIRV_Binary::BindingInfo& info = setInfo.Binding(bindingIndex);
					if (info.type >= Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT) continue;
					const TryFindAndVerify findFn = findByType[static_cast<size_t>(info.type)];
					if (findFn.verify != nullptr && (!findFn.verify(info, this))) {
						log->Error("SegmentTreeGenerationKernel::Helpers::SetBindings::Build - ",
							"Failed to find resource binding for set ", info.set, " binding ", info.binding,
							"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
				}

				return true;
			}

			template<typename Type>
			struct Find {
				typedef BindingReference<Type>(*Fn)(const SetBindings*, const Graphics::BindingSet::BindingDescriptor);
			};

			inline Graphics::BindingSet::Descriptor::BindingSearchFunctions SearchFunctions() const {
				static const auto find = [](const auto& set, const Graphics::BindingSet::BindingDescriptor& desc) {
					const auto it = set.find(desc.setBindingIndex);
					return (it == set.end()) ? nullptr : it->second;
				};
				static const Find<Graphics::Buffer>::Fn constantBuffer = 
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->constantBuffers, desc); };
				static const Find<Graphics::ArrayBuffer>::Fn structuredBuffer =
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->structuredBuffers, desc); };
				static const Find<Graphics::TextureSampler>::Fn textureSampler =
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->textureSamplers, desc); };
				static const Find<Graphics::TextureView>::Fn textureView =
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->textureViews, desc); };
				static const Find<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>::Fn bindlessStructuredBuffers =
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->bindlessBuffers, desc); };
				static const Find<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>::Fn bindlessTextureSamplers =
					[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor desc) { return find(self->bindlessSamplers, desc); };
				
				Graphics::BindingSet::Descriptor::BindingSearchFunctions result = {};
				result.constantBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::Buffer>(constantBuffer, this);
				result.structuredBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::ArrayBuffer>(structuredBuffer, this);
				result.textureSampler = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureSampler>(textureSampler, this);
				result.textureView = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureView>(textureView, this);
				result.bindlessStructuredBuffers = Graphics::BindingSet::Descriptor::BindingSearchFn<
					Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>(bindlessStructuredBuffers, this);
				result.bindlessTextureSamplers = Graphics::BindingSet::Descriptor::BindingSearchFn<
					Graphics::BindlessSet<Graphics::TextureSampler>::Instance>(bindlessTextureSamplers, this);
				return result;
			}
		};

		struct BindingsPerSet : public virtual Object {
			Stacktor<SetBindings, 4u> sets;

			bool Build(const Graphics::SPIRV_Binary* binary, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log) {
				sets.Clear();
				sets.Resize(binary->BindingSetCount());
				for (size_t i = 0u; i < binary->BindingSetCount(); i++)
					if (!sets[i].Build(binary->BindingSet(i), search, log))
						return false;
				return true;
			}
		};
	};

	SegmentTreeGenerationKernel::SegmentTreeGenerationKernel(
		Graphics::GraphicsDevice* device,
		Graphics::ShaderResourceBindings::StructuredBufferBinding* resultBufferBinding,
		Graphics::BindingPool* bindingPool,
		Graphics::Experimental::ComputePipeline* pipeline,
		const Object* cachedBindings,
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
		const Graphics::BindingSet::Descriptor::BindingSearchFunctions& additionalBindings) {
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
			if (desc.bindingName == generationKernelSettingsName) return settingsBufferBinding;
			else return additionalBindings.constantBuffer(desc);
		};

		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> segmentTreeBufferBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
		auto findStructuredBuffer = [&](const Graphics::BindingSet::BindingDescriptor& desc) 
			-> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
			if (desc.bindingName == segmentTreeBufferBindingName) return segmentTreeBufferBinding;
			return additionalBindings.structuredBuffer(desc);
		};

		Graphics::BindingSet::Descriptor::BindingSearchFunctions searchFunctions = additionalBindings;
		searchFunctions.constantBuffer = &findConstantBuffer;
		searchFunctions.structuredBuffer = &findStructuredBuffer;

		const Reference<Helpers::BindingsPerSet> setBindings = Object::Instantiate<Helpers::BindingsPerSet>();
		if (!setBindings->Build(shaderBinary, searchFunctions, device->Log()))
			return error("Failed to define bindings! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::BindingPool> bindingPool = device->CreateBindingPool(maxInFlightCommandBuffers);
		if (bindingPool == nullptr)
			return error("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::Experimental::ComputePipeline> pipeline = device->GetComputePipeline(shaderBinary);
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
					desc.find = dynamic_cast<const Helpers::BindingsPerSet*>(m_cachedBindings.operator->())->sets[setId].SearchFunctions();
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
