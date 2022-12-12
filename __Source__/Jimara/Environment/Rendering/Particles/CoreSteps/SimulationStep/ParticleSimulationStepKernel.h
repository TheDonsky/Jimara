#pragma once
#include "../InitializationStep/ParticleInitializationStepKernel.h"


namespace Jimara {
	/// <summary>
	/// After the primary simulation kernels are done with the particle state, ParticleSimulationStep kernel gets executed.
	/// It is responsible for decrementing lifetime and moving the particles around using velocity and angular velocity.
	/// </summary>
	class JIMARA_API ParticleSimulationStep : public virtual GraphicsSimulation::Task {
	public:
		/// <summary> Simulation time 'mode' </summary>
		typedef ParticleTimestepTask::TimeMode TimeMode;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		ParticleSimulationStep(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleSimulationStep();

		/// <summary>
		/// Sets target ParticleBuffers
		/// </summary>
		/// <param name="buffers"> ParticleBuffers </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary>
		/// Sets simulation time scale in addition to time mode
		/// </summary>
		/// <param name="timeScale"> Time scale </param>
		void SetTimeScale(float timeScale);

		/// <summary>
		/// Sets time mode
		/// </summary>
		/// <param name="timeMode"> Timestep settings </param>
		void SetTimeMode(TimeMode timeMode);

		size_t TimestepTaskCount()const;

		ParticleTimestepTask* TimestepTask(size_t index)const;

		void SetTimestepTask(size_t index, ParticleTimestepTask* task);

		void AddTimestepTask(ParticleTimestepTask* task);

		/// <summary> Updates settings buffer </summary>
		virtual void Synchronize()override;

		/// <summary>
		/// Invoked by ParticleSimulation during the graphics synch point;
		/// <para/> Reports simulation tasks as dependencies, whcih themselves rely on spawning step
		/// </summary>
		/// <param name="recordDependency"> Each task this one depends on will be reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		/// <summary> Particle initialization step </summary>
		inline ParticleInitializationStepKernel::Task* InitializationStep()const { return m_initializationStep; }

	private:
		// Initialization step
		const Reference<ParticleInitializationStepKernel::Task> m_initializationStep;

		// Lock for m_buffers
		SpinLock m_bufferLock;

		// Target buffers
		Reference<ParticleBuffers> m_buffers;

		// Time scale
		std::atomic<float> m_timeScale = 1.0f;

		// Time mode
		std::atomic<uint32_t> m_timeMode = static_cast<uint32_t>(TimeMode::SCALED_DELTA_TIME);

		// ParticleBuffers from the last Synchronize() call
		Reference<ParticleBuffers> m_lastBuffers;

		// ParticleState buffer binding from the last Synchronize() call
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_particleStateBuffer;

		// Time step tasks
		Stacktor<Reference<ParticleTimestepTask>> m_timestepTasks;

	private:
		// Some private classes reside in here..
		struct Helpers;
	};
}
