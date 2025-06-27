#include "SetRandomColor.h"
#include "../../ParticleState.h"
#include "../../CombinedParticleKernel.h"
#include "../../../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../../../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace ParticleInitialization {
		SetRandomColor::SetRandomColor(const ParticleSystemInfo* systemInfo)
			: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>(
				"Jimara/Environment/Rendering/Particles/InitializationTasks/SetRandomColor/SetRandomColor.comp"), systemInfo->Context()) {
		}

		SetRandomColor::~SetRandomColor() {}

		void SetRandomColor::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				static_assert(offsetof(Vector4, x) == 0u);

				static_assert(
					(offsetof(SimulationTaskSettings, aG) - offsetof(SimulationTaskSettings, aR)) ==
					(offsetof(Vector4, y) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, aB) - offsetof(SimulationTaskSettings, aR)) ==
					(offsetof(Vector4, z) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, aB) - offsetof(SimulationTaskSettings, aG)) ==
					(offsetof(Vector4, z) - offsetof(Vector4, y)));
				static_assert(
					(offsetof(SimulationTaskSettings, aA) - offsetof(SimulationTaskSettings, aR)) ==
					(offsetof(Vector4, w) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, aA) - offsetof(SimulationTaskSettings, aB)) ==
					(offsetof(Vector4, w) - offsetof(Vector4, z)));
				Vector4& a = *reinterpret_cast<Vector4*>((void*)(&m_simulationSettings.aR));
				assert((&a.x) == (&m_simulationSettings.aR));
				assert((&a.y) == (&m_simulationSettings.aG));
				assert((&a.z) == (&m_simulationSettings.aB));
				assert((&a.w) == (&m_simulationSettings.aA));

				static_assert(
					(offsetof(SimulationTaskSettings, bG) - offsetof(SimulationTaskSettings, bR)) ==
					(offsetof(Vector4, y) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, bB) - offsetof(SimulationTaskSettings, bR)) ==
					(offsetof(Vector4, z) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, bB) - offsetof(SimulationTaskSettings, bG)) ==
					(offsetof(Vector4, z) - offsetof(Vector4, y)));
				static_assert(
					(offsetof(SimulationTaskSettings, bA) - offsetof(SimulationTaskSettings, bR)) ==
					(offsetof(Vector4, w) - offsetof(Vector4, x)));
				static_assert(
					(offsetof(SimulationTaskSettings, bA) - offsetof(SimulationTaskSettings, bB)) ==
					(offsetof(Vector4, w) - offsetof(Vector4, z)));
				Vector4& b = *reinterpret_cast<Vector4*>((void*)(&m_simulationSettings.bR));
				assert((&b.x) == (&m_simulationSettings.bR));
				assert((&b.y) == (&m_simulationSettings.bG));
				assert((&b.z) == (&m_simulationSettings.bB));
				assert((&b.w) == (&m_simulationSettings.bA));

				JIMARA_SERIALIZE_FIELD(a, "Color A", "First Color",
					Object::Instantiate<Serialization::ColorAttribute>());
				if (m_simulationSettings.aR > m_simulationSettings.bR)
					m_simulationSettings.bR = m_simulationSettings.aR;
				if (m_simulationSettings.aG > m_simulationSettings.bG)
					m_simulationSettings.bG = m_simulationSettings.aG;
				if (m_simulationSettings.aB > m_simulationSettings.bB)
					m_simulationSettings.bB = m_simulationSettings.aB;

				JIMARA_SERIALIZE_FIELD(b, "Color B", "Second Color",
					Object::Instantiate<Serialization::ColorAttribute>());
				if (m_simulationSettings.aR > m_simulationSettings.bR)
					m_simulationSettings.aR = m_simulationSettings.bR;
				if (m_simulationSettings.aG > m_simulationSettings.bG)
					m_simulationSettings.aG = m_simulationSettings.bG;
				if (m_simulationSettings.aB > m_simulationSettings.bB)
					m_simulationSettings.aB = m_simulationSettings.bB;

				JIMARA_SERIALIZE_FIELD(m_simulationSettings.blendMode, "Blend Mode", "Blending mode",
					Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Mode>>>(false,
						"INTERPOLATE_COLOR", Mode::INTERPOLATE_COLOR,
						"INTERPOLATE_CHANNELS", Mode::INTERPOLATE_CHANNELS));
			};
		}

		void SetRandomColor::SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) {
			m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
			m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
			m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			m_simulationSettings.particleBudget = particleBudget;
		}

		void SetRandomColor::UpdateSettings() {
			m_simulationSettings.taskThreadCount = SpawnedParticleCount();
			SetSettings(m_simulationSettings);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomColor>(const Callback<const Object*>& report) {
		static const Reference<const Object> factory = ParticleInitializationTask::Factory::Create<ParticleInitialization::SetRandomColor>(
			"SetRandomColor", "Jimara/SetRandomColor", "Sets random Color/Euler-Angles per newly spawned particle");
		report(factory);
	}
}
