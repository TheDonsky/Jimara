#pragma once
#include "../../ParticleKernel.h"


namespace Jimara {
	/// <summary>
	/// The first kernel that executes during simulation is ParticleWrangleStepKernel.
	/// It's responsibility is to take a look at the particle states, evaluate which particles have died,
	/// regenerate the 'Particle Indirection Buffer' accordingly and update live particle count accordingly.
	/// <para/> Note: Particle count will be updated once more after the spawning step is complete.
	/// </summary>
	class JIMARA_API ParticleWrangleStepKernel : public virtual ParticleKernel {
	public:
		class JIMARA_API Task : public virtual ParticleKernel::Task {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			Task(SceneContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~Task();

			/// <summary>
			/// Sets target ParticleBuffers
			/// </summary>
			/// <param name="buffers"> ParticleBuffers </param>
			void SetBuffers(ParticleBuffers* buffers);

			/// <summary> Updates settings buffer </summary>
			virtual void Synchronize()override;

		private:
			// Lock for m_buffers
			SpinLock m_bufferLock;

			// Target buffers
			Reference<ParticleBuffers> m_buffers;

			// ParticleBuffers from the last Synchronize() call
			Reference<ParticleBuffers> m_lastBuffers;
		};

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleWrangleStepKernel();

		/// <summary>
		/// Creates an instance of the kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Kernel instance </returns>
		virtual Reference<ParticleKernel::Instance> CreateInstance(SceneContext* context)const override;

	private:
		// No custom instances can be created
		ParticleWrangleStepKernel();

		// Some private classes reside in here..
		struct Helpers;
	};
}
