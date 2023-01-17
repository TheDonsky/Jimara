#include "SetRandomVelocity.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleInitialization {
		SetRandomVelocity::SetRandomVelocity(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/SetRandomVelocity/SetRandomVelocity"), systemInfo->Context()) {}

		SetRandomVelocity::~SetRandomVelocity() {}

		void SetRandomVelocity::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.minimal, "Min velocity", "Minimal magnitude of the velocity");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal) 
					m_simulationSettings.maximal = m_simulationSettings.minimal;
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.maximal, "Max velocity", "Maximal magnitude of the velocity");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal)
					m_simulationSettings.minimal = m_simulationSettings.maximal;
			};
		}

		void SetRandomVelocity::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void SetRandomVelocity::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomVelocity>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::SetRandomVelocity>(
			"SetRandomVelocity", "Jimara/SetRandomVelocity", "Sets random omnidirectional velocity per newly spawned particle");
		report(factory);
	}
}
