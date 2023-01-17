#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleInitialization::PlaceInSphere);

	namespace ParticleInitialization {
		/// <summary>
		/// A particle initialization kernel that randomizes particle position inside a sphere
		/// </summary>
		class JIMARA_API PlaceInSphere : public virtual ParticleInitializationTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="systemInfo"> Particle system data </param>
			PlaceInSphere(const ParticleSystemInfo* systemInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~PlaceInSphere();

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
			const Reference<const ParticleSystemInfo> m_systemInfo;
			struct SimulationTaskSettings {
				alignas(16) Vector3 matX = Vector3(1.0f, 0.0f, 0.0f);	// Bytes [0 - 12)
				alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [12 - 16)
				alignas(16) Vector3 matY = Vector3(0.0f, 1.0f, 0.0f);	// Bytes [16 - 28)
				alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [28 - 32)
				alignas(16) Vector3 matZ = Vector3(0.0f, 0.0f, 1.0f);	// Bytes [32 - 44)
				alignas(4) uint32_t stateBufferId = 0u;					// Bytes [44 - 48)
				alignas(4) uint32_t particleBudget = 0u;				// Bytes [48 - 52)
				alignas(4) uint32_t taskThreadCount = 0u;				// Bytes [52 - 56)
				alignas(4) float radius = 1.0f;							// Bytes [56 - 60)
				alignas(4) uint32_t pad = 0u;							// Bytes [60 - 64)
			} m_simulationSettings;
			static_assert(sizeof(SimulationTaskSettings) == 64u);
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::PlaceInSphere>(const Callback<const Object*>& report);
}
