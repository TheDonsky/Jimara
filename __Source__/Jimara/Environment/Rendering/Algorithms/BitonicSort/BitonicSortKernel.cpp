#include "BitonicSortKernel.h"



namespace Jimara {
	struct BitonicSortKernel::Helpers {
		class BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		private:
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet* m_baseBindings;
			const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> m_bitonicSortBinding;
			const std::string_view m_bitonicSortBindingName;

		public:
			inline BindingSet(
				const Graphics::ShaderResourceBindings::ShaderResourceBindingSet* baseBindings,
				Graphics::ShaderResourceBindings::ConstantBufferBinding* bitonicSortBinding,
				const std::string_view& bitonicSortBindingName) 
				: m_baseBindings(baseBindings)
				, m_bitonicSortBinding(bitonicSortBinding)
				, m_bitonicSortBindingName(bitonicSortBindingName) {}

			inline virtual ~BindingSet() {}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == m_bitonicSortBindingName) return m_bitonicSortBinding;
				else if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindConstantBufferBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindStructuredBufferBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindTextureSamplerBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindTextureViewBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindBindlessStructuredBufferSetBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindBindlessTextureSamplerSetBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override {
				if (m_baseBindings == nullptr) return nullptr;
				else return m_baseBindings->FindBindlessTextureViewSetBinding(name);
			}
		};

		class SingleListBindings : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		private:
			const Graphics::ShaderResourceBindings::StructuredBufferBinding* const m_binding;

		public:
			inline SingleListBindings(const Graphics::ShaderResourceBindings::StructuredBufferBinding* binding) : m_binding(binding) {}
			inline virtual ~SingleListBindings() {}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				static const constexpr std::string_view bindingName = "elements";
				if (name == bindingName) return m_binding;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override { return nullptr; }
		};

		class PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
		private:
			const Reference<Graphics::Shader> m_shader;
			const std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> m_bindingSets;
			const std::shared_ptr<const Size3> m_kernelSize;

		public:
			inline PipelineDescriptor(
				Graphics::Shader* shader,
				std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>>&& bindingSets,
				const std::shared_ptr<const Size3>& kernelSize)
				: m_shader(shader), m_bindingSets(std::move(bindingSets)), m_kernelSize(kernelSize) {}
			inline virtual ~PipelineDescriptor() {}

			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual size_t BindingSetCount()const override { return m_bindingSets.size(); }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const override { return m_bindingSets[index]; }
			inline virtual Size3 NumBlocks()const override { return *m_kernelSize; }
		};

		inline static Reference<Graphics::ComputePipeline::Descriptor> CreatePipelineDescriptor(
			Graphics::Shader* shader, OS::Logger* logger,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings,
			const std::shared_ptr<const Size3>& kernelSize) {
			if (shader == nullptr) return nullptr;
			std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> bindingSetDescriptors(shader->Binary()->BindingSetCount());
			const Graphics::SPIRV_Binary* const binary = shader->Binary();
			bool success = true;
			success &= Graphics::ShaderResourceBindings::GenerateShaderBindings(&binary, 1, bindings, [&](const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo) {
				if (setInfo.setIndex >= bindingSetDescriptors.size()) {
					logger->Error("BitonicSortKernel::Helpers::CreatePipelineDescriptor - setIndex out of range! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					success = false;
				}
				else bindingSetDescriptors[setInfo.setIndex] = setInfo.set;
				}, logger);
			if (!success) 
				return nullptr;
			const Reference<PipelineDescriptor> descriptor = new PipelineDescriptor(shader, std::move(bindingSetDescriptors), kernelSize);
			descriptor->ReleaseRef();
			return descriptor;
		}
	};

	Reference<BitonicSortKernel> BitonicSortKernel::Create(
		Graphics::GraphicsDevice* device,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings,
		size_t maxInFlightCommandBuffers, size_t workGroupSize,
		Graphics::Shader* singleStepShader, Graphics::Shader* groupsharedShader,
		const std::string_view& bitonicSortSettingsName) {

		if (device == nullptr) return nullptr;

		if (maxInFlightCommandBuffers == 0) 
			maxInFlightCommandBuffers = 1u;

		if (workGroupSize <= 0u) {
			device->Log()->Error("BitonicSortKernel::Create - 0 workgroup size provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		if (singleStepShader == nullptr)
			singleStepShader = groupsharedShader;
		if (singleStepShader == nullptr) {
			device->Log()->Error("BitonicSortKernel::Create - singleStepShader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		if ((singleStepShader->Binary()->ShaderStages() & Graphics::PipelineStage::COMPUTE) == Graphics::PipelineStage::NONE) {
			device->Log()->Error("BitonicSortKernel::Create - singleStepShader expected to be a COMPUTE shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		if (groupsharedShader != nullptr && (groupsharedShader->Binary()->ShaderStages() & Graphics::PipelineStage::COMPUTE) == Graphics::PipelineStage::NONE) {
			device->Log()->Error("BitonicSortKernel::Create - groupsharedShader expected to be a COMPUTE shader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		if (groupsharedShader != nullptr) {
			size_t groupSize = 1u;
			while (true) {
				if (groupSize == workGroupSize) break;
				else if (workGroupSize > workGroupSize) {
					device->Log()->Error(
						"BitonicSortKernel::Create - When groupsharedShader shader is provided, workGroupSize should be a power of 2! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				else groupSize <<= 1u;
			}
		}

		const Graphics::BufferReference<BitonicSortSettings> settingsBuffer = device->CreateConstantBuffer<BitonicSortSettings>();
		if (settingsBuffer == nullptr) {
			device->Log()->Error("BitonicSortKernel::Create - Failed to generate settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> settingsBinding = 
			Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>(settingsBuffer);
		if (settingsBinding == nullptr) {
			device->Log()->Error("BitonicSortKernel::Create - Failed to generate settings binding! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const std::shared_ptr<Size3> kernelSize = std::make_shared<Size3>();

		const Helpers::BindingSet bindingSet(&bindings, settingsBinding, bitonicSortSettingsName);

		
		const Reference<Graphics::ComputePipeline::Descriptor> singleStepPipelineDescriptor =
			Helpers::CreatePipelineDescriptor(singleStepShader, device->Log(), bindingSet, kernelSize);
		if (singleStepPipelineDescriptor == nullptr) {
			device->Log()->Error("BitonicSortKernel::Create - Failed to create pipeline descriptor for singleStepShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<Graphics::ComputePipeline::Descriptor> groupsharedPipelineDescriptor =
			(groupsharedShader == nullptr) ? nullptr :
			(groupsharedShader == singleStepShader) ? singleStepPipelineDescriptor :
			Helpers::CreatePipelineDescriptor(groupsharedShader, device->Log(), bindingSet, kernelSize);
		if (groupsharedShader != nullptr && singleStepPipelineDescriptor == nullptr) {
			device->Log()->Error("BitonicSortKernel::Create - Failed to create pipeline descriptor for groupsharedShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<BitonicSortKernel> kernelInstance = 
			new BitonicSortKernel(device, singleStepPipelineDescriptor, groupsharedPipelineDescriptor, settingsBuffer, kernelSize, maxInFlightCommandBuffers, workGroupSize);
		kernelInstance->ReleaseRef();
		return kernelInstance;
	}

	BitonicSortKernel::BitonicSortKernel(
		Graphics::GraphicsDevice* device,
		Graphics::ComputePipeline::Descriptor* singleStepPipelineDescriptor,
		Graphics::ComputePipeline::Descriptor* groupsharedPipelineDescriptor,
		const Graphics::BufferReference<BitonicSortSettings>& settingsBuffer,
		const std::shared_ptr<Size3>& kernelSize,
		size_t maxInFlightCommandBuffers, size_t workGroupSize)
		: m_device(device)
		, m_singleStepPipelineDescriptor(singleStepPipelineDescriptor)
		, m_groupsharedPipelineDescriptor(groupsharedPipelineDescriptor)
		, m_settingsBuffer(settingsBuffer)
		, m_kernelSize(kernelSize)
		, m_maxInFlightCommandBuffers(maxInFlightCommandBuffers)
		, m_workGroupSize(workGroupSize)
		, m_maxListSizeBit(0) {}

	BitonicSortKernel::~BitonicSortKernel() {}

	void BitonicSortKernel::Execute(const Graphics::Pipeline::CommandBufferInfo& commandBuffer, size_t elemCount) {
		// Number of single steps to perform:
		uint32_t numSingleSteps = 0u;
		uint32_t numGroupsharedSteps = 0u;

		// Calculate listSizeBit for the last invokation:
		uint32_t listSizeBit = 0;
		while (((size_t)1u << listSizeBit) < elemCount) {
			uint32_t comparizonStepBit = listSizeBit;
			while (true) {
				if (m_groupsharedPipelineDescriptor == nullptr || ((size_t)1u << comparizonStepBit) > m_workGroupSize) {
					numSingleSteps++;
					if (comparizonStepBit <= 0u) break;
					else comparizonStepBit--;
				}
				else {
					numGroupsharedSteps++;
					if (m_groupsharedPipelineDescriptor == m_singleStepPipelineDescriptor)
						numSingleSteps++;
					break;
				}
			}
			listSizeBit++;
		}

		// Clean pipelines if they are no longer enough:
		if (m_maxListSizeBit < listSizeBit) {
			m_singleStepPipeline = nullptr;
			m_groupsharedPipeline = nullptr;
		}

		// Early-return if the list size is no larger than 1:
		if (listSizeBit <= 0) 
			return;

		// Warn that elem count has to be a power of 2 for the algorithm to work properly:
		if (((size_t)1u << listSizeBit) != elemCount)
			m_device->Log()->Warning("BitonicSortKernel::Execute - Elem count should be a power of 2 for the algorithm to work correctly! ", elemCount, " provided!");
		
		// (Re)Create singleStepPipeline if needed:
		if (m_singleStepPipeline == nullptr) {
			m_singleStepPipeline = m_device->CreateComputePipeline(m_singleStepPipelineDescriptor, m_maxInFlightCommandBuffers * numSingleSteps);
			if (m_singleStepPipeline == nullptr) {
				m_device->Log()->Error("BitonicSortKernel::Create - Failed to create pipeline for singleStepShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// (Re)Create groupsharedPipeline if needed:
		if (m_groupsharedPipeline == nullptr && m_groupsharedPipelineDescriptor != nullptr) {
			if (m_groupsharedPipelineDescriptor == m_singleStepPipelineDescriptor)
				m_groupsharedPipeline = m_singleStepPipeline;
			else m_groupsharedPipeline = m_device->CreateComputePipeline(m_groupsharedPipelineDescriptor, m_maxInFlightCommandBuffers * numGroupsharedSteps);
			if (m_groupsharedPipeline == nullptr) {
				m_device->Log()->Error("BitonicSortKernel::Create - Failed to create pipeline for groupsharedShader! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Update maxListSizeBit:
		m_maxListSizeBit = Math::Max(m_maxListSizeBit.load(), listSizeBit);

		// Cleanup attachments for unused in-flight indices:
		{
			(*m_kernelSize) = Size3(0u);
			size_t end = ((m_maxListSizeBit + 1) * m_maxListSizeBit / 2);
			for (size_t i = numSingleSteps; i < end; i++)
				m_singleStepPipeline->Execute(Graphics::Pipeline::CommandBufferInfo(
					commandBuffer.commandBuffer, m_maxInFlightCommandBuffers * i + commandBuffer.inFlightBufferId));
			if (m_groupsharedPipeline != nullptr && m_groupsharedPipeline != m_singleStepPipeline)
				for (size_t i = numGroupsharedSteps; i < m_maxListSizeBit; i++)
					m_groupsharedPipeline->Execute(Graphics::Pipeline::CommandBufferInfo(
						commandBuffer.commandBuffer, m_maxInFlightCommandBuffers * i + commandBuffer.inFlightBufferId));
		}

		// Set kernel size:
		(*m_kernelSize) = Size3(static_cast<uint32_t>((((size_t)1u << listSizeBit) + m_workGroupSize - 1u) / m_workGroupSize), 1u, 1u);

		// Execute pipelines:
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
				
				// Execute pipelines:
				if (m_groupsharedPipeline == nullptr || ((size_t)1u << comparizonStepBit) > m_workGroupSize) {
					if (numSingleSteps <= 0u)
						m_device->Log()->Fatal("BitonicSortKernel::Execute - Internal error: not enough single step pipeline descriptors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					numSingleSteps--;
					m_singleStepPipeline->Execute(Graphics::Pipeline::CommandBufferInfo(
						commandBuffer.commandBuffer, m_maxInFlightCommandBuffers * numSingleSteps + commandBuffer.inFlightBufferId));
				}
				else {
					if (numGroupsharedSteps <= 0u)
						m_device->Log()->Fatal("BitonicSortKernel::Execute - Internal error: not enough groupshared pipeline descriptors! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					numGroupsharedSteps--;
					m_groupsharedPipeline->Execute(Graphics::Pipeline::CommandBufferInfo(
						commandBuffer.commandBuffer, m_maxInFlightCommandBuffers * numGroupsharedSteps + commandBuffer.inFlightBufferId));
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
		const Graphics::ShaderResourceBindings::StructuredBufferBinding* binding) {
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
		const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
		if (shaderCache == nullptr) {
			device->Log()->Error("BitonicSortKernel::CreateFloatSortingKernel - Failed to retrieve shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		auto getShader = [&](const Graphics::ShaderClass& shaderClass) -> Reference<Graphics::Shader> {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr) {
				device->Log()->Error("BitonicSortKernel::CreateFloatSortingKernel - Failed to load shader binary for '", shaderClass.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
			if (shader == nullptr)
				device->Log()->Error("BitonicSortKernel::CreateFloatSortingKernel - Failed to create shader module for '", shaderClass.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return shader;
		};
		const Reference<Graphics::Shader> singleStepShader = getShader(BITONIC_SORT_FLOATS_SINGLE_STEP);
		const Reference<Graphics::Shader> groupsharedShader = getShader(BITONIC_SORT_FLOATS_GROUPSHARED);
		if (singleStepShader == nullptr || groupsharedShader == nullptr) return nullptr;
		Helpers::SingleListBindings bindings(binding);
		return Create(device, bindings, maxInFlightCommandBuffers, BLOCK_SIZE, singleStepShader, groupsharedShader);
	}
}
