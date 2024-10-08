#include "PlaceInSphere.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleInitialization {
		PlaceInSphere::PlaceInSphere(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/PlaceInSphere/PlaceInSphere.comp"), systemInfo->Context())
			, m_systemInfo(systemInfo) {}

		PlaceInSphere::~PlaceInSphere() {}

		void PlaceInSphere::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_simulationSettings.radius, "Radius", "Radius of the spawn area");
			};
		}

		void PlaceInSphere::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void PlaceInSphere::UpdateSettings() {
			const Matrix4 trasnform = m_systemInfo->HasFlag(ParticleSystemInfo::Flag::SIMULATE_IN_LOCAL_SPACE) ? Math::Identity() : m_systemInfo->WorldTransform();
			m_simulationSettings.matX = trasnform[0];
			m_simulationSettings.matY = trasnform[1];
			m_simulationSettings.matZ = trasnform[2];
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::PlaceInSphere>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::PlaceInSphere>(
			"PlaceInSphere", "Jimara/PlaceInSphere", "Places newly spawned particles at random positions inside a sphere");
		report(factory);
	}
}
