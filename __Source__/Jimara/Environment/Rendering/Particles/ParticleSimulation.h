#pragma once
#include "../../Scene/Scene.h"
#include "ParticleKernel.h"

namespace Jimara {
	/// <summary>
	/// Particle kernel tasks can be executed automatically by a particle simulation system.
	/// All the user has to do is to add tasks when they're needed and remove them when they are no longer required.
	/// </summary>
	class JIMARA_API ParticleSimulation {
	public:
		/// <summary> Particle simulation task </summary>
		typedef ParticleKernel::Task Task;

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
		typedef Reference<Task, ParticleSimulation> TaskBinding;

	private:
		// This is a static class; Constructor and the destructor are private!
		inline ParticleSimulation() {}
		inline virtual ~ParticleSimulation() {}

		// Actual system implementation is hidden behind this
		struct Helpers;

		// Add/Remove reference functions for the TaskBinding smart pointer type
		static void AddReference(Task* task);
		static void ReleaseReference(Task* task);

		// TaskBinding has to access AddReference and ReleaseReference...
		friend class TaskBinding;
	};
}
