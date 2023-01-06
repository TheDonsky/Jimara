#include "ColorOverLifetime.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleTimestep {
		ColorOverLifetime::ColorOverLifetime(GraphicsSimulation::Task* initializationTask)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/TimestepTasks/ColorOverLifetime/ColorOverLifetime"), initializationTask->Context())
			, ParticleTimestepTask(initializationTask)
			, m_colorCurve(initializationTask->Context()->Graphics()->Device(), "Curve", "Color over lifetime") {}

		ColorOverLifetime::~ColorOverLifetime() {}

		void ColorOverLifetime::SetTimeMode(TimeMode timeMode) {}

		void ColorOverLifetime::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			m_colorCurve.GetFields(recordElement);
		}

		void ColorOverLifetime::SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer) {
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.taskThreadCount = particleBudget;
		}

		void ColorOverLifetime::UpdateSettings() {
			const Graphics::ArrayBufferReference<GraphicsTimelineCurve<Vector4>::KeyFrame> curveBuffer = m_colorCurve.GetCurveBuffer();
			if (curveBuffer == nullptr) {
				Context()->Log()->Error("ColorOverLifetime::UpdateSettings - Failed to get curve data on GPU! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				m_colorCurveBinding = nullptr;
				m_simulationSettings.curveBufferId = 0u;
			}
			else if (m_colorCurveBinding == nullptr || m_colorCurveBinding->BoundObject() != curveBuffer) {
				m_colorCurveBinding = Context()->Graphics()->Bindless().Buffers()->GetBinding(curveBuffer);
				if (m_colorCurveBinding == nullptr) {
					Context()->Log()->Error("ColorOverLifetime::UpdateSettings - Failed to get bindless index for the curve! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					m_simulationSettings.curveBufferId = 0u;
				}
				else m_simulationSettings.curveBufferId = m_colorCurveBinding->Index();
			}
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::ColorOverLifetime>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleTimestepTask::Factory::Create<ParticleTimestep::ColorOverLifetime>(
			"ColorOverLifetime", "Jimara/ColorOverLifetime", "Sets particle color over lifetime");
		report(factory);
	}
}
