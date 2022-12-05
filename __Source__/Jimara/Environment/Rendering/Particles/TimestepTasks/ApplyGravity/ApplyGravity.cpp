#include "ApplyGravity.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleTimestep {
		ApplyGravity::ApplyGravity(GraphicsSimulation::Task* initializationTask)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/TimestepTasks/ApplyGravity/ApplyGravity"), initializationTask->Context())
			, ParticleTimestepTask(initializationTask) {}

		ApplyGravity::~ApplyGravity() {}

		void ApplyGravity::SetTimeMode(TimeMode timeMode) {
			m_simulationSettings.timeMode = static_cast<uint32_t>(timeMode);
		}

		void ApplyGravity::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_gravityScale, "Gravity Scale", "Multiplier for the applied gravity");
			};
		}

		void ApplyGravity::SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer) {
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.taskThreadCount = particleBudget;
		}

		void ApplyGravity::UpdateSettings() {
			m_simulationSettings.gravity = Context()->Physics()->Gravity() * m_gravityScale;
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::ApplyGravity>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleTimestepTask::Factory::Create<ParticleTimestep::ApplyGravity>(
			"ApplyGravity", "Jimara/ApplyGravity", "Applies Gravity to particles");
		report(factory);
	}
}
