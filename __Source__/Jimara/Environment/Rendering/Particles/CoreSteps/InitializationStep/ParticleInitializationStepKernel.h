#pragma once
#include "../../ParticleKernels.h"


namespace Jimara {
	/// <summary>
	/// Particle simulation has two primary steps. 
	/// First is this one (ParticleInitializationStep) and it's responsibility is to run initialization tasks and increment live particle count.
	/// After this come timestep tasks and timestep kernel.
	/// (Internally created by ParticleSimulationStep)
	/// </summary>
	class JIMARA_API ParticleInitializationStep : public virtual GraphicsSimulation::Task {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="systemInfo"> Particle system info </param>
		ParticleInitializationStep(const ParticleSystemInfo* systemInfo);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleInitializationStep();

		/// <summary> Particle system information </summary>
		inline const ParticleSystemInfo* SystemInfo()const { return m_systemInfo; }

		/// <summary>
		/// Sets particle buffers for the initialization step and all underlying tasks
		/// </summary>
		/// <param name="buffers"> Particle buffers </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary> Particle initialization task collection </summary>
		ParticleTaskSet<ParticleInitializationTask>& InitializationTasks() { return m_initializationTasks; }

		/// <summary> Updates task settings (invoked by simulation system; non of the user's concern) </summary>
		virtual void Synchronize()override;

		/// <summary>
		/// Retrieves tasks that have to be executed before this one (invoked by simulation system; non of the user's concern)
		/// </summary>
		/// <param name="reportDependency"> Dependencies will be reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& reportDependency)const override;

	private:
		// Particle system info
		const Reference<const ParticleSystemInfo> m_systemInfo;
		
		// Buffers, set during last SetBuffers() call
		Reference<ParticleBuffers> m_buffers;

		// Buffers from the last synch point
		Reference<ParticleBuffers> m_lastBuffers;

		// Particle initialization task collection
		ParticleTaskSet<ParticleInitializationTask> m_initializationTasks;

		// Private stuff resides here
		struct Helpers;
	};
}
