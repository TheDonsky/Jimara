#pragma once
#include "GraphicsSimulation.h"


namespace Jimara {
	/// <summary>
	/// A helper class that makes it relatively simple for the user to add more GraphicsSimulation::Kernel-s that execute multiple tasks as a part of a single kernel.
	/// <para/> One example usage would be as follows:
	/// <para/> ____________________________________________________
	/// <para/> // Path/To/Shader/Source.comp:
	/// <para/> #version 450
	/// <para/> #extension GL_EXT_nonuniform_qualifier : enable
	/// <para/>
	/// <para/> layout (set = 0, binding = 0) buffer StateBuffers { State[] state; } stateBuffers[];
	/// <para/>
	///	<para/>	struct SimulationTaskSettings { // This name is mandatory; otherwise, the template will have no way to know about your structure
	///	<para/>		uint stateBufferId;		// Bindless index to some buffer within the StateBuffers (it's up to the user to include such stuff)
	///	<para/>		uint taskThreadCount;	// This field is required, offset from buffer's start is irrelevant.
	/// <para/>		// rest of the tasks preoperties...
	///	<para/>	};
	/// <para/>
	/// <para/> void ExecuteSimulationTask(in SimulationTaskSettings settings, uint taskThreadId) { // This name is important; CombinedGraphicsSimulationKernel_Body depends on it.
	/// <para/>		// stateBuffers[settings.bufferId].state[taskThreadId] is our target; Be free to do anything to it :D
	/// <para/> }
	/// <para/>
	/// <para/> // CombinedGraphicsSimulationKernel_Body uses a single custom binding named jimara_CombinedGraphicsSimulationKernelTasks; 
	/// <para/> // Do not reuse said name and provide the binding set and binding id before including it.
	/// <para/> #define COMBINED_SIMULATION_KERNEL_BINDING_SET 1
	/// <para/> #define COMBINED_SIMULATION_KERNEL_BINDING 0
	/// <para/> #include "path/to/CombinedGraphicsSimulationKernel_Body.glh"
	/// <para/>
	/// <para/> ____________________________________________________
	/// <para/> // CustomKernel.h/cpp:
	/// <para/> class CustomKernel : public virtual GraphicsSimulation::Kernel {
	/// <para/> public:
	///	<para/>		struct TaskSettings {
	///	<para/>			alignas(uint32_t) bufferId;			// Just an example... Could be bindless index of the state, could be missing, CombinedGraphicsSimulationKernel does not care..
	///	<para/>			alignas(uint32_t) taskThreadCount;	// This field is required, offset from buffer's start is irrelevant.
	/// <para/>			// rest of the tasks preoperties...
	///	<para/>		};
	/// <para/>
	/// <para/>		inline CustomKernel() : GraphicsSimulation::Kernel(sizeof(TaskSettings)) {} // Size of TaskSettings is important!
	/// <para/>
	/// <para/>		virtual Reference&lt;GraphicsSimulation::KernelInstance&gt; CreateInstance(SceneContext* context)const override {
	/// <para/>			Graphics::ShaderResourceBindings::ShaderResourceBindingSet* bindingSet = ... // A binding set that can supply the bindless sets and other bound objects to the pipeline.
	/// <para/>			static const Graphics::ShaderClass SHADER_CLASS("Path/To/Shader/Source");
	///	<para/>			return CombinedGraphicsSimulationKernel&lt;TaskSettings&gt;::Create(context, &#38;SHADER_CLASS, *bindingSet);
	///	<para/>		}
	/// <para/> };
	/// </summary>
	/// <typeparam name="SimulationTaskSettings"> Type of the settings buffer per task (has to match the settings of the kernel) </typeparam>
	template<typename SimulationTaskSettings>
	class JIMARA_API CombinedGraphicsSimulationKernel : public virtual GraphicsSimulation::KernelInstance {
	public:
		/// <summary>
		/// Creates a combined kernel instance
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="shaderClass"> Shader class </param>
		/// <param name="bindings"> Resource binding set </param>
		/// <returns> New instance of the CombinedGraphicsSimulationKernel </returns>
		inline static Reference<CombinedGraphicsSimulationKernel> Create(
			SceneContext* context, const Graphics::ShaderClass* shaderClass, 
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings);

		/// <summary> Virtual destructor </summary>
		inline virtual ~CombinedGraphicsSimulationKernel() {}

		/// <summary>
		/// Executes all tasks through a single kernel
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight index </param>
		/// <param name="tasks"> List of the tasks to be executed (if all was configured correctly, one would expect their settings buffer type to be SimulationTaskSettings) </param>
		/// <param name="taskCount"> Number of tasks within the buffer </param>
		inline virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) override;





	private:
		// TaskSettings and boundary information for the kernel to do a correct thread-id to task mapping:
		struct TaskDescriptor {
			// Copy of the SimulationTaskSettings from the task
			SimulationTaskSettings taskSettings = {};

			// X holds the index of the first thread tied to this task, while Y is the index of the first thread of the next task
			alignas(16) Size3 taskBoundaries = {};
		};

		// Scene context
		const Reference<SceneContext> m_context;

		// Binding of the jimara_CombinedGraphicsSimulationKernelTasks buffer
		const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_taskDescriptorBinding;

		// Compute pipeline
		const Reference<Graphics::Experimental::ComputePipeline> m_computePipeline;

		// Binding sets
		const Stacktor<Reference<Graphics::BindingSet>, 4u> m_bindingSets;

		// Last content of the m_taskDescriptorBinding buffer
		std::vector<TaskDescriptor> m_lastTaskDescriptors;

		// Constructor is private..
		inline CombinedGraphicsSimulationKernel(
			SceneContext* context,
			Graphics::ShaderResourceBindings::StructuredBufferBinding* taskDescriptorBinding,
			Graphics::Experimental::ComputePipeline* computePipeline,
			Stacktor<Reference<Graphics::BindingSet>, 4u>&& bindingSets)
			: m_context(context), m_taskDescriptorBinding(taskDescriptorBinding)
			, m_computePipeline(computePipeline), m_bindingSets(std::move(bindingSets)) {}
	};


	template<typename SimulationTaskSettings>
	inline Reference<CombinedGraphicsSimulationKernel<SimulationTaskSettings>> CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(
		SceneContext* context, const Graphics::ShaderClass* shaderClass,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings) {

		if (context == nullptr) return nullptr;
		auto fail = [&](auto... message) {
			context->Log()->Error("CombinedGraphicsSimulationKernel<", TypeId::Of<SimulationTaskSettings>().Name(), ">::Create - ", message...);
			return nullptr;
		};

		// Load shader:
		const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
		if (shaderSet == nullptr) return fail("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
		if (binary == nullptr) return fail("Failed to get shader binary for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create combined binding set:
		static const std::string CombinedGraphicsSimulationKernelTasksBindingName = "jimara_CombinedGraphicsSimulationKernelTasks";
		if (bindings.FindStructuredBufferBinding(CombinedGraphicsSimulationKernelTasksBindingName) != nullptr)
			return fail("Binding for ", CombinedGraphicsSimulationKernelTasksBindingName, 
				" is supposed to be provided by CombinedGraphicsSimulationKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create task descriptor binding:
		const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> taskDescriptorBinding =
			Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>();

		// Get compute pipeline:
		const Reference<Graphics::Experimental::ComputePipeline> computePipeline = context->Graphics()->Device()->GetComputePipeline(binary);
		if (computePipeline == nullptr) return fail("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create binding pool:
		const Reference<Graphics::BindingPool> bindingPool = context->Graphics()->Device()->CreateBindingPool(
			context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (bindingPool == nullptr) return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Define binding search functions:
		auto findConstantBuffer = [&](const auto& setDesc) { return bindings.FindConstantBufferBinding(setDesc.bindingName); };
		auto findStructuredBuffer = [&](const auto& setDesc) -> Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> {
			if (setDesc.bindingName == CombinedGraphicsSimulationKernelTasksBindingName) return taskDescriptorBinding;
			return bindings.FindStructuredBufferBinding(setDesc.bindingName);
		};
		auto findTextureSampler = [&](const auto& setDesc) { return bindings.FindTextureSamplerBinding(setDesc.bindingName); };
		auto findTextureView = [&](const auto& setDesc) { return bindings.FindTextureViewBinding(setDesc.bindingName); };

		using BindlessArrays = Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>;
		Reference<const BindlessArrays> bindlessBuffers;
		auto findBindlessStructuredBuffers = [&](const auto& setDesc) {
			const Reference<const BindlessArrays> binding = bindings.FindBindlessStructuredBufferSetBinding(setDesc.bindingName);
			if (binding != nullptr) return binding;
			if (bindlessBuffers == nullptr)
				bindlessBuffers = Object::Instantiate<BindlessArrays>(context->Graphics()->Bindless().BufferBinding());
			return bindlessBuffers;
		};

		using BindlessSamplers = Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>;
		Reference<const BindlessSamplers> bindlessSamplers;
		auto findBindlessTextureSamplers = [&](const auto& setDesc) {
			Reference<const BindlessSamplers> binding = bindings.FindBindlessTextureSamplerSetBinding(setDesc.bindingName);
			if (binding != nullptr) return binding;
			if (bindlessSamplers == nullptr)
				bindlessSamplers = Object::Instantiate<BindlessSamplers>(context->Graphics()->Bindless().SamplerBinding());
			return bindlessSamplers;
		};

		// Create binding sets:
		Graphics::BindingSet::Descriptor bindingSetDescriptor = {};
		{
			bindingSetDescriptor.pipeline = computePipeline;
			bindingSetDescriptor.findConstantBuffer = &findConstantBuffer;
			bindingSetDescriptor.findStructuredBuffer = &findStructuredBuffer;
			bindingSetDescriptor.findTextureSampler = &findTextureSampler;
			bindingSetDescriptor.findTextureView = &findTextureView;
			bindingSetDescriptor.findBindlessStructuredBuffers = &findBindlessStructuredBuffers;
			bindingSetDescriptor.findBindlessTextureSamplers = &findBindlessTextureSamplers;
		}
		Stacktor<Reference<Graphics::BindingSet>, 4u> bindingSets;
		for (size_t i = 0u; i < computePipeline->BindingSetCount(); i++) {
			bindingSetDescriptor.bindingSetId = i;
			const Reference<Graphics::BindingSet> bindingSet = bindingPool->AllocateBindingSet(bindingSetDescriptor);
			if (bindingSet == nullptr)
				return fail("Failed to allocate binding set ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			bindingSets.Push(bindingSet);
		}

		// Create CombinedGraphicsSimulationKernel Object:
		const Reference<CombinedGraphicsSimulationKernel> instance = new CombinedGraphicsSimulationKernel(
			context, taskDescriptorBinding, computePipeline, std::move(bindingSets));
		instance->ReleaseRef();
		return instance;
	}

	template<typename SimulationTaskSettings>
	inline void CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) {
		if (taskCount <= 0u) return;
		bool taskDescriptorBufferDirty = false;

		// Allocate enough bytes:
		if (m_lastTaskDescriptors.size() != taskCount) {
			m_lastTaskDescriptors.resize(taskCount);
			taskDescriptorBufferDirty = true;
		}

		// Store memory on CPU side and update kernel size:
		const uint32_t threadCount = [&]() {
			uint32_t numberOfThreads = 0u;
			TaskDescriptor* descPtr = m_lastTaskDescriptors.data();
			const GraphicsSimulation::Task* const* taskPtr = tasks;
			const GraphicsSimulation::Task* const* const end = taskPtr + taskCount;
			while (taskPtr < end) {
				const SimulationTaskSettings settings = (*taskPtr)->GetSettings<SimulationTaskSettings>();
				if (settings.taskThreadCount > 0u) {
					if (std::memcmp((void*)&settings, (void*)(&descPtr->taskSettings), sizeof(SimulationTaskSettings)) != 0) {
						descPtr->taskSettings = settings;
						taskDescriptorBufferDirty = true;
					}
					const uint32_t startIndex = numberOfThreads;
					numberOfThreads += settings.taskThreadCount;
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
			return numberOfThreads;
		}();

		// (Re)Allocate buffer if needed:
		if (m_taskDescriptorBinding->BoundObject() == nullptr || m_taskDescriptorBinding->BoundObject()->ObjectCount() != taskCount) {
			m_taskDescriptorBinding->BoundObject() = nullptr;
			m_taskDescriptorBinding->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<TaskDescriptor>(taskCount);
			if (m_taskDescriptorBinding->BoundObject() == nullptr) {
				m_lastTaskDescriptors.clear();
				m_context->Log()->Error("CombinedGraphicsSimulationKernel<", TypeId::Of<SimulationTaskSettings>().Name(), ">::Execute - Failed to allocate input buffer for the kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
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
		for (size_t i = 0u; i < m_bindingSets.Size(); i++) {
			Graphics::BindingSet* const set = m_bindingSets[i];
			set->Update(commandBufferInfo);
			set->Bind(commandBufferInfo);
		}
		{
			const constexpr size_t BLOCK_SIZE = 256u;
			const Size3 blockCount = Size3((threadCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
			m_computePipeline->Dispatch(commandBufferInfo, blockCount);
		}
	}
}
