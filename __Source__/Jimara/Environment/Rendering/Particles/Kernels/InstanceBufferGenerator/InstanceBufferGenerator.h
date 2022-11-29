#pragma once
#include "../../../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	/// <summary>
	/// A particle kernel that generates transform matrices for particles.
	/// <para/> Notes:
	///	<para/> 0. Used internally by the Particle Systems and user does not have to think too much about it;
	/// <para/> 1. Executed after ParticleSimulationStepKernel.
	/// </summary>
	class JIMARA_API ParticleInstanceBufferGenerator : public virtual GraphicsSimulation::Kernel {
	public:
		/// <summary>
		/// Settings for a task
		/// </summary>
		struct JIMARA_API TaskSettings {
			/// <summary> 
			/// World matrix of the particle system, if the simulation runs in local space, Math::Identity() otherwise;
			/// Same, but multiplied by a viewport-facing rotation if we have camera-facing quads instead of meshes.
			/// </summary>
			alignas(16) Matrix4 baseTransform = Math::Identity();	// Bytes [0 - 64)

			/// <summary> Indirection/Index Wrangle bindless buffer id </summary>
			alignas(4) uint32_t particleIndirectionBufferId = 0u;	// Bytes [64 - 68)

			/// <summary> Bindless buffer id for ParticleState </summary>
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [68 - 72)

			/// <summary> Bindless buffer id for the resulting Matrix4 instance buffer </summary>
			alignas(4) uint32_t instanceBufferId = 0u;				// Bytes [72 - 76)

			/// <summary> Index of the first particle's instance within the instanceBuffer </summary>
			alignas(4) uint32_t instanceStartId = 0u;				// Bytes [76 - 80)

			/// <summary> Number of particles within the particle system (name is important; ) </summary>
			alignas(4) uint32_t particleCount = 0u;					// Bytes [80 - 84)

			/// <summary> Bindless buffer id for the 'Live Particle Count' buffer </summary>
			alignas(4) uint32_t liveParticleCountBufferId = 0u;		// Bytes [84 - 88)

			/// <summary> Padding words... </summary>
			alignas(4) uint32_t pad_0 = 0u, pad_1 = 0u;				// Bytes [88 - 96)
		};
		static_assert(sizeof(TaskSettings) == 96);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleInstanceBufferGenerator();

		/// <summary> Singleton instance of ParticleInstanceBufferGenerator
		static const ParticleInstanceBufferGenerator* Instance();

		/// <summary>
		/// Creates a combined particle kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Instance of the particle kernel </returns>
		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;

	private:
		// No externally created instances, please...
		ParticleInstanceBufferGenerator();
	};
}
