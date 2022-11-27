#pragma once
#include "../../ParticleKernel.h"


namespace Jimara {
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
		/// Creates an instance of a simulation step kernel
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
