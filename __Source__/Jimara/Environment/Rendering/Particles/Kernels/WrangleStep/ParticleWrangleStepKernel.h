#pragma once
#include "../../../../GraphicsSimulation/GraphicsSimulation.h"
#include "../../ParticleBuffers.h"


namespace Jimara {
	/// <summary>
	/// The first kernel that executes during simulation is ParticleWrangleStepKernel.
	/// It's responsibility is to take a look at the particle states, evaluate which particles have died,
	/// regenerate the 'Particle Indirection Buffer' accordingly and update live particle count accordingly.
	/// <para/> Note: Particle count will be updated once more after the spawning step is complete.
	/// </summary>
	class JIMARA_API ParticleWrangleStepKernel : public virtual GraphicsSimulation::Kernel {
	public:
		/// <summary>
		/// Simulation task responsible for updating live particle count and regenerating the indirection buffer
		/// </summary>
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			/// <param name="particleState"> Indirection buffer Particle state </param>
			/// <param name="indirectionBuffer"> Indirection buffer for 'index wrangling' </param>
			/// <param name="liveParticleCount"> Single element buffer holding count of 'alive' particles at the end of the last frame </param>
			Task(SceneContext* context,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* particleState,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount);

			/// <summary> Virtual destructor </summary>
			virtual ~Task();

		private:
			// Indirection buffer Particle state
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleState;

			// Indirection buffer for 'index wrangling'
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_indirectionBuffer;

			// Single element buffer holding count of 'alive' particles at the end of the last frame
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCount;
		};

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleWrangleStepKernel();

		/// <summary>
		/// Creates an instance of the kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Kernel instance </returns>
		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;

	private:
		// No custom instances can be created
		ParticleWrangleStepKernel();

		// Some private classes reside in here..
		struct Helpers;
	};
}
