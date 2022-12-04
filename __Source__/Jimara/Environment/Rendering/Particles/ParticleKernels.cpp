#include "ParticleKernels.h"
#include "../../../Data/Serialization/Attributes/EnumAttribute.h"



namespace Jimara {
	void ParticleInitializationTask::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		m_dependencies.Clear();
		auto findBuffer = [&](const ParticleBuffers::BufferId* bufferId) -> uint32_t {
			if (buffers == nullptr) return 0u;
			ParticleBuffers::BufferInfo bufferInfo = buffers->GetBufferInfo(bufferId);
			if (bufferInfo.allocationTask != nullptr) 
				m_dependencies.Push(bufferInfo.allocationTask);
			return bufferInfo.buffer == nullptr ? ~uint32_t(0u) : bufferInfo.buffer->Index();
		};
		m_buffers = buffers;
		SetBuffers(
			(m_buffers == nullptr) ? 0u : static_cast<uint32_t>(m_buffers->ParticleBudget()),
			findBuffer(ParticleBuffers::IndirectionBufferId()),
			findBuffer(ParticleBuffers::LiveParticleCountBufferId()),
			BufferSearchFn::FromCall(&findBuffer));
	}

	void ParticleInitializationTask::Synchronize() {
		m_lastBuffers = m_buffers;
		UpdateSettings();
	}

	void ParticleInitializationTask::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		const Reference<GraphicsSimulation::Task>* taskPtr = m_dependencies.Data();
		const Reference<GraphicsSimulation::Task>* const end = taskPtr + m_dependencies.Size();
		while (taskPtr < end) {
			recordDependency(taskPtr->operator->());
			taskPtr++;
		}
	}

	uint32_t ParticleInitializationTask::SpawnedParticleCount()const {
		return m_buffers == nullptr ? 0u : m_buffers->SpawnedParticleCount()->load();
	}



	ParticleTimestepTask::ParticleTimestepTask(GraphicsSimulation::Task* spawningStep) 
		: m_spawningStep(spawningStep) {
		assert(spawningStep != nullptr);
	}

	void ParticleTimestepTask::SetBuffers(ParticleBuffers* buffers) {
		auto findBuffer = [&](const ParticleBuffers::BufferId* bufferId) -> uint32_t {
			if (m_buffers == nullptr) return 0u;
			ParticleBuffers::BufferInfo bufferInfo = m_buffers->GetBufferInfo(bufferId);
			return bufferInfo.buffer == nullptr ? ~uint32_t(0u) : bufferInfo.buffer->Index();
		};
		m_buffers = buffers;
		SetBuffers(
			(m_buffers == nullptr) ? 0u : static_cast<uint32_t>(m_buffers->ParticleBudget()),
			BufferSearchFn::FromCall(&findBuffer));
	}

	void ParticleTimestepTask::Synchronize() {
		m_lastBuffers = m_buffers;
		UpdateSettings();
	}

	void ParticleTimestepTask::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_spawningStep);
	}
}
