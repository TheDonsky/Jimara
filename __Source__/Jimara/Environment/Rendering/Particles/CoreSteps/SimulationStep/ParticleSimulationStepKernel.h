#pragma once
#include "../InitializationStep/ParticleInitializationStepKernel.h"


namespace Jimara {
	/// <summary>
	/// After the primary simulation kernels are done with the particle state, ParticleSimulationStep kernel gets executed.
	/// It is responsible for decrementing lifetime and moving the particles around using velocity and angular velocity.
	/// </summary>
	class JIMARA_API ParticleSimulationStep : public virtual GraphicsSimulation::Task {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="systemInfo"> Particle system data </param>
		ParticleSimulationStep(const ParticleSystemInfo* systemInfo);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleSimulationStep();

		/// <summary>
		/// Sets target ParticleBuffers
		/// </summary>
		/// <param name="buffers"> ParticleBuffers </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary> Timestep subtask collection </summary>
		inline ParticleTaskSet<ParticleTimestepTask>& TimestepTasks() { return m_timestepTasks; }

		/// <summary> Updates settings buffer </summary>
		virtual void Synchronize()override;

		/// <summary>
		/// Invoked by ParticleSimulation during the graphics synch point;
		/// <para/> Reports simulation tasks as dependencies, whcih themselves rely on spawning step
		/// </summary>
		/// <param name="recordDependency"> Each task this one depends on will be reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		/// <summary> Particle initialization step </summary>
		inline ParticleInitializationStep* InitializationStep()const { return m_initializationStep; }

	private:
		// Particle system data
		const Reference<const ParticleSystemInfo> m_systemInfo;

		// Initialization step
		const Reference<ParticleInitializationStep> m_initializationStep;

		// Lock for m_buffers
		SpinLock m_bufferLock;

		// Target buffers
		Reference<ParticleBuffers> m_buffers;

		// ParticleBuffers from the last Synchronize() call
		Reference<ParticleBuffers> m_lastBuffers;

		// ParticleState buffer binding from the last Synchronize() call
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer;

		// Time step tasks
		ParticleTaskSet<ParticleTimestepTask> m_timestepTasks;

		// Private constructor
		ParticleSimulationStep(const ParticleSystemInfo* systemInfo, Reference<ParticleInitializationStep> initializationStep);

	private:
		// Some private classes reside in here..
		struct Helpers;
	};
}
