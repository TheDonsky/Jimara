#include "ParticleInitializationStepKernel.h"



namespace Jimara {
	struct ParticleInitializationStep::Helpers {
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

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			std::vector<ParticleTaskSettings> m_lastSettings;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_settingsBuffer;
			const Graphics::BufferReference<uint32_t> m_settingCountBuffer;
			const Reference<Graphics::Experimental::ComputePipeline> m_pipeline;
			const Stacktor<Reference<Graphics::BindingSet>, 2u> m_bindingSets;

		public:
			inline KernelInstance(SceneContext* context,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* settingsBuffer,
				Graphics::Buffer* settingCountBuffer,
				Graphics::Experimental::ComputePipeline* pipeline,
				Stacktor<Reference<Graphics::BindingSet>, 2u>& bindingSets)
				: m_context(context)
				, m_settingsBuffer(settingsBuffer)
				, m_settingCountBuffer(settingCountBuffer)
				, m_pipeline(pipeline)
				, m_bindingSets(std::move(bindingSets)) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) override {
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

				// Update and bind binding sets:
				for (size_t i = 0u; i < m_bindingSets.Size(); i++) {
					Graphics::BindingSet* set = m_bindingSets[i];
					set->Update(commandBufferInfo);
					set->Bind(commandBufferInfo);
				}

				// Execute pipeline:
				{
					const constexpr uint32_t BLOCK_SIZE = 256u;
					const Size3 blockCount = Size3((static_cast<uint32_t>(taskCount) + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
					m_pipeline->Dispatch(commandBufferInfo, blockCount);
				}
			}
		};


		class ParticleInitializationStepKernel : public virtual GraphicsSimulation::Kernel {
		public:
			ParticleInitializationStepKernel() : GraphicsSimulation::Kernel(sizeof(ParticleTaskSettings)) {}

			virtual ~ParticleInitializationStepKernel() {}

			virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
				if (context == nullptr) return nullptr;
				auto error = [&](auto... message) { 
					context->Log()->Error("ParticleInitializationStepKernel::CreateInstance - ", message...); 
					return nullptr; 
				};

				// Create pipeline:
				const Reference<Graphics::Experimental::ComputePipeline> pipeline = [&]() -> Reference<Graphics::Experimental::ComputePipeline> {
					const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
					if (shaderSet == nullptr) 
						return error("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					static const Graphics::ShaderClass SHADER_CLASS(
						"Jimara/Environment/Rendering/Particles/CoreSteps/InitializationStep/ParticleInitializationStepKernel");
					const Reference<Graphics::SPIRV_Binary> shaderBinary = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
					if (shaderBinary == nullptr) 
						return error("Failed to load shader binary for '", SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return context->Graphics()->Device()->GetComputePipeline(shaderBinary);
				}();
				if (pipeline == nullptr)
					return error("Failed to get/create compute pipeline! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				// Create resource bindings:
				const Graphics::BufferReference<uint32_t> settingCountBuffer = context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
				if (settingCountBuffer == nullptr)
					return error("Failed to create settings count buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingCountBinding = 
					Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(settingCountBuffer);
				auto findSettingCountBuffer = [&](const auto&) { return settingCountBinding; };

				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> settingsBufferBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				auto findSettingsBuffer = [&](const auto&) { return settingsBufferBinding; };

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> bindlessBinding = 
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
						context->Graphics()->Bindless().BufferBinding());
				auto findBindlessBuffers = [&](const auto&) { return bindlessBinding; };

				// Create binding sets:
				const Reference<Graphics::BindingPool> bindingPool = context->Graphics()->Device()->CreateBindingPool(
					context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (bindingPool == nullptr)
					return error("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				Graphics::BindingSet::Descriptor desc = {};
				{
					desc.pipeline = pipeline;
					desc.find.constantBuffer = &findSettingCountBuffer;
					desc.find.structuredBuffer = &findSettingsBuffer;
					desc.find.bindlessStructuredBuffers = &findBindlessBuffers;
				}
				Stacktor<Reference<Graphics::BindingSet>, 2u> sets;
				for (size_t i = 0u; i < pipeline->BindingSetCount(); i++) {
					desc.bindingSetId = i;
					const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
					if (set == nullptr)
						return error("Failed to allocate descriptor set for set ", i, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					sets.Push(set);
				}
				return Object::Instantiate<KernelInstance>(context, settingsBufferBinding, settingCountBuffer, pipeline, sets);
			}
		};


		static const ParticleInitializationStepKernel* Instance() {
			static const ParticleInitializationStepKernel instance;
			return &instance;
		}
	};


	ParticleInitializationStep::ParticleInitializationStep(const ParticleSystemInfo* systemInfo)
		: GraphicsSimulation::Task(Helpers::Instance(), systemInfo->Context())
		, m_systemInfo(systemInfo), m_initializationTasks(systemInfo, nullptr) {}

	ParticleInitializationStep::~ParticleInitializationStep() {}

	void ParticleInitializationStep::SetBuffers(ParticleBuffers* buffers) {
		if (m_buffers == buffers) return;
		m_buffers = buffers;
		m_initializationTasks.SetBuffers(buffers);
	}

	void ParticleInitializationStep::Synchronize() {
		Helpers::ParticleTaskSettings settings = {};
		m_lastBuffers = m_buffers;
		if (m_lastBuffers != nullptr) {
			settings.particleCountBufferId = m_lastBuffers->LiveParticleCountBuffer()->Index();
			settings.particleBudget = static_cast<uint32_t>(m_lastBuffers->ParticleBudget());
			settings.spawnedParticleCount = m_lastBuffers->SpawnedParticleCount()->load();
		}
		SetSettings(settings);
	}

	void ParticleInitializationStep::GetDependencies(const Callback<GraphicsSimulation::Task*>& reportDependency)const {
		if (m_buffers != nullptr)
			m_buffers->GetAllocationTasks(reportDependency);
		m_initializationTasks.GetDependencies(reportDependency);
	}
}
