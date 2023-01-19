#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleTimestep::ApplyGravity);
	
	namespace ParticleTimestep {
		/// <summary>
		/// A particle timestep task that adds gravity to the particles
		/// </summary>
		class JIMARA_API ApplyGravity : public virtual ParticleTimestepTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="systemInfo"> Particle system info </param>
			ApplyGravity(const ParticleSystemInfo* systemInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~ApplyGravity();

			/// <summary>
			/// Records modifiable parameters
			/// </summary>
			/// <param name="recordElement"> Modifiable parameters are reported throigh this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		protected:
			/// <summary>
			/// Updates SimulationTaskSettings
			/// </summary>
			/// <param name="particleBudget"> SimulationTaskSettings::particleBudget </param>
			/// <param name="findBuffer"> SimulationTaskSettings::stateBufferId = findBuffer(ParticleState::BufferId()) </param>
			virtual void SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer)override;

			/// <summary> Updates SimulationTaskSettings::taskThreadCount and sets settings buffer of the GraphicsSimulation::Tesk </summary>
			virtual void UpdateSettings()override;

		private:
			const Reference<const ParticleSystemInfo> m_systemInfo;
			struct SimulationTaskSettings {
				alignas(16) Vector3 gravity = Vector3(0.0f);
				alignas(4) uint32_t timeMode = static_cast<uint32_t>(ParticleSystemInfo::TimeMode::SCALED_DELTA_TIME);
				alignas(4) uint32_t stateBufferId = 0u;
				alignas(4) uint32_t taskThreadCount = 0u;
			} m_simulationSettings;
			float m_gravityScale = 1.0f;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::ApplyGravity>(const Callback<const Object*>& report);
}
