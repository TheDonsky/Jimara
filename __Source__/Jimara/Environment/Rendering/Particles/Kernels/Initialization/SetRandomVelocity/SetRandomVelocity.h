#pragma once
#include "../../../ParticleKernels.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::InitializationKernels::SetRandomVelocity);

	namespace InitializationKernels {
		/// <summary>
		/// A particle initialization kernel that randomizes particle velocity
		/// </summary>
		class JIMARA_API SetRandomVelocity : public virtual ParticleInitializationTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			SetRandomVelocity(SceneContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SetRandomVelocity();

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
				alignas(4) float minimal = 0.0f;						// Bytes [20 - 24)
				alignas(4) float maximal = 1.0f;						// Bytes [24 - 28)
			} m_simulationSettings;
			class Kernel;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::SetRandomVelocity>(const Callback<const Object*>& report);
}
