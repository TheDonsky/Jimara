#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleInitialization::SetRandomColor);

	namespace ParticleInitialization {
		/// <summary>
		/// A particle initialization kernel that randomizes particle color
		/// </summary>
		class JIMARA_API SetRandomColor : public virtual ParticleInitializationTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="systemInfo"> Particle system data </param>
			SetRandomColor(const ParticleSystemInfo* systemInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~SetRandomColor();

			/// <summary>
			/// Records modifiable parameters
			/// </summary>
			/// <param name="recordElement"> Modifiable parameters are reported throigh this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

			/// <summary> Blending mode between first and second color values </summary>
			enum class Mode : uint32_t {
				/// <summary> Randomizes a single value and interpolates in-between colors </summary>
				INTERPOLATE_COLOR = 0,

				/// <summary> Randomizes a value per-channel and interpolates in-between channels </summary>
				INTERPOLATE_CHANNELS = 1
			};

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

				alignas(4) float aR = 1.0f;								// Bytes [20 - 24)
				alignas(4) float aG = 1.0f;								// Bytes [24 - 28)
				alignas(4) float aB = 1.0f;								// Bytes [28 - 32)
				alignas(4) float aA = 1.0f;								// Bytes [32 - 36)

				alignas(4) float bR = 1.0f;								// Bytes [36 - 40)
				alignas(4) float bG = 1.0f;								// Bytes [40 - 44)
				alignas(4) float bB = 1.0f;								// Bytes [44 - 48)
				alignas(4) float bA = 1.0f;								// Bytes [48 - 52)

				alignas(4) Mode blendMode = Mode::INTERPOLATE_COLOR;	// Bytes [52 - 56)
			} m_simulationSettings;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleInitialization::SetRandomColor>(const Callback<const Object*>& report);
}
