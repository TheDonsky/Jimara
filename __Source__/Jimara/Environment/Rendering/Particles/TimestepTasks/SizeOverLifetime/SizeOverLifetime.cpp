#include "SizeOverLifetime.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace ParticleTimestep {
		SizeOverLifetime::SizeOverLifetime(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/TimestepTasks/SizeOverLifetime/SizeOverLifetime.comp"), systemInfo->Context())
			, m_sizeCurve(systemInfo->Context()->Graphics()->Device(), "Curve", "Size over lifetime", std::vector<Reference<const Object>> {
			Object::Instantiate<Serialization::CurveGraphCoordinateLimits>(0.0f, 1.0f, 0.0f)
		}) {
			Unused(systemInfo);
		}

		SizeOverLifetime::~SizeOverLifetime() {}

		void SizeOverLifetime::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			m_sizeCurve.GetFields(recordElement);
		}

		void SizeOverLifetime::SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer) {
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.taskThreadCount = particleBudget;
		}

		void SizeOverLifetime::UpdateSettings() {
			const Graphics::ArrayBufferReference<GraphicsTimelineCurve<float>::KeyFrame> curveBuffer = m_sizeCurve.GetCurveBuffer();
			if (curveBuffer == nullptr) {
				Context()->Log()->Error("SizeOverLifetime::UpdateSettings - Failed to get curve data on GPU! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				m_sizeCurveBinding = nullptr;
				m_simulationSettings.curveBufferId = 0u;
			}
			else if (m_sizeCurveBinding == nullptr || m_sizeCurveBinding->BoundObject() != curveBuffer) {
				m_sizeCurveBinding = Context()->Graphics()->Bindless().Buffers()->GetBinding(curveBuffer);
				if (m_sizeCurveBinding == nullptr) {
					Context()->Log()->Error("SizeOverLifetime::UpdateSettings - Failed to get bindless index for the curve! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					m_simulationSettings.curveBufferId = 0u;
				}
				else m_simulationSettings.curveBufferId = m_sizeCurveBinding->Index();
			}
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::SizeOverLifetime>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleTimestepTask::Factory::Create<ParticleTimestep::SizeOverLifetime>(
			"SizeOverLifetime", "Jimara/SizeOverLifetime", "Sets particle size over lifetime");
		report(factory);
	}
}
