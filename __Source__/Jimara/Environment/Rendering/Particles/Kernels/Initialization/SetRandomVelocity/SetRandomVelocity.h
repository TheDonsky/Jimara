#pragma once
#include "../../../ParticleKernels.h"


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::InitializationKernels::SetRandomVelocity);

	namespace InitializationKernels {
		class JIMARA_API SetRandomVelocity : public virtual ParticleInitializationTask {
		public:
			SetRandomVelocity(SceneContext* context);

			virtual ~SetRandomVelocity();

			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		protected:
			virtual void SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer)override;

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

	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::SetRandomVelocity>(const Callback<const Object*>& report);
}
