#include "ParticleSimulationStepKernel.h"
#include "../../ParticleState.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"

namespace Jimara {
	struct ParticleSimulationStepKernel::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;		// Bytes [0 - 4)
			alignas(4) uint32_t taskThreadCount = 0u;			// Bytes [4 - 8)
			alignas(4) float timeScale = 0.0f;					// Bytes [8 - 12)
			alignas(4) uint32_t timeType = 0u;					// Bytes [12 - 16) (0 - scaled; 1 - unscaled)
		};

		static const ParticleSimulationStepKernel* Instance() {
			static const ParticleSimulationStepKernel instance;
			return &instance;
		}

		class KernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			const Graphics::BufferReference<Vector4> m_timeInfo;
			const Reference<GraphicsSimulation::KernelInstance> m_kernel;
			

		public:
			inline KernelInstance(SceneContext* context, Graphics::BufferReference<Vector4>& timeInfo, GraphicsSimulation::KernelInstance* kernel)
				: m_context(context), m_timeInfo(timeInfo), m_kernel(kernel) {}

			inline virtual ~KernelInstance() {}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const GraphicsSimulation::Task* const* tasks, size_t taskCount) override {
				const Vector4 timeInfo = Vector4(
					0.0f,
					m_context->Time()->UnscaledDeltaTime(),
					m_context->Time()->ScaledDeltaTime(),
					m_context->Physics()->Time()->ScaledDeltaTime());
				m_timeInfo.Map() = timeInfo;
				m_timeInfo->Unmap(true);
				m_kernel->Execute(commandBufferInfo, tasks, taskCount);
			}
		};

		struct BindingSet : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
			const Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding> constantBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::ConstantBufferBinding>();
			const Reference<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> bindlessBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding>();
			
			inline BindingSet(SceneContext* context, Graphics::Buffer* constantBuffer) {
				constantBinding->BoundObject() = constantBuffer;
				bindlessBinding->BoundObject() = context->Graphics()->Bindless().BufferBinding(); 
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string&)const override { return constantBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override { return bindlessBinding; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override { return nullptr; }
			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override { return nullptr; }
		};
	};


	ParticleSimulationStepKernel::Task::Task(SceneContext* context) 
		: GraphicsSimulation::Task(Helpers::Instance(), context)
		, m_initializationStep(Object::Instantiate<ParticleInitializationStepKernel::Task>(context)) {}

	ParticleSimulationStepKernel::Task::~Task() {}

	void ParticleSimulationStepKernel::Task::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_bufferLock);
		m_buffers = buffers;
		m_initializationStep->SetBuffers(buffers);
		const Reference<ParticleTimestepTask>* ptr = m_timestepTasks.Data();
		const Reference<ParticleTimestepTask>* const end = ptr + m_timestepTasks.Size();
		while (ptr < end) {
			(*ptr)->SetBuffers(buffers);
			ptr++;
		}
	}

	void ParticleSimulationStepKernel::Task::SetTimeScale(float timeScale) {
		m_timeScale = timeScale;
	}

	void ParticleSimulationStepKernel::Task::SetTimeMode(TimeMode timeMode) {
		m_timeMode = static_cast<uint32_t>(Math::Min(timeMode, TimeMode::PHYSICS_DELTA_TIME));
		const Reference<ParticleTimestepTask>* ptr = m_timestepTasks.Data();
		const Reference<ParticleTimestepTask>* const end = ptr + m_timestepTasks.Size();
		while (ptr < end) {
			(*ptr)->SetTimeMode(timeMode);
			ptr++;
		}
	}

	size_t ParticleSimulationStepKernel::Task::TimestepTaskCount()const {
		return m_timestepTasks.Size();
	}

	ParticleTimestepTask* ParticleSimulationStepKernel::Task::TimestepTask(size_t index)const {
		return m_timestepTasks[index];
	}

	void ParticleSimulationStepKernel::Task::SetTimestepTask(size_t index, ParticleTimestepTask* task) {
		if (index >= m_timestepTasks.Size()) {
			AddTimestepTask(task);
			return;
		}
		if (task != nullptr) {
			m_timestepTasks[index] = task;
			task->SetBuffers(m_buffers);
			task->SetTimeMode(static_cast<ParticleTimestepTask::TimeMode>(m_timeMode.load()));
		}
		else m_timestepTasks.RemoveAt(index);
	}

	void ParticleSimulationStepKernel::Task::AddTimestepTask(ParticleTimestepTask* task) {
		if (task == nullptr) return;
		m_timestepTasks.Push(task);
		task->SetBuffers(m_buffers);
		task->SetTimeMode(static_cast<ParticleTimestepTask::TimeMode>(m_timeMode.load()));
	}

	void ParticleSimulationStepKernel::Task::Synchronize() {
		const Reference<ParticleBuffers> buffers = [&]() ->Reference<ParticleBuffers> {
			std::unique_lock<SpinLock> lock(m_bufferLock);
			Reference<ParticleBuffers> rv = m_buffers;
			return rv;
		}();
		if (m_lastBuffers != buffers) {
			if (buffers != nullptr) {
				m_particleStateBuffer = buffers->GetBuffer(ParticleState::BufferId());
				if (m_particleStateBuffer == nullptr)
					Context()->Log()->Error("ParticleSimulationStepKernel::Task::Synchronize - Failed to get ParticleState buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
			else m_particleStateBuffer = nullptr;
			m_lastBuffers = buffers;
		}
		Helpers::ParticleTaskSettings settings = {};
		{
			if (m_particleStateBuffer != nullptr) {
				settings.particleStateBufferId = m_particleStateBuffer->Index();
				settings.taskThreadCount = static_cast<uint32_t>(m_lastBuffers->ParticleBudget());
			}
			{
				settings.timeScale = m_timeScale.load();
				settings.timeType = m_timeMode.load();
			}
		}
		SetSettings(settings);
	}

	void ParticleSimulationStepKernel::Task::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_initializationStep);
		const Reference<ParticleTimestepTask>* ptr = m_timestepTasks.Data();
		const Reference<ParticleTimestepTask>* const end = ptr + m_timestepTasks.Size();
		while (ptr < end) {
			recordDependency(ptr->operator->());
			ptr++;
		}
	}


	ParticleSimulationStepKernel::ParticleSimulationStepKernel() : GraphicsSimulation::Kernel(sizeof(Helpers::ParticleTaskSettings)) {}
	ParticleSimulationStepKernel::~ParticleSimulationStepKernel() {}

	Reference<GraphicsSimulation::KernelInstance> ParticleSimulationStepKernel::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;

		const Graphics::BufferReference<Vector4> timeInfo = context->Graphics()->Device()->CreateConstantBuffer<Vector4>();
		if (timeInfo == nullptr) {
			context->Log()->Error("ParticleSimulationStepKernel::CreateInstance - Failed to create time buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		static const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/CoreSteps/SimulationStep/ParticleSimulationStepKernel");
		const Helpers::BindingSet bindingSet(context, timeInfo);
		const Reference<GraphicsSimulation::KernelInstance> kernelInstance = CombinedGraphicsSimulationKernel<Helpers::ParticleTaskSettings>::Create(context, &SHADER_CLASS, bindingSet);
		if (kernelInstance == nullptr) {
			context->Log()->Error("ParticleSimulationStepKernel::CreateInstance - Failed to create combined kernel instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		return Object::Instantiate<Helpers::KernelInstance>(context, timeInfo, kernelInstance);
	}
}
