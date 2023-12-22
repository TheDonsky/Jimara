#pragma once
#include "../Scene/Scene.h"

namespace Jimara {
	/// <summary>
	/// Graphics simulation system is responsible for executing simulation kernel tasks as a part of a regular scene update cycle;
	/// All the user has to do is to add tasks when they're needed and remove them when they are no longer required.
	/// <para/> General concepts and usage is as follows: 
	/// <para/> 0. Components/Users create a bunch of GraphicsSimulation::Task objects and add them to the GraphicsSimulation during a regular update cycle;
	/// <para/> 1. Each GraphicsSimulation::Task may have arbitrary dependencies, that have to be executed before them;
	/// <para/> 2. GraphicsSimulation::Task objects also have raw data buffers, associated with the GraphicsSimulation::Kernel they 'belong' to;
	/// <para/> 3. On each graphics synch point GraphicsSimulation collects all GraphicsSimulation::Task-s and request to synchronize their settings buffers with the scene logic;
	/// <para/> 4. During the synch point the simulation also builds up the dependency graph and schedules several simulation steps for the tasks that can be executed at the same time;
	/// <para/> 5. As a part of the render jobs, each simulation step consists of a bunch of GraphicsSimulation::KernelInstance objects through which it requests the tasks to be executed;
	/// <para/> 6. GraphicsSimulation::KernelInstance-es are expected to run a single kernel for all tasks passed as arguments to Execute() function (One is expected to use something like CombinedGraphicsSimulationKernel template);
	/// <para/> 7. For clarity, the task buffers are expected to contain all work-related data, like bindless id-s of state buffers and it's fully up to them to keep those binding alive in-between Synchronize() calls.
	/// </summary>
	class JIMARA_API GraphicsSimulation {
	public:
		/// <summary> Graphics simulation kernel </summary>
		class Kernel;

		/// <summary> Graphics simulation task </summary>
		class Task;

		/// <summary> Instance of a GraphicsSimulation::Kernel </summary>
		class KernelInstance;

		/// <summary> Object that gives access to the simulation step jobs of the system </summary>
		class JobDependencies;

		/// <summary>
		/// Adds task to the scene-wide simulation
		/// </summary>
		/// <param name="task"> Task to add to the simulation </param>
		static void AddTask(Task* task);

		/// <summary>
		/// Removes task from the scene-wide simulation
		/// </summary>
		/// <param name="task"> Task to remove from the simulation </param>
		static void RemoveTask(Task* task);

		/// <summary> Smart pointer to a task that also adds and removes assigned tasks from the scene-wide simulation </summary>
		typedef Reference<Task, GraphicsSimulation> TaskBinding;

	private:
		// This is a static class; Constructor and the destructor are private!
		inline GraphicsSimulation() {}
		inline virtual ~GraphicsSimulation() {}

		// Actual system implementation is hidden behind this
		struct Helpers;

		// Add/Remove reference functions for the TaskBinding smart pointer type
		static void AddReference(Task* task);
		static void ReleaseReference(Task* task);

		// TaskBinding has to access AddReference and ReleaseReference...
		friend class Reference<Task, GraphicsSimulation>;
	};





	/// <summary>
	/// Instance of a GraphicsSimulation::Kernel
	/// </summary>
	class JIMARA_API GraphicsSimulation::KernelInstance : public virtual Object {
	public:
		/// <summary>
		/// Invoked by GraphicsSimulation from the render job system;
		/// <para/> As parameters, this one is receiving a list of all tasks that can run at the same time from the same GraphicsSimulation::Kernel that created the instance.
		/// </summary>
		/// <param name="commandBufferInfo"> Command buffer and in-flight buffer index </param>
		/// <param name="tasks"> List of tasks to execute </param>
		/// <param name="taskCount"> Number of tasks in the task list </param>
		virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) = 0;
	};





	/// <summary> Graphics simulation kernel </summary>
	class JIMARA_API GraphicsSimulation::Kernel : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="taskSettingsSize"> Size of the settings buffer for the tasks </param>
		inline Kernel(size_t taskSettingsSize) : m_taskSettingsSize(taskSettingsSize) {}

		/// <summary> Size of the settings buffer for the tasks </summary>
		inline size_t TaskSettingsSize()const { return m_taskSettingsSize; }

		/// <summary>
		/// Should create a kernel instance that can execute groups of tasks.
		/// <para/> Notes:
		/// <para/> 0. Invoked by GraphicsSimulation if there are some tasks relying on the kernel;
		/// <para/> 1. Depending on the dependency graph, the simulation may request creating an instance more than once and it's expected for the subsequent requests to create new instances.
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> New instance of the Kernel </returns>
		inline virtual Reference<KernelInstance> CreateInstance(SceneContext* context)const = 0;


	private:
		// Size of the settings buffer for the tasks
		const size_t m_taskSettingsSize;
	};





	/// <summary>
	/// Graphics simulation task
	/// </summary>
	class JIMARA_API GraphicsSimulation::Task : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="kernel"> Instance of a simulation kernel, this task belongs to (Expected to be a singleton instance of the kernel) </param>
		/// <param name="context"> Scene context </param>
		inline Task(const Kernel* kernel, SceneContext* context)
			: m_kernel(kernel), m_context(context) {
			assert(kernel != nullptr);
			assert(context != nullptr);
			m_settingsBuffer.Resize(m_kernel->TaskSettingsSize());
		}

		/// <summary> Instance of a simulation kernel, this task belongs to </summary>
		inline const GraphicsSimulation::Kernel* Kernel()const { return m_kernel; }

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
		inline const Type& GetSettings()const {
			assert(m_settingsBuffer.Size() >= sizeof(Type));
			return *reinterpret_cast<const Type*>(m_settingsBuffer.Data());
		}

		/// <summary> Settings memory block </summary>
		virtual MemoryBlock Settings()const { return MemoryBlock(m_settingsBuffer.Data(), m_settingsBuffer.Size(), nullptr); }

		/// <summary> 
		/// Invoked by GraphicsSimulation during the graphics synch point; 
		/// This is the method you override when you want to keep track of some data from the update cycle 
		/// </summary>
		inline virtual void Synchronize() {}

		/// <summary>
		/// Invoked by GraphicsSimulation during the graphics synch point;
		/// If a task has dependencies that have to be executed before it, this is the place to report them
		/// </summary>
		/// <param name="recordDependency"> Each task this one depends on should be reported through this callback </param>
		inline virtual void GetDependencies(const Callback<Task*>& recordDependency)const { Unused(recordDependency); }


	private:
		// Kernel instance
		const Reference<const GraphicsSimulation::Kernel> m_kernel;

		// Scene context
		const Reference<SceneContext> m_context;

		// Settings buffer
		Stacktor<uint32_t, 128> m_settingsBuffer;
	};





	/// <summary> Object that gives access to the simulation step jobs of the system </summary>
	class JIMARA_API GraphicsSimulation::JobDependencies : public virtual Object {
	public:
		/// <summary> Virtual destructor </summary>
		virtual ~JobDependencies();

		/// <summary>
		/// Gets shared instance of a scene context
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Shared instance </returns>
		static Reference<JobDependencies> For(SceneContext* context);

		/// <summary>
		/// Reports dependencies
		/// </summary>
		/// <param name="report"> Report callback </param>
		void CollectDependencies(const Callback<JobSystem::Job*>& report);

	private:
		// Weak pointer to the data
		const Reference<Object> m_dataPtr;

		// Constructor is private!
		JobDependencies(Object* dataPtr);
	};
}
