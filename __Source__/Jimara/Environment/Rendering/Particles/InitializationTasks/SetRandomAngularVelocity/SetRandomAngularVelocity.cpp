#include "SetRandomAngularVelocity.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleInitialization {
		SetRandomAngularVelocity::SetRandomAngularVelocity(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/SetRandomAngularVelocity/SetRandomAngularVelocity.comp"), systemInfo->Context()) {
		}

		SetRandomAngularVelocity::~SetRandomAngularVelocity() {}

		void SetRandomAngularVelocity::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				static_assert(offsetof(Vector3, x) == 0u);
				
				static_assert(
					(offsetof(SimulationTaskSettings, minimalY) - offsetof(SimulationTaskSettings, minimalX)) ==
					(offsetof(Vector3, y) - offsetof(Vector3, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, minimalZ) - offsetof(SimulationTaskSettings, minimalX)) ==
					(offsetof(Vector3, z) - offsetof(Vector3, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, minimalZ) - offsetof(SimulationTaskSettings, minimalY)) ==
					(offsetof(Vector3, z) - offsetof(Vector3, y)));
				Vector3& minimal = *reinterpret_cast<Vector3*>((void*)(&m_simulationSettings.minimalX));
				assert((&minimal.x) == (&m_simulationSettings.minimalX));
				assert((&minimal.y) == (&m_simulationSettings.minimalY));
				assert((&minimal.z) == (&m_simulationSettings.minimalZ));

				static_assert(
					(offsetof(SimulationTaskSettings, maximalY) - offsetof(SimulationTaskSettings, maximalX)) ==
					(offsetof(Vector3, y) - offsetof(Vector3, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, maximalZ) - offsetof(SimulationTaskSettings, maximalX)) ==
					(offsetof(Vector3, z) - offsetof(Vector3, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, maximalZ) - offsetof(SimulationTaskSettings, maximalY)) ==
					(offsetof(Vector3, z) - offsetof(Vector3, y)));
				Vector3& maximal = *reinterpret_cast<Vector3*>((void*)(&m_simulationSettings.maximalX));
				assert((&maximal.x) == (&m_simulationSettings.maximalX));
				assert((&maximal.y) == (&m_simulationSettings.maximalY));
				assert((&maximal.z) == (&m_simulationSettings.maximalZ));

				JIMARA_SERIALIZE_FIELD(minimal, "Min angular velocity", "Minimal amount of the angulr velocity");
				if (m_simulationSettings.minimalX > m_simulationSettings.maximalX)
					m_simulationSettings.maximalX = m_simulationSettings.minimalX;
				if (m_simulationSettings.minimalY > m_simulationSettings.maximalY)
					m_simulationSettings.maximalY = m_simulationSettings.minimalY;
				if (m_simulationSettings.minimalZ > m_simulationSettings.maximalZ)
					m_simulationSettings.maximalZ = m_simulationSettings.minimalZ;

				JIMARA_SERIALIZE_FIELD(maximal, "Max angular velocity", "Maximal amount of the angulr velocity");
				if (m_simulationSettings.minimalX > m_simulationSettings.maximalX)
					m_simulationSettings.minimalX = m_simulationSettings.maximalX;
				if (m_simulationSettings.minimalY > m_simulationSettings.maximalY)
					m_simulationSettings.minimalY = m_simulationSettings.maximalY;
				if (m_simulationSettings.minimalZ > m_simulationSettings.maximalZ)
					m_simulationSettings.minimalZ = m_simulationSettings.maximalZ;
			};
		}

		void SetRandomAngularVelocity::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void SetRandomAngularVelocity::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomAngularVelocity>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::SetRandomAngularVelocity>(
			"SetRandomAngularVelocity", "Jimara/SetRandomAngularVelocity", "Sets random angular velocity per newly spawned particle");
		report(factory);
	}
}
