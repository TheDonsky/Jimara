#include "ParticleKernels.h"
#include "../../../Data/Serialization/Attributes/EnumAttribute.h"



namespace Jimara {
	void ParticleInitializationTask::SetBuffers(ParticleBuffers* buffers) {
		std::unique_lock<SpinLock> lock(m_dependencyLock);
		m_dependencies.Clear();
		auto findBuffer = [&](const ParticleBuffers::BufferId* bufferId) -> Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* {
			if (buffers == nullptr) return nullptr;
			ParticleBuffers::BufferInfo bufferInfo = buffers->GetBufferInfo(bufferId);
			if (bufferInfo.allocationTask != nullptr) 
				m_dependencies.Push(bufferInfo.allocationTask);
			return bufferInfo.buffer;
		};
		m_buffers = buffers;
		SetBuffers(
			findBuffer(ParticleBuffers::IndirectionBufferId()),
			findBuffer(ParticleBuffers::LiveParticleCountBufferId()),
			ParticleBuffers::BufferSearchFn::FromCall(&findBuffer));
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

	uint32_t ParticleInitializationTask::ParticleBudget()const {
		return m_buffers == nullptr ? 0u : static_cast<uint32_t>(m_buffers->ParticleBudget());
	}



	void ParticleTimestepTask::SetBuffers(ParticleBuffers* buffers) {
		m_buffers = buffers;
		auto findNull = [](const ParticleBuffers::BufferId*) -> Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* { return nullptr; };
		SetBuffers(buffers == nullptr
			? ParticleBuffers::BufferSearchFn::FromCall(&findNull)
			: ParticleBuffers::BufferSearchFn(&ParticleBuffers::GetBuffer, buffers));
	}

	void ParticleTimestepTask::Synchronize() {
		m_lastBuffers = m_buffers;
		UpdateSettings();
	}

	void ParticleTimestepTask::GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const {
		recordDependency(m_spawningStep);
	}
}
