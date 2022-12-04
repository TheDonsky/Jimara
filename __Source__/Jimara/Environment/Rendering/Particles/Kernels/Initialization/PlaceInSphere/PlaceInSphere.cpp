#include "PlaceInSphere.h"
#include "../CombinedParticleInitializationKernel.h"
#include "../../../ParticleState.h"
#include "../../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace InitializationKernels {
		PlaceInSphere::PlaceInSphere(SceneContext* context)
			: GraphicsSimulation::Task(CombinedParticleInitializationKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/Kernels/Initialization/PlaceInSphere/PlaceInSphere"), context) {}

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
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::PlaceInSphere>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<InitializationKernels::PlaceInSphere>(
			"PlaceInSphere", "Jimara/PlaceInSphere", "Places newly spawned particles at random positions inside a sphere");
		report(factory);
	}
}
