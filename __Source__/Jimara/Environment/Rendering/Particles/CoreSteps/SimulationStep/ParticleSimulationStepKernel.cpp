#include "ParticleSimulationStepKernel.h"
#include "../../ParticleState.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../CombinedParticleKernel.h"

namespace Jimara {
	struct ParticleSimulationStep::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;		// Bytes [0 - 4)
			alignas(4) uint32_t taskThreadCount = 0u;			// Bytes [4 - 8)
			alignas(4) float timeScale = 0.0f;					// Bytes [8 - 12)
			alignas(4) uint32_t timeType = 0u;					// Bytes [12 - 16) (0 - scaled; 1 - unscaled)
		};

		static const CombinedParticleKernel* Kernel() {
			static const Reference<const CombinedParticleKernel> kernel = CombinedParticleKernel::GetCached<ParticleTaskSettings>(
				"Jimara/Environment/Rendering/Particles/CoreSteps/SimulationStep/ParticleSimulationStepKernel");
			return kernel;
		}
	};


	ParticleSimulationStep::ParticleSimulationStep(const ParticleSystemInfo* systemInfo)
		: GraphicsSimulation::Task(Helpers::Kernel(), systemInfo->Context())
		, m_initializationStep(Object::Instantiate<ParticleInitializationStepKernel::Task>(systemInfo)) {}

	ParticleSimulationStep::~ParticleSimulationStep() {}

	void ParticleSimulationStep::SetBuffers(ParticleBuffers* buffers) {
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

	void ParticleSimulationStep::SetTimeScale(float timeScale) {
		m_timeScale = timeScale;
	}

	void ParticleSimulationStep::SetTimeMode(TimeMode timeMode) {
		m_timeMode = static_cast<uint32_t>(Math::Min(timeMode, TimeMode::PHYSICS_DELTA_TIME));
		const Reference<ParticleTimestepTask>* ptr = m_timestepTasks.Data();
		const Reference<ParticleTimestepTask>* const end = ptr + m_timestepTasks.Size();
		while (ptr < end) {
			(*ptr)->SetTimeMode(timeMode);
			ptr++;
		}
	}

	size_t ParticleSimulationStep::TimestepTaskCount()const {
		return m_timestepTasks.Size();
	}

	ParticleTimestepTask* ParticleSimulationStep::TimestepTask(size_t index)const {
		return m_timestepTasks[index];
	}

	void ParticleSimulationStep::SetTimestepTask(size_t index, ParticleTimestepTask* task) {
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

	void ParticleSimulationStep::AddTimestepTask(ParticleTimestepTask* task) {
		if (task == nullptr) return;
		m_timestepTasks.Push(task);
		task->SetBuffers(m_buffers);
		task->SetTimeMode(static_cast<ParticleTimestepTask::TimeMode>(m_timeMode.load()));
	}

	void ParticleSimulationStep::Synchronize() {
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

	void ParticleSimulationStep::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_initializationStep);
		const Reference<ParticleTimestepTask>* ptr = m_timestepTasks.Data();
		const Reference<ParticleTimestepTask>* const end = ptr + m_timestepTasks.Size();
		while (ptr < end) {
			recordDependency(ptr->operator->());
			ptr++;
		}
	}
}
