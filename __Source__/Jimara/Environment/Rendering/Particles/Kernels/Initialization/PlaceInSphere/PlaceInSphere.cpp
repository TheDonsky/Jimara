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

		void PlaceInSphere::SetBuffers(
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
			const ParticleBuffers::BufferSearchFn& findBuffer) {
			const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* stateBuffer = findBuffer(ParticleState::BufferId());
			if (indirectionBuffer == nullptr || liveParticleCountBuffer == nullptr || stateBuffer == nullptr) {
				m_simulationSettings.liveParticleCountBufferId = 0u;
				m_simulationSettings.particleIndirectionBufferId = 0u;
				m_simulationSettings.stateBufferId = 0u;
				m_simulationSettings.particleBudget = 0u;
			}
			else {
				m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer->Index();
				m_simulationSettings.particleIndirectionBufferId = indirectionBuffer->Index();
				m_simulationSettings.stateBufferId = stateBuffer->Index();
				m_simulationSettings.particleBudget = ParticleBudget();
			}
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
