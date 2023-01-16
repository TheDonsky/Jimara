#pragma once
#include "../../Scene/Scene.h"


namespace Jimara {
	/// <summary>
	/// Interface that exposes particle system details to the particle kernels and transforms 
	/// </summary>
	class JIMARA_API ParticleSystemInfo : public virtual Object {
	public:
		/// <summary> Simulation time 'mode' </summary>
		enum class JIMARA_API TimeMode : uint32_t {
			/// <summary> Time does not 'flow'; delta time is always 0 </summary>
			NO_TIME = 0u,

			/// <summary> Timestep is unscaled delta time </summary>
			UNSCALED_DELTA_TIME = 1u,

			/// <summary> Timestep is scaled delta time </summary>
			SCALED_DELTA_TIME = 2u,

			/// <summary> Timestep is tied to physics simulation (not adviced) </summary>
			PHYSICS_DELTA_TIME = 3u
		};

		/// <summary> Simulation space world/local </summary>
		enum class SimulationMode : uint32_t {
			/// <summary> Simulation of this system will run in world-space </summary>
			WORLD_SPACE = 0u,

			/// <summary> Simulation of this system will run in local-space </summary>
			LOCAL_SPACE = 1u
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		inline ParticleSystemInfo(SceneContext* context) : m_context(context) { assert(context != nullptr); }

		/// <summary>
		/// Sets simulation time mode
		/// </summary>
		/// <param name="mode"> Time mode to use </param>
		inline void SetTimeMode(TimeMode mode) { m_timeMode = Math::Min(Math::Max(TimeMode::NO_TIME, mode), TimeMode::PHYSICS_DELTA_TIME); }

		/// <summary> Simulation time 'mode' </summary>
		inline TimeMode TimestepMode()const { return m_timeMode; }

		/// <summary>
		/// Sets simulation space to world/local
		/// </summary>
		/// <param name="mode"> Simulation space </param>
		inline void SetSimulationSpace(SimulationMode mode) { m_simulationMode = Math::Min(Math::Max(SimulationMode::WORLD_SPACE, mode), SimulationMode::WORLD_SPACE); }

		/// <summary> Simulation space (world/local) </summary>
		inline SimulationMode SimulationSpace()const { return m_simulationMode; }

		/// <summary> World-space transform of the particle system </summary>
		virtual Matrix4 WorldTransform()const = 0;


	private:
		// Context
		const Reference<SceneContext> m_context;

		// Simulation time 'mode'
		TimeMode m_timeMode = TimeMode::SCALED_DELTA_TIME;

		// Simulation space
		SimulationMode m_simulationMode = SimulationMode::WORLD_SPACE;
	};
}
