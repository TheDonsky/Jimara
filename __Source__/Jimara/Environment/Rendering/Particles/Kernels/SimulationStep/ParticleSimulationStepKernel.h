#pragma once
#include "../InitializationStep/ParticleInitializationStepKernel.h"


namespace Jimara {
	/// <summary>
	/// After the primary simulation kernels are done with the particle state, ParticleSimulationStepKernel gets executed.
	/// It is responsible for decrementing lifetime and moving the particles around using velocity and angular velocity.
	/// </summary>
	class JIMARA_API ParticleSimulationStepKernel : public virtual GraphicsSimulation::Kernel {
	public:
		/// <summary>
		/// Simulation time 'mode'
		/// </summary>
		enum class JIMARA_API TimeMode : uint32_t {
			/// <summary> Time does not 'flow'; delta time is always 0 </summary>
			NO_TIME = 0u,

			/// <summary> Timestep is unscaled delta time </summary>
			UNSCALED_DELTA_TIME = 1u,

			/// <summary> Timestep is scaled delta time </summary>
			SCALED_DELTA_TIME = 2u,

			/// <summary> Timestep is tied to physics simulation (not adviced) </summary>
			PHYSICS_DELTA_TIME = 3u
		};

		/// <summary>
		/// Task of ParticleSimulationStepKernel
		/// <para/> Note: Used internally by Particle Systems
		/// </summary>
		class JIMARA_API Task : public virtual GraphicsSimulation::Task {
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

			/// <summary> Updates settings buffer </summary>
			virtual void Synchronize()override;

			/// <summary>
			/// Invoked by ParticleSimulation during the graphics synch point;
			/// <para/> Reports simulation tasks as dependencies, whcih themselves rely on spawning step
			/// </summary>
			/// <param name="recordDependency"> Each task this one depends on will be reported through this callback </param>
			virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

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
		};
		
		/// <summary> Virtual destructor </summary>
		virtual ~ParticleSimulationStepKernel();

		/// <summary>
		/// Creates an instance of a simulation step kernel
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Kernel instance </returns>
		virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override;

	private:
		// No custom instances can be created
		ParticleSimulationStepKernel();

		// Some private classes reside in here..
		struct Helpers;
	};
}
