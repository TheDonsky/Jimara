#include "ParticleSimulationStepKernel.h"
#include "../../ParticleState.h"
#include "../../../../GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../CombinedParticleKernel.h"

namespace Jimara {
	struct ParticleSimulationStep::Helpers {
		struct ParticleTaskSettings {
			alignas(4) uint32_t particleStateBufferId = 0u;		// Bytes [0 - 4)
			alignas(4) uint32_t taskThreadCount = 0u;			// Bytes [4 - 8)
			alignas(4) float timeScale = 1.0f;					// Bytes [8 - 12)
			alignas(4) uint32_t timeType = 0u;					// Bytes [12 - 16) (0 - scaled; 1 - unscaled)
		};

		static const CombinedParticleKernel* Kernel() {
			static const Reference<const CombinedParticleKernel> kernel = CombinedParticleKernel::GetCached<ParticleTaskSettings>(
				"Jimara/Environment/Rendering/Particles/CoreSteps/SimulationStep/ParticleSimulationStepKernel");
			return kernel;
		}
	};


	ParticleSimulationStep::ParticleSimulationStep(const ParticleSystemInfo* systemInfo)
		: ParticleSimulationStep(systemInfo, Object::Instantiate<ParticleInitializationStepKernel::Task>(systemInfo)) {}

	ParticleSimulationStep::ParticleSimulationStep(const ParticleSystemInfo* systemInfo, Reference<ParticleInitializationStepKernel::Task> initializationStep)
		: GraphicsSimulation::Task(Helpers::Kernel(), systemInfo->Context())
		, m_systemInfo(systemInfo)
		, m_initializationStep(initializationStep)
		, m_timestepTasks(systemInfo, initializationStep) {}

	ParticleSimulationStep::~ParticleSimulationStep() {}

	void ParticleSimulationStep::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_bufferLock);
		m_buffers = buffers;
		m_initializationStep->SetBuffers(buffers);
		m_timestepTasks.SetBuffers(buffers);
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
			settings.timeType = static_cast<uint32_t>(m_systemInfo->TimestepMode());
		}
		SetSettings(settings);
	}

	void ParticleSimulationStep::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		m_timestepTasks.GetDependencies(recordDependency);
	}
}
