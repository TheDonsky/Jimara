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

		/// <summary> System flags </summary>
		enum class JIMARA_API Flag : uint32_t {
			/// <summary> No flags </summary>
			NONE = 0u,

			/// <summary> Will cause simulation of this system to run in local space </summary>
			SIMULATE_IN_LOCAL_SPACE = (1u << 0u),

			/// <summary> If simulation is running in world space and this flag is set, the new particles should not inherit the system's rotation </summary>
			INDEPENDENT_PARTICLE_ROTATION = (1u << 1u),

			/// <summary> If this flag is set, particle system will only have to perform a simulation step if it is visible </summary>
			DO_NOT_SIMULATE_IF_INVISIBLE = (1u << 2u)
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		inline ParticleSystemInfo(SceneContext* context) : m_context(context) { assert(context != nullptr); }

		/// <summary> Scene context </summary>
		inline SceneContext* Context()const { return m_context; }

		/// <summary> Simulation time 'mode' </summary>
		inline TimeMode TimestepMode()const { return m_timeMode; }

		/// <summary>
		/// Sets simulation time mode
		/// </summary>
		/// <param name="mode"> Time mode to use </param>
		inline void SetTimeMode(TimeMode mode) { m_timeMode = Math::Min(Math::Max(TimeMode::NO_TIME, mode), TimeMode::PHYSICS_DELTA_TIME); }

		/// <summary> System simulation flags </summary>
		inline Flag Flags()const { return m_flags; }

		/// <summary>
		/// Updates simulation flags
		/// </summary>
		/// <param name="flags"> Flags to use </param>
		inline void SetFlags(Flag flags) { m_flags = flags; }

		/// <summary>
		/// Checks if the system has given flag set
		/// </summary>
		/// <param name="flag"> Flag (or collection of flags) to check </param>
		/// <returns> True if all bits from flag are set </returns>
		inline bool HasFlag(Flag flag)const { return (static_cast<uint32_t>(m_flags) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag); }

		/// <summary>
		/// Sets flag or collection of flags
		/// </summary>
		/// <param name="flag"> Flag to set </param>
		/// <param name="value"> Flag value </param>
		inline void SetFlag(Flag flag, bool value) { 
			if (value) m_flags = static_cast<Flag>(static_cast<uint32_t>(m_flags) | static_cast<uint32_t>(flag));
			else m_flags = static_cast<Flag>(static_cast<uint32_t>(m_flags) & (~static_cast<uint32_t>(flag)));
		}

		/// <summary> World-space transform of the particle system </summary>
		virtual Matrix4 WorldTransform()const = 0;

		/// <summary>
		/// Local-space simulation boundaries and on-screen size limits
		/// </summary>
		/// <param name="bbox"> Local bounding box </param>
		/// <param name="minOnScreenSize"> Minimal fractional on-screen size to displaay the system </param>
		/// <param name="maxOnScreenSize"> Maximal fractional on-screen size to displaay the system (negative values mean infinity) </param>
		virtual void GetCullingSettings(AABB& bbox, float& minOnScreenSize, float& maxOnScreenSize)const = 0;


	private:
		// Context
		const Reference<SceneContext> m_context;

		// Simulation time 'mode'
		TimeMode m_timeMode = TimeMode::SCALED_DELTA_TIME;

		// Simulation flags
		Flag m_flags = Flag::NONE;
	};
}
