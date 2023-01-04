#pragma once
#include "../../ParticleKernels.h"
#include "../../../../../Math/Curves.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleTimestep::SizeOverLifetime);

	namespace ParticleTimestep {
		/// <summary>
		/// A particle timestep task that sets particle size over lifetime
		/// </summary>
		class JIMARA_API SizeOverLifetime : public virtual ParticleTimestepTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="initializationTask"> Particle Initialization task </param>
			SizeOverLifetime(GraphicsSimulation::Task* initializationTask);

			/// <summary> Virtual destructor </summary>
			virtual ~SizeOverLifetime();

			/// <summary>
			/// Sets simulation time mode
			/// </summary>
			/// <param name="timeMode"> Time mode to use </param>
			virtual void SetTimeMode(TimeMode timeMode)override;

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
			struct SimulationTaskSettings {
				alignas(4) uint32_t timeMode = static_cast<uint32_t>(TimeMode::SCALED_DELTA_TIME);
				alignas(4) uint32_t stateBufferId = 0u;
				alignas(4) uint32_t taskThreadCount = 0u;
			} m_simulationSettings;
			TimelineCurve<float> m_sizeCurve;
			TimelineCurve<Vector2> m_size2Curve;
			TimelineCurve<Vector3> m_size3Curve;
			TimelineCurve<Vector4> m_size4Curve;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::SizeOverLifetime>(const Callback<const Object*>& report);
}