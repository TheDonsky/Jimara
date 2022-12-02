#pragma once
#include "../../../ParticleKernels.h"


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::InitializationKernels::PlaceOnSphere);

	namespace InitializationKernels {
		class JIMARA_API PlaceOnSphere : public virtual ParticleInitializationTask {
		public:
			PlaceOnSphere(SceneContext* context);

			virtual ~PlaceOnSphere();

			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		protected:
			virtual void SetBuffers(
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
				const ParticleBuffers::BufferSearchFn& findBuffer) override;

			virtual void UpdateSettings()override;

		private:
			struct SimulationTaskSettings {
				alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [0 - 4)
				alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [4 - 8)
				alignas(4) uint32_t stateBufferId = 0u;					// Bytes [8 - 12)
				alignas(4) uint32_t particleBudget = 0u;				// Bytes [12 - 16)
				alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [16 - 20)
				alignas(4) float radius = 1.0f;							// Bytes [20 - 24)
			} m_simulationSettings;
			class Kernel;
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<InitializationKernels::PlaceOnSphere>(const Callback<const Object*>& report);
}
