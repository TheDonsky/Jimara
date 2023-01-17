#include "SetRandomScale.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleInitialization {
		SetRandomScale::SetRandomScale(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/SetRandomScale/SetRandomScale"), systemInfo->Context()) {}

		SetRandomScale::~SetRandomScale() {}

		void SetRandomScale::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.minimal, "Min Scale", "Minimal scale/size");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal)
					m_simulationSettings.maximal = m_simulationSettings.minimal;
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.maximal, "Max Scale", "Maximal scale/size");
				if (m_simulationSettings.minimal > m_simulationSettings.maximal)
					m_simulationSettings.minimal = m_simulationSettings.maximal;
			};
		}

		void SetRandomScale::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void SetRandomScale::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomScale>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::SetRandomScale>(
			"SetRandomScale", "Jimara/SetRandomScale", "Sets random inform scale per newly spawned particle");
		report(factory);
	}
}
