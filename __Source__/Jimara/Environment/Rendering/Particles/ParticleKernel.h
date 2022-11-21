#pragma once
#include "ParticleBuffers.h"


namespace Jimara {
	/// <summary>
	/// Particles are simulated with a graph consisting of interdependent particle kernels.
	/// <para/> General concepts and usage is as follows: 
	/// <para/> 0. Particle systems create a bunch of ParticleKernel::Task objects and add them to the ParticleSimulation;
	/// <para/> 1. Each ParticleKernel::Task may have arbitrary dependencies, that have to be executed before them;
	/// <para/> 2. ParticleKernel::Task objects also have raw data buffers, associated with the ParticleKernel they 'belong' to;
	/// <para/> 3. On each graphics synch point ParticleSimulation collects all ParticleKernel::Task-s and request to synchronize their settings buffers with the scene logic;
	/// <para/> 4. During the synch point the simulation also builds up the dependency graph and schedules several simulation steps for the tasks that can be executed at the same time;
	/// <para/> 5. As a part of the render jobs, each simulation step consists of a bunch of ParticleKernel::Instance objects through which it requests the tasks to be executed;
	/// <para/> 6. ParticleKernel::Instance-es are expected to run a single kernel for all tasks passed as arguments to Execute() function (One is expected to use something like CombinedParticleKernel template);
	/// <para/> 7. For clarity, the task buffers are expected to contain particle system related data, like bindless id-s of state buffers and it's fully up to them to keep those binding alive in-between Synchronize() calls.
	/// </summary>
	class JIMARA_API ParticleKernel : public virtual Object {
	public:
		/// <summary>
		/// Particle simulation task
		/// </summary>
		class JIMARA_API Task : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="kernel"> Instance of a particle kernel, this task belongs to (Expected to be a singleton instance of the kernel) </param>
			/// <param name="context"> Scene context </param>
			inline Task(const ParticleKernel* kernel, SceneContext* context)
				: m_kernel(kernel), m_context(context) {
				assert(kernel != nullptr);
				assert(context != nullptr);
				m_settingsBuffer.Resize(m_kernel->TaskSettingsSize());
			}

			/// <summary> Instance of a particle kernel, this task belongs to </summary>
			inline const ParticleKernel* Kernel()const { return m_kernel; }

			/// <summary> Scene context </summary>
			inline SceneContext* Context()const { return m_context; }

			/// <summary>
			/// Sets settings
			/// </summary>
			/// <typeparam name="Type"> Settings type (has to be a simple buffer with no external allocations and size has to match Kernel()->TaskSettingsSize()) </typeparam>
			/// <param name="settings"> Settings </param>
			template<typename Type>
			inline void SetSettings(const Type& settings) {
				std::memcpy(
					reinterpret_cast<void*>(m_settingsBuffer.Data()), 
					reinterpret_cast<const void*>(&settings), 
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
			}

			/// <summary>
			/// Reinterpret-casts settings buffer to given type
			/// </summary>
			/// <typeparam name="Type"> Settings type (has to be a simple buffer with no external allocations and size has to match Kernel()->TaskSettingsSize()) </typeparam>
			/// <returns> Settings </returns>
			template<typename Type>
			inline Type GetSettings()const {
				Type settings = {};
				std::memcpy(
					reinterpret_cast<void*>(&settings),
					reinterpret_cast<const void*>(m_settingsBuffer.Data()),
					Math::Min(sizeof(settings), m_settingsBuffer.Size()));
				return settings;
			}

			/// <summary> Settings memory block </summary>
			virtual MemoryBlock Settings()const { return MemoryBlock(m_settingsBuffer.Data(), m_settingsBuffer.Size(), nullptr); }

			/// <summary> 
			/// Invoked by ParticleSimulation during the graphics synch point; 
			/// This is the method you override when you want to keep track of some data from the update cycle 
			/// </summary>
			inline virtual void Synchronize() {}

			/// <summary>
			/// Invoked by ParticleSimulation during the graphics synch point;
			/// If a task has dependencies that have to be executed before it, this is the place to report them
			/// </summary>
			/// <param name="recordDependency"> Each task this one depends on should be reported through this callback </param>
			inline virtual void GetDependencies(const Callback<Task*>& recordDependency)const { Unused(recordDependency); }


		private:
			// Kernel instance
			const Reference<const ParticleKernel> m_kernel;

			// Scene context
			const Reference<SceneContext> m_context;

			// Settings buffer
			Stacktor<uint32_t, 128> m_settingsBuffer;
		};


		/// <summary>
		/// Instance of a ParticleKernel
		/// </summary>
		class JIMARA_API Instance : public virtual Object {
		public:
			/// <summary>
			/// Invoked by ParticleSimulation from the render job system;
			/// <para/> As parameters, this one is receiving a list of all tasks that can run at the same time from the same ParticleKernel that created the instance.
			/// </summary>
			/// <param name="commandBufferInfo"> Command buffer and in-flight buffer index </param>
			/// <param name="tasks"> List of tasks to execute </param>
			/// <param name="taskCount"> Number of tasks in the task list </param>
			virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) = 0;
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="taskSettingsSize"> Size of the settings buffer for the tasks </param>
		inline ParticleKernel(size_t taskSettingsSize) : m_taskSettingsSize(taskSettingsSize) {}

		/// <summary> Size of the settings buffer for the tasks </summary>
		inline size_t TaskSettingsSize()const { return m_taskSettingsSize; }

		/// <summary>
		/// Should create a kernel instance that can execute groups of tasks.
		/// <para/> Notes:
		/// <para/> 0. Invoked by ParticleSimulation if there are some tasks relying on the kernel;
		/// <para/> 1. Depending on the dependency graph, the simulation may request creating an instance more than once and it's expected for the subsequent requests to create new instances.
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> New instance of the ParticleKernel </returns>
		inline virtual Reference<Instance> CreateInstance(SceneContext* context)const = 0;


	private:
		// Size of the settings buffer for the tasks
		const size_t m_taskSettingsSize;
	};
}
