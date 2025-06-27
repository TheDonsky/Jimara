#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleInitialization::SetRandomAngularVelocity);

	namespace ParticleInitialization {
		/// <summary>
		/// A particle initialization kernel that randomizes particle angular velocity
		/// </summary>
		class JIMARA_API SetRandomAngularVelocity : public virtual ParticleInitializationTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="systemInfo"> Particle system data </param>
			SetRandomAngularVelocity(const ParticleSystemInfo* systemInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~SetRandomAngularVelocity();

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
			/// <param name="indirectionBuffer"> SimulationTaskSettings::particleIndirectionBufferId </param>
			/// <param name="liveParticleCountBuffer"> SimulationTaskSettings::liveParticleCountBufferId </param>
			/// <param name="findBuffer"> SimulationTaskSettings::stateBufferId = findBuffer(ParticleState::BufferId()) </param>
			virtual void SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer)override;

			/// <summary> Updates SimulationTaskSettings::taskThreadCount and sets settings buffer of the GraphicsSimulation::Tesk </summary>
			virtual void UpdateSettings()override;

		private:
			struct SimulationTaskSettings {
				alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [0 - 4)
				alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [4 - 8)
				alignas(4) uint32_t stateBufferId = 0u;					// Bytes [8 - 12)
				alignas(4) uint32_t particleBudget = 0u;				// Bytes [12 - 16)
				alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [16 - 20)
				
				alignas(4) float minimalX = 0.0f;						// Bytes [20 - 24)
				alignas(4) float minimalY = 0.0f;						// Bytes [24 - 28)
				alignas(4) float minimalZ = 0.0f;						// Bytes [28 - 32)
				
				alignas(4) float maximalX = 0.0f;						// Bytes [32 - 36)
				alignas(4) float maximalY = 0.0f;						// Bytes [36 - 40)
				alignas(4) float maximalZ = 0.0f;						// Bytes [40 - 44)

				alignas(4) uint32_t pad = {};							// Bytes [44 - 48)
			} m_simulationSettings;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomAngularVelocity>(const Callback<const Object*>& report);
}
