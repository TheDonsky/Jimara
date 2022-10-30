#pragma once
#include "../Object.h"
#include "../Function.h"
#include "../Helpers.h"
#include "../Collections/ObjectSet.h"
#include "../Collections/ThreadBlock.h"
#include "../../OS/Logging/Logger.h"
#include <unordered_set>


namespace Jimara {
	/// <summary>
	/// Job system for executing interdependent tasks asynchronously
	/// </summary>
	class JIMARA_API JobSystem : public virtual Object {
	public:
		/// <summary>
		/// Job(task) within a job system
		/// </summary>
		class JIMARA_API Job : public virtual Object {
		protected:
			/// <summary> Invoked by job system to execute the task at hand (called, once all dependencies are resolved) </summary>
			virtual void Execute() = 0;

			/// <summary>
			/// Should report the jobs, this particular task depends on (invoked by job system to build dependency graph)
			/// </summary>
			/// <param name="addDependency"> Calling this will record dependency for given job (individual dependencies do not have to be added to the system to be invoked) </param>
			virtual void CollectDependencies(Callback<Job*> addDependency) = 0;

			/// <summary> Job system is allowed to invoke Execute and CollectDependencies callbacks </summary>
			friend class JobSystem;
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="maxThreads"> Maximal number of threads, the job system is allowed to utilize </param>
		/// <param name="threadThreshold"> Minial number of jobs to 'justify' an additional execution thread </param>
		JobSystem(size_t maxThreads = std::thread::hardware_concurrency(), size_t threadThreshold = 1);

		/// <summary> Virtual destructor </summary>
		virtual ~JobSystem();


		/// <summary>
		/// Executes entire job system
		/// </summary>
		/// <param name="log"> Logger for JobSystem error reporting (this logger is not accessible by jobs themselves, it's just for the system itself) </param>
		/// <param name="onIterationComplete"> Invoked after each iteration of non-interdependent task block execution </param>
		/// <returns> True, if all jobs within the system have been executed, false otherwise </returns>
		bool Execute(OS::Logger* log = nullptr, const Callback<>& onIterationComplete = Callback<>(Unused<>));

		/// <summary>
		/// Adds a job to the system
		/// </summary>
		/// <param name="job"> Job to add </param>
		void Add(Job* job);

		/// <summary>
		/// Removes job from the systeme
		/// </summary>
		/// <param name="job"> Job to remove </param>
		void Remove(Job* job);

		/// <summary>
		/// Set of jobs from a JobSystem
		/// </summary>
		class JobSet {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~JobSet() {}

			/// <summary>
			/// Adds a job to the system
			/// </summary>
			/// <param name="job"> Job to add </param>
			virtual void Add(Job* job) = 0;

			/// <summary>
			/// Removes job from the systeme
			/// </summary>
			/// <param name="job"> Job to remove </param>
			virtual void Remove(Job* job) = 0;
		};

		/// <summary> Set of jobs from the JobSystem (for access to the system without the ability to execute the system or hold any references to it) </summary>
		JobSet& Jobs();

	private:
		// Job collection
		struct InternalJobSet : public virtual JobSet {
			// Add call
			virtual void Add(Job* job) final override;

			// Remove call
			virtual void Remove(Job* job) final override;

			// Lock for AddJob/RemoveJob
			std::mutex m_dataLock;

			// Job collection
			ObjectSet<Job> m_jobs;
		} m_jobs;

		// Lock for execution
		std::mutex m_executionLock;

		// Maximal number of threads, the job system is allowed to utilize
		std::atomic<size_t> m_maxThreads;

		// Minial number of jobs to 'justify' an additional execution thread
		std::atomic<size_t> m_threadThreshold;

		// Execution thread block
		ThreadBlock m_threadBlock;

		// Job description (execution time)
		struct JobWithDependencies {
			Reference<Job> job;
			mutable size_t dependencies;

			JobWithDependencies(Job* j = nullptr);
		};

		// Execution job buffer:
		ObjectSet<Job, JobWithDependencies> m_jobBuffer;
		std::vector<std::vector<Job*>> m_dependants;

		// Execution dependency buffer:
		std::unordered_set<Reference<Job>> m_dependencyBuffer;

		// Executable job back and front buffers:
		std::vector<Job*> m_executableJobs[2];
	};
}
