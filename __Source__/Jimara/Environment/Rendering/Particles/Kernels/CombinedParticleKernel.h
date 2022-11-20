#pragma once
#include "../ParticleKernel.h"


namespace Jimara {
	template<typename TaskSettings>
	class JIMARA_API CombinedParticleKernel : public virtual ParticleKernel::Instance {
	public:
		inline static Reference<CombinedParticleKernel> Create(
			SceneContext* context, const Graphics::ShaderClass* shaderClass, 
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings);

		inline virtual ~CombinedParticleKernel() {}

		inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) override;





	private:
		struct TaskDescriptor {
			TaskSettings taskSettings = {};
			alignas(16) Size3 taskBoundaries = {};
		};

		class PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
		private:
			const std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> m_bindingSets;
			const Reference<Graphics::Shader> m_shader;
			uint32_t m_threadCount = 0u;

		public:
			inline PipelineDescriptor(std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>>* bindingSets, Graphics::Shader* shader)
				: m_bindingSets(std::move(*bindingSets)), m_shader(shader) {
			}

			inline virtual ~PipelineDescriptor() {}

			inline virtual size_t BindingSetCount()const { return 2u; }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const { return m_bindingSets[index]; }

			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual Size3 NumBlocks() override {
				const constexpr size_t BLOCK_SIZE = 256u;
				return Size3((m_threadCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
			}

			inline uint32_t ThreadCount()const { return m_threadCount; }
			inline void SetThreadCount(uint32_t threadCount) { m_threadCount = threadCount; }
		};

	private:
		const Reference<SceneContext> m_context;
		const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_taskDescriptorBinding;
		const Reference<PipelineDescriptor> m_pipelineDescriptor;
		const Reference<Graphics::ComputePipeline> m_pipeline;
		std::vector<TaskDescriptor> m_lastTaskDescriptors;

		inline CombinedParticleKernel(
			SceneContext* context,
			Graphics::ShaderResourceBindings::StructuredBufferBinding* taskDescriptorBinding,
			PipelineDescriptor* pipelineDescriptor, Graphics::ComputePipeline* pipeline)
			: m_context(context), m_taskDescriptorBinding(taskDescriptorBinding), m_pipelineDescriptor(pipelineDescriptor), m_pipeline(pipeline) {}
	};


	template<typename TaskSettings>
	inline Reference<CombinedParticleKernel<TaskSettings>> CombinedParticleKernel<TaskSettings>::Create(
		SceneContext* context, const Graphics::ShaderClass* shaderClass,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings) {

		if (context == nullptr) return nullptr;
		auto fail = [&](auto... message) {
			context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Create - ", message...);
			return nullptr;
		};

		// Load shader:
		const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
		if (shaderSet == nullptr) return fail("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
		if (binary == nullptr) return fail("Failed to get shader binary for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(context->Graphics()->Device());
		if (shaderCache == nullptr) return fail("Failed to get shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
		if (shader == nullptr) return fail("Failed to create shader module for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create combined binding set:
		static const std::string combinedParticleKernelTasksBindingName = "jimara_CombinedParticleKernelTasks";
		if (bindings.FindStructuredBufferBinding(combinedParticleKernelTasksBindingName) != nullptr)
			return fail("Binding for ", combinedParticleKernelTasksBindingName, " is supposed to be provided by CombinedParticleKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> taskDescriptorBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>();
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet* baseBindings = nullptr;

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override { return baseBindings->FindConstantBufferBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == combinedParticleKernelTasksBindingName) return taskDescriptorBinding;
				return baseBindings->FindStructuredBufferBinding(name);
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override { return baseBindings->FindTextureSamplerBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override { return baseBindings->FindTextureViewBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override { return baseBindings->FindBindlessStructuredBufferSetBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override { return baseBindings->FindBindlessTextureSamplerSetBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override { return baseBindings->FindBindlessTextureViewSetBinding(name); }
		} bindingSet;
		bindingSet.baseBindings = &bindings;

		// Create graphics binding set descriptors:
		std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> bindingSetDescriptors(binary->BindingSetCount());
		{
			assert(bindingSetDescriptors.size() == binary->BindingSetCount());
			const Graphics::SPIRV_Binary* binaryPtr = binary;
			size_t maxIndex = 0u;
			if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(&binaryPtr, 1u, bindingSet,
				[&](const Graphics::ShaderResourceBindings::BindingSetInfo& info) {
					maxIndex = Math::Max(info.setIndex, maxIndex);
					if (info.setIndex < bindingSetDescriptors.size()) {
						if (bindingSetDescriptors[info.setIndex] == nullptr)
							bindingSetDescriptors[info.setIndex] = info.set;
						else context->Log()->Warning("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Create - ",
							"Duplicate binding set descriptor generated for set ", info.setIndex, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
					};
				}, context->Log()))
				return fail("Failed to generate shader bindings! [File:", __FILE__, "; Line: ", __LINE__, "]");
			if (maxIndex >= bindingSetDescriptors.size())
				return fail("Generated shader binding set index out of range! [File:", __FILE__, "; Line: ", __LINE__, "]");
			for (size_t i = 0; i < bindingSetDescriptors.size(); i++)
				if (bindingSetDescriptors[i] == nullptr)
					return fail("Binding set descriptor missing for set ", i, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
		}

		// Create pipeline:
		const Reference<PipelineDescriptor> pipelineDescriptor = Object::Instantiate<PipelineDescriptor>(&bindingSetDescriptors, shader);
		const Reference<Graphics::ComputePipeline> pipeline = context->Graphics()->Device()->CreateComputePipeline(
			pipelineDescriptor, context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (pipeline == nullptr) return fail("Compute Pipeline could not be created! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create CombinedParticleKernel Object:
		const Reference<CombinedParticleKernel> instance = new CombinedParticleKernel(context, bindingSet.taskDescriptorBinding, pipelineDescriptor, pipeline);
		instance->ReleaseRef();
		return instance;
	}

	template<typename TaskSettings>
	inline void CombinedParticleKernel<TaskSettings>::Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) {
		if (taskCount <= 0u) return;
		bool taskDescriptorBufferDirty = false;

		// Allocate enough bytes:
		if (m_lastTaskDescriptors.size() != taskCount) {
			m_lastTaskDescriptors.resize(taskCount);
			taskDescriptorBufferDirty = true;
		}

		// Store memory on CPU side and update kernel size:
		{
			uint32_t numberOfThreads = 0u;
			TaskDescriptor* descPtr = m_lastTaskDescriptors.data();
			const ParticleKernel::Task* const* taskPtr = tasks;
			const ParticleKernel::Task* const* const end = taskPtr + taskCount;
			while (taskPtr < end) {
				const TaskSettings settings = (*taskPtr)->GetSettings<TaskSettings>();
				if (settings.particleCount > 0u) {
					if (std::memcmp((void*)&settings, (void*)(&descPtr->taskSettings), sizeof(TaskSettings)) != 0) {
						descPtr->taskSettings = settings;
						taskDescriptorBufferDirty = true;
					}
					const uint32_t startIndex = numberOfThreads;
					numberOfThreads += settings.particleCount;
					if (descPtr->taskBoundaries.x != startIndex || descPtr->taskBoundaries.y != numberOfThreads)
					{
						descPtr->taskBoundaries.x = startIndex;
						descPtr->taskBoundaries.y = numberOfThreads;
						taskDescriptorBufferDirty = true;
					}
					descPtr++;
				}
				else taskCount--;
				taskPtr++;
			}
			if (m_pipelineDescriptor->ThreadCount() != numberOfThreads) {
				m_pipelineDescriptor->SetThreadCount(numberOfThreads);
				taskDescriptorBufferDirty = true;
			}
		}

		// (Re)Allocate buffer if needed:
		if (m_taskDescriptorBinding->BoundObject() == nullptr || m_taskDescriptorBinding->BoundObject()->ObjectCount() != taskCount) {
			m_taskDescriptorBinding->BoundObject() = nullptr;
			m_taskDescriptorBinding->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<TaskDescriptor>(taskCount);
			if (m_taskDescriptorBinding->BoundObject() == nullptr) {
				m_lastTaskDescriptors.clear();
				m_context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Execute - Failed to allocate input buffer for the kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			else taskDescriptorBufferDirty = true;
		}

		// If dirty, update buffer on GPU:
		if (taskDescriptorBufferDirty && taskCount > 0u) {
			std::memcpy(m_taskDescriptorBinding->BoundObject()->Map(), m_lastTaskDescriptors.data(), taskCount * sizeof(TaskDescriptor));
			m_taskDescriptorBinding->BoundObject()->Unmap(true);
		}

		// Execute pipeline:
		m_pipeline->Execute(commandBufferInfo);
	}
}
