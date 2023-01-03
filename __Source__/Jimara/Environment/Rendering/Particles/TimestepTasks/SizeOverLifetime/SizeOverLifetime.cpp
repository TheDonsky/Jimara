#include "SizeOverLifetime.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleTimestep {
		SizeOverLifetime::SizeOverLifetime(GraphicsSimulation::Task* initializationTask)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/TimestepTasks/SizeOverLifetime/SizeOverLifetime"), initializationTask->Context())
			, ParticleTimestepTask(initializationTask) {}

		SizeOverLifetime::~SizeOverLifetime() {}

		void SizeOverLifetime::SetTimeMode(TimeMode timeMode) {
			m_simulationSettings.timeMode = static_cast<uint32_t>(timeMode);
		}

		void SizeOverLifetime::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_sizeCurve, "Curve", "Curve for size");
				JIMARA_SERIALIZE_FIELD(m_size2Curve, "Curve2", "Curve for size");
				JIMARA_SERIALIZE_FIELD(m_size3Curve, "Curve3", "Curve for size");
				JIMARA_SERIALIZE_FIELD(m_size4Curve, "Curve4", "Curve for size");
			};
		}

		void SizeOverLifetime::SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer) {
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.taskThreadCount = particleBudget;
		}

		void SizeOverLifetime::UpdateSettings() {
			// __TODO__: Update curve buffer if dirty? I guess...
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::SizeOverLifetime>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleTimestepTask::Factory::Create<ParticleTimestep::SizeOverLifetime>(
			"SizeOverLifetime", "Jimara/SizeOverLifetime", "Sets particle size over lifetime");
		report(factory);
	}
}
