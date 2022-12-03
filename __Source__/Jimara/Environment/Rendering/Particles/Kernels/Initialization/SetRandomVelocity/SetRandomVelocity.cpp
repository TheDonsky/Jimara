#include "SetRandomVelocity.h"
#include "../CombinedParticleInitializationKernel.h"
#include "../../../ParticleState.h"
#include "../../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace InitializationKernels {
		SetRandomVelocity::SetRandomVelocity(SceneContext* context)
			: GraphicsSimulation::Task(CombinedParticleInitializationKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/Kernels/Initialization/SetRandomVelocity/SetRandomVelocity"), context) {}

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

		void SetRandomVelocity::SetBuffers(
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

		void SetRandomVelocity::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::SetRandomVelocity>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Of<InitializationKernels::SetRandomVelocity>();
		report(factory);
	}
}
