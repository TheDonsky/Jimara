#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationStepKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			Task(const ParticleSystemInfo* systemInfo);

			virtual ~Task();

			inline const ParticleSystemInfo* SystemInfo()const { return m_systemInfo; }

			void SetBuffers(ParticleBuffers* buffers);

			ParticleTaskSet<ParticleInitializationTask>& InitializationTasks() { return m_initializationTasks; }

			virtual void Synchronize()override;

			virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& reportDependency)const override;

		private:
			const Reference<const ParticleSystemInfo> m_systemInfo;
			Reference<ParticleBuffers> m_buffers;
			Reference<ParticleBuffers> m_lastBuffers;
			ParticleTaskSet<ParticleInitializationTask> m_initializationTasks;
		};

		virtual ~ParticleInitializationStepKernel();

		/// <summary>
		/// Creates an instance of the kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Kernel instance </returns>
		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;


	private:
		ParticleInitializationStepKernel();

		struct Helpers;
	};
}
