#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationStepKernel : public virtual GraphicsSimulation::Kernel {
	public:
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
		public:
			Task(SceneContext* context);

			virtual ~Task();

			void SetBuffers(ParticleBuffers* buffers);

			size_t InitializationTaskCount()const;

			ParticleInitializationTask* InitializationTask(size_t index)const;

			void SetInitializationTask(size_t index, ParticleInitializationTask* task);

			void AddInitializationTask(ParticleInitializationTask* task);

			virtual void Synchronize()override;

			virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& reportDependency)const override;

		private:
			Reference<ParticleBuffers> m_buffers;
			Reference<ParticleBuffers> m_lastBuffers;
			Stacktor<Reference<ParticleInitializationTask>> m_initializationTasks;
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
