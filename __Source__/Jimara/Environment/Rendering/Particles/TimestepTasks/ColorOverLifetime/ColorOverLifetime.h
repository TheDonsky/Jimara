#pragma once
#include "../../ParticleKernels.h"
#include "../../../../../Math/GraphicsCurves.h"


namespace Jimara {
	/// <summary> Let the engine know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ParticleTimestep::ColorOverLifetime);

	namespace ParticleTimestep {
		/// <summary>
		/// A particle timestep task that sets particle color over lifetime
		/// </summary>
		class JIMARA_API ColorOverLifetime : public virtual ParticleTimestepTask {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="initializationTask"> Particle Initialization task </param>
			/// <param name="systemInfo"> Particle system info </param>
			ColorOverLifetime(GraphicsSimulation::Task* initializationTask, const ParticleSystemInfo* systemInfo);

			/// <summary> Virtual destructor </summary>
			virtual ~ColorOverLifetime();

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
				alignas(4) uint32_t curveBufferId = 0u;
				alignas(4) uint32_t stateBufferId = 0u;
				alignas(4) uint32_t taskThreadCount = 0u;
			} m_simulationSettings;
			GraphicsTimelineCurve<Vector4> m_colorCurve;
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_colorCurveBinding;
		};
	}

	/// <summary> Reports the factory of kernels </summary>
	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleTimestep::ColorOverLifetime>(const Callback<const Object*>& report);
}
