#include "ParticleInitializationStepKernel.h"



namespace Jimara {
	struct ParticleInitializationStepKernel::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleCountBufferId = 0u;
			alignas(4) uint32_t particleBudget = 0u;
			alignas(4) uint32_t spawnedParticleCount = 0u;
			alignas(4) uint32_t padding = {};
		
			inline bool operator!=(const ParticleTaskSettings& settings)const {
				return
					particleCountBufferId != settings.particleCountBufferId ||
					particleBudget != settings.particleBudget ||
					spawnedParticleCount != settings.spawnedParticleCount;
			}
		};

		static const ParticleInitializationStepKernel* Instance() {
			static const ParticleInitializationStepKernel instance;
			return &instance;
		}

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> settingCountBinding = Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> settingsBufferBinding = Object::Instantiate<Graphics::ShaderResourceBindings::StructuredBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> bindlessBinding = Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return settingCountBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return settingsBufferBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return bindlessBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
		};

		struct PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
			Reference<Graphics::Shader> shader;
			Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> bindingSets[2u];
			Size3 blockCount;

			inline virtual size_t BindingSetCount()const override { return 2u; }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const { return bindingSets[index]; }
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
			inline virtual Size3 NumBlocks() { return blockCount; }
		};

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			std::vector<ParticleTaskSettings> m_lastSettings;
			const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_settingsBuffer;
			const Graphics::BufferReference<uint32_t> m_settingCountBuffer;
			const Reference<PipelineDescriptor> m_pipelineDescriptor;
			const Reference<Graphics::ComputePipeline> m_pipeline;

		public:
			inline KernelInstance(SceneContext* context,
				Graphics::ShaderResourceBindings::StructuredBufferBinding* settingsBuffer,
				Graphics::Buffer* settingCountBuffer,
				PipelineDescriptor* pipelineDescriptor,
				Graphics::ComputePipeline* pipeline)
				: m_context(context)
				, m_settingsBuffer(settingsBuffer)
				, m_settingCountBuffer(settingCountBuffer)
				, m_pipelineDescriptor(pipelineDescriptor)
				, m_pipeline(pipeline) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) override {
				bool settingsDirty = false;
				
				// Make sure we have enough entries in CPU-side settings buffer:
				if (m_lastSettings.size() < taskCount) {
					m_lastSettings.resize(Math::Max(m_lastSettings.size() << 1u, taskCount));
					settingsDirty = true;
				}

				// Update CPU-side buffer:
				{
					const GraphicsSimulation::Task* const* ptr = tasks;
					const GraphicsSimulation::Task* const* const end = ptr + taskCount;
					ParticleTaskSettings* lastSettings = m_lastSettings.data();
					while (ptr < end) {
						const ParticleTaskSettings settings = (*ptr)->GetSettings<ParticleTaskSettings>();
						if (settings != (*lastSettings)) {
							(*lastSettings) = settings;
							settingsDirty = true;
						}
						lastSettings++;
						ptr++;
					}
				}

				// (Re)Allocate GPU buffer if needed:
				if (m_settingsBuffer->BoundObject() == nullptr || m_settingsBuffer->BoundObject()->ObjectCount() < m_lastSettings.size()) {
					m_settingsBuffer->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<ParticleTaskSettings>(m_lastSettings.size());
					if (m_settingsBuffer->BoundObject() == nullptr) {
						m_context->Log()->Error(
							"ParticleInitializationStepKernel::Helpers::KernelInstance::Execute - Failed to allocate settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_lastSettings.clear();
						return;
					}
					settingsDirty = true;
				}

				// If needed, upload settings buffer to GPU:
				if (settingsDirty) {
					std::memcpy(m_settingsBuffer->BoundObject()->Map(), (const void*)m_lastSettings.data(), sizeof(ParticleTaskSettings) * taskCount);
					m_settingsBuffer->BoundObject()->Unmap(true);
				}

				// Update constant buffer binding:
				{
					m_settingCountBuffer.Map() = static_cast<uint32_t>(taskCount);
					m_settingCountBuffer->Unmap(true);
				}

				// Update kernel size:
				{
					const constexpr uint32_t BLOCK_SIZE = 256u;
					m_pipelineDescriptor->blockCount = Size3((static_cast<uint32_t>(taskCount) + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
				}

				// Execute pipeline:
				m_pipeline->Execute(commandBufferInfo);
			}
		};
	};


	ParticleInitializationStepKernel::Task::Task(SceneContext* context) : GraphicsSimulation::Task(Helpers::Instance(), context) {}

	ParticleInitializationStepKernel::Task::~Task() {}

	void ParticleInitializationStepKernel::Task::SetBuffers(ParticleBuffers* buffers) {
		if (m_buffers == buffers) return;
		m_buffers = buffers;
		const Reference<ParticleInitializationTask>* ptr = m_initializationTasks.Data();
		const Reference<ParticleInitializationTask>* const end = ptr + m_initializationTasks.Size();
		while (ptr < end) {
			(*ptr)->SetBuffers(buffers);
			ptr++;
		}
	}

	size_t ParticleInitializationStepKernel::Task::InitializationTaskCount()const {
		return m_initializationTasks.Size();
	}

	ParticleInitializationTask* ParticleInitializationStepKernel::Task::InitializationTask(size_t index)const {
		return m_initializationTasks[index];
	}

	void ParticleInitializationStepKernel::Task::SetInitializationTask(size_t index, ParticleInitializationTask* task) {
		if (index >= m_initializationTasks.Size()) {
			AddInitializationTask(task);
			return;
		}
		if (task != nullptr) {
			m_initializationTasks[index] = task;
			task->SetBuffers(m_buffers);
		}
		else m_initializationTasks.RemoveAt(index);
	}

	void ParticleInitializationStepKernel::Task::AddInitializationTask(ParticleInitializationTask* task) {
		if (task != nullptr) m_initializationTasks.Push(task);
	}

	void ParticleInitializationStepKernel::Task::Synchronize() {
		Helpers::ParticleTaskSettings settings = {};
		m_lastBuffers = m_buffers;
		if (m_lastBuffers != nullptr) {
			settings.particleCountBufferId = m_lastBuffers->LiveParticleCountBuffer()->Index();
			settings.particleBudget = static_cast<uint32_t>(m_lastBuffers->ParticleBudget());
			settings.spawnedParticleCount = m_lastBuffers->SpawnedParticleCount()->load();
		}
		SetSettings(settings);
	}

	void ParticleInitializationStepKernel::Task::GetDependencies(const Callback<GraphicsSimulation::Task*>& reportDependency)const {
		if (m_buffers != nullptr)
			m_buffers->GetAllocationTasks(reportDependency);
		const Reference<ParticleInitializationTask>* ptr = m_initializationTasks.Data();
		const Reference<ParticleInitializationTask>* const end = ptr + m_initializationTasks.Size();
		while (ptr < end) {
			reportDependency(ptr->operator->());
			ptr++;
		}
	}

	ParticleInitializationStepKernel::ParticleInitializationStepKernel() : GraphicsSimulation::Kernel(sizeof(Helpers::ParticleTaskSettings)) {}
	
	ParticleInitializationStepKernel::~ParticleInitializationStepKernel() {}

	Reference<GraphicsSimulation::KernelInstance> ParticleInitializationStepKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		auto error = [&](auto... message) { context->Log()->Error("ParticleInitializationStepKernel::CreateInstance - ", message...); return nullptr; };

		const Reference<Helpers::PipelineDescriptor> pipelineDescriptor = Object::Instantiate<Helpers::PipelineDescriptor>();
		Helpers::BindingSet bindingSet;

		// Load shader:
		{
			const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
			if (shaderSet == nullptr) return error("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/InitializationStep/ParticleInitializationStepKernel");
			const Reference<Graphics::SPIRV_Binary> shaderBinary = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
			if (shaderBinary == nullptr) return error("Failed to load shader binary for '", SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			if (shaderBinary->BindingSetCount() != 2u) error("Shader binary expected to have 2 shader sets! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(context->Graphics()->Device());
			if (shaderCache == nullptr) return error("Failed to get shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			pipelineDescriptor->shader = shaderCache->GetShader(shaderBinary);
			if (pipelineDescriptor->shader == nullptr) 
				return error("Failed to create shader module for '", SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		// Create binding sets:
		{
			const Graphics::SPIRV_Binary* const binary = pipelineDescriptor->shader->Binary();
			if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(&binary, 1u, bindingSet, [&](const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo) {
				assert(setInfo.setIndex < 2u);
				pipelineDescriptor->bindingSets[setInfo.setIndex] = setInfo.set;
				}, context->Log())) return error("Failed to generate shader bindings! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			if (pipelineDescriptor->bindingSets[0] == nullptr || pipelineDescriptor->bindingSets[1] == nullptr)
				return error("Shader bindings incomplete! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		// Bind buffers:
		{
			bindingSet.bindlessBinding->BoundObject() = context->Graphics()->Bindless().BufferBinding();
			bindingSet.settingCountBinding->BoundObject() = context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
			if (bindingSet.settingCountBinding->BoundObject() == nullptr)
				return error("Failed to allocate setting count buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		}

		// Create pipeline:
		const Reference<Graphics::ComputePipeline> pipeline = 
			context->Graphics()->Device()->CreateComputePipeline(pipelineDescriptor, context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (pipeline == nullptr) 
			return error("Failed to create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::KernelInstance>(context, bindingSet.settingsBufferBinding, bindingSet.settingCountBinding->BoundObject(), pipelineDescriptor, pipeline);
	}
}
