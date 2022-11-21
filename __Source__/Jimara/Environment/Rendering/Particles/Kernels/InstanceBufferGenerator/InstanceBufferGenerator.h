#pragma once
#include "../../ParticleKernel.h"


namespace Jimara {
	/// <summary>
	/// A particle kernel that generates transform matrices for particles.
	/// <para/> Notes:
	///	<para/> 0. Used internally by the Particle Systems and user does not have to think too much about it;
	/// <para/> 1. Executed after ParticleSimulationStepKernel.
	/// </summary>
	class JIMARA_API ParticleInstanceBufferGenerator : public virtual ParticleKernel {
	public:
		/// <summary>
		/// Settings for a task
		/// </summary>
		struct JIMARA_API TaskSettings {
			/// <summary> Bindless buffer id for ParticleState </summary>
			alignas(4) uint32_t particleStateBufferId = 0u;			// Bytes [0 - 4)

			/// <summary> Bindless buffer id for the resulting Matrix4 instance buffer </summary>
			alignas(4) uint32_t instanceBufferId = 0u;				// Bytes [0 - 8)

			/// <summary> Index of the first particle's instance within the instanceBuffer </summary>
			alignas(4) uint32_t instanceStartId = 0u;				// Bytes [0 - 12)

			/// <summary> Number of particles within the particle system (name is important; ) </summary>
			alignas(4) uint32_t particleCount = 0u;					// Bytes [0 - 16)

			/// <summary> 
			/// World matrix of the particle system, if the simulation runs in local space, Math::Identity() otherwise;
			/// Same, but multiplied by a viewport-facing rotation if we have camera-facing quads instead of meshes.
			/// </summary>
			alignas(16) Matrix4 baseTransform = Math::Identity();	// Bytes [16 - 80)
		};
		static_assert(sizeof(TaskSettings) == 80);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleInstanceBufferGenerator();

		/// <summary> Singleton instance of ParticleInstanceBufferGenerator
		static const ParticleInstanceBufferGenerator* Instance();

		/// <summary>
		/// Creates a combined particle kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Instance of the particle kernel </returns>
		virtual Reference<ParticleKernel::Instance> CreateInstance(SceneContext* context)const override;

	private:
		// No externally created instances, please...
		ParticleInstanceBufferGenerator();
	};
}
