#include "SetRandomLifetime.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleInitialization {
		SetRandomLifetime::SetRandomLifetime(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/SetRandomLifetime/SetRandomLifetime.comp"), systemInfo->Context()) {}

		SetRandomLifetime::~SetRandomLifetime() {}

		void SetRandomLifetime::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.minimal, "Min Lifetime", "Minimal lifetime per particle");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal)
					m_simulationSettings.maximal = m_simulationSettings.minimal;
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.maximal, "Max Lifetime", "Maximal lifetime per particle");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal)
					m_simulationSettings.minimal = m_simulationSettings.maximal;
			};
		}

		void SetRandomLifetime::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void SetRandomLifetime::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomLifetime>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::SetRandomLifetime>(
			"SetRandomLifetime", "Jimara/SetRandomLifetime", "Sets random inform lifetime per newly spawned particle");
		report(factory);
	}
}
