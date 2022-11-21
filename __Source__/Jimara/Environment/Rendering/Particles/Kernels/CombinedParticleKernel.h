#pragma once
#include "../ParticleKernel.h"


namespace Jimara {
	/// <summary>
	/// A helper class that makes it relatively simple for the user to add more ParticleKernel-s that execute multiple tasks as a part of a single kernel.
	/// <para/> Example usage would be as follows:
	/// <para/> ____________________________________________________
	/// <para/> // Path/To/Shader/Source.comp:
	/// <para/> #version 450
	/// <para/> #extension GL_EXT_nonuniform_qualifier : enable
	/// <para/> #include "path/to/ParticleState.glh"
	/// <para/>
	///	<para/>	struct ParticleTaskSettings { // This name is mandatory; otherwise, the template will have no way to know about your structure
	///	<para/>		uint bufferId;			// Just an example... Could be bindless index of the state, could be missing, CombinedParticleKernel does not care..
	///	<para/>		uint particleCount;	// This field is required, offset from buffer's start is irrelevant.
	/// <para/>		// rest of the tasks preoperties...
	///	<para/>	};
	/// <para/>
	/// <para/> layout (set = 0, binding = 0) buffer StateBuffers { ParticleState[] state; } stateBuffers[];
	/// <para/>
	/// <para/> void UpdateParticle(in ParticleTaskSettings settings, uint particleIndex) { // This name is important; CombinedParticleKernel_Body depends on it.
	/// <para/>		// stateBuffers[settings.bufferId].state[particleIndex] is our particle; Be free to do anything to it :D
	/// <para/> }
	/// <para/>
	/// <para/> // CombinedParticleKernel_Body uses a single custom binding named jimara_CombinedParticleKernelTasks; 
	/// <para/> // Do not reuse said name and provide the binding set and binding id before including it.
	/// <para/> #define COMBINED_PARTICLE_KERNEL_BINDING_SET 1
	/// <para/> #define COMBINED_PARTICLE_KERNEL_BINDING 0
	/// <para/> #include "path/to/CombinedParticleKernel_Body.glh"
	/// <para/>
	/// <para/> ____________________________________________________
	/// <para/> // CustomKernel.h/cpp:
	/// <para/> class CustomKernel : public virtual ParticleKernel {
	/// <para/> public:
	///	<para/>		struct TaskSettings {
	///	<para/>			alignas(uint32_t) bufferId;			// Just an example... Could be bindless index of the state, could be missing, CombinedParticleKernel does not care..
	///	<para/>			alignas(uint32_t) particleCount;	// This field is required, offset from buffer's start is irrelevant.
	/// <para/>			// rest of the tasks preoperties...
	///	<para/>		};
	/// <para/>
	/// <para/>		inline CustomKernel() : ParticleKernel(sizeof(TaskSettings)) {} // Size of TaskSettings is important!
	/// <para/>
	/// <para/>		virtual Reference&lt;Instance&gt; CreateInstance(SceneContext* context)const override {
	/// <para/>			Graphics::ShaderResourceBindings::ShaderResourceBindingSet* bindingSet = ... // A binding set that can supply the bindless sets and other bound objects to the pipeline.
	/// <para/>			static const Graphics::ShaderClass SHADER_CLASS("Path/To/Shader/Source");
	///	<para/>			return CombinedParticleKernel&lt;TaskSettings&gt;::Create(context, &#38;SHADER_CLASS, *bindingSet);
	///	<para/>		}
	/// <para/> };
	/// </summary>
	/// <typeparam name="ParticleTaskSettings"> Type of the settings buffer per task (has to match the settings of the kernel) </typeparam>
	template<typename ParticleTaskSettings>
	class JIMARA_API CombinedParticleKernel : public virtual ParticleKernel::Instance {
	public:
		/// <summary>
		/// Creates a particle kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="shaderClass"> Shader class </param>
		/// <param name="bindings"> Resource binding set </param>
		/// <returns> New instance of the CombinedParticleKernel </returns>
		inline static Reference<CombinedParticleKernel> Create(
			SceneContext* context, const Graphics::ShaderClass* shaderClass, 
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings);

		/// <summary> Virtual destructor </summary>
		inline virtual ~CombinedParticleKernel() {}

		/// <summary>
		/// Executes all tasks through a single kernel
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight index </param>
		/// <param name="tasks"> List of the tasks to be executed (if all was configured correctly, one would expect their settings buffer type to be ParticleTaskSettings) </param>
		/// <param name="taskCount"> Number of tasks within the buffer </param>
		inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) override;





	private:
		// TaskSettings and boundary information for the kernel to do a correct thread-id to task mapping:
		struct TaskDescriptor {
			// Copy of the ParticleTaskSettings from the task
			ParticleTaskSettings taskSettings = {};

			// X holds the index of the first thread tied to this task, while Y is the index of the first thread of the next task
			alignas(16) Size3 taskBoundaries = {};
		};

		// Compute pipeline descriptor
		class PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
		private:
			// Binding set descriptors, created automagically based on the shader binary and the binding descriptors
			const std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> m_bindingSets;

			// Compute shader from the shader class
			const Reference<Graphics::Shader> m_shader;

			// Total number of threads
			uint32_t m_threadCount = 0u;

		public:
			// Constructor
			inline PipelineDescriptor(std::vector<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>>* bindingSets, Graphics::Shader* shader)
				: m_bindingSets(std::move(*bindingSets)), m_shader(shader) {
			}

			// virtual desctructor
			inline virtual ~PipelineDescriptor() {}

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const { return 2u; }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const { return m_bindingSets[index]; }

			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual Size3 NumBlocks() override {
				const constexpr size_t BLOCK_SIZE = 256u;
				return Size3((m_threadCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
			}

			// Thread count controls for Execute() method:
			inline uint32_t ThreadCount()const { return m_threadCount; }
			inline void SetThreadCount(uint32_t threadCount) { m_threadCount = threadCount; }
		};

	private:
		// Scene context
		const Reference<SceneContext> m_context;

		// Binding of the jimara_CombinedParticleKernelTasks buffer
		const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_taskDescriptorBinding;

		// Compute pipeline descriptor
		const Reference<PipelineDescriptor> m_pipelineDescriptor;

		// Compute pipeline instance
		const Reference<Graphics::ComputePipeline> m_pipeline;

		// Last content of the m_taskDescriptorBinding buffer
		std::vector<TaskDescriptor> m_lastTaskDescriptors;

		// Constructor is private..
		inline CombinedParticleKernel(
			SceneContext* context,
			Graphics::ShaderResourceBindings::StructuredBufferBinding* taskDescriptorBinding,
			PipelineDescriptor* pipelineDescriptor, Graphics::ComputePipeline* pipeline)
			: m_context(context), m_taskDescriptorBinding(taskDescriptorBinding), m_pipelineDescriptor(pipelineDescriptor), m_pipeline(pipeline) {}
	};


	template<typename ParticleTaskSettings>
	inline Reference<CombinedParticleKernel<ParticleTaskSettings>> CombinedParticleKernel<ParticleTaskSettings>::Create(
		SceneContext* context, const Graphics::ShaderClass* shaderClass,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings) {

		if (context == nullptr) return nullptr;
		auto fail = [&](auto... message) {
			context->Log()->Error("CombinedParticleKernel<", TypeId::Of<ParticleTaskSettings>().Name(), ">::Create - ", message...);
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
			SceneContext* context = nullptr;
			mutable Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> bindlessBuffers;
			mutable Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> bindlessSamplers;

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override { return baseBindings->FindConstantBufferBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == combinedParticleKernelTasksBindingName) return taskDescriptorBinding;
				return baseBindings->FindStructuredBufferBinding(name);
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override { return baseBindings->FindTextureSamplerBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override { return baseBindings->FindTextureViewBinding(name); }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override { 
				Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> binding = baseBindings->FindBindlessStructuredBufferSetBinding(name);
				if (binding != nullptr) return binding;
				if (bindlessBuffers == nullptr)
					bindlessBuffers = Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>(context->Graphics()->Bindless().BufferBinding());
				return bindlessBuffers;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override { 
				Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> binding = baseBindings->FindBindlessTextureSamplerSetBinding(name);
				if (binding != nullptr) return binding;
				if (bindlessSamplers == nullptr)
					bindlessSamplers = Object::Instantiate<Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding>(context->Graphics()->Bindless().SamplerBinding());
				return bindlessSamplers;
			}
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override { return baseBindings->FindBindlessTextureViewSetBinding(name); }
		} bindingSet;
		bindingSet.baseBindings = &bindings;
		bindingSet.context = context;

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
						else context->Log()->Warning("CombinedParticleKernel<", TypeId::Of<ParticleTaskSettings>().Name(), ">::Create - ",
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

	template<typename ParticleTaskSettings>
	inline void CombinedParticleKernel<ParticleTaskSettings>::Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) {
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
				const ParticleTaskSettings settings = (*taskPtr)->GetSettings<ParticleTaskSettings>();
				if (settings.particleCount > 0u) {
					if (std::memcmp((void*)&settings, (void*)(&descPtr->taskSettings), sizeof(ParticleTaskSettings)) != 0) {
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
				m_context->Log()->Error("CombinedParticleKernel<", TypeId::Of<ParticleTaskSettings>().Name(), ">::Execute - Failed to allocate input buffer for the kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
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
