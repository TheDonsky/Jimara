#pragma once
#include "../Object.h"
#include "../Collections/ObjectSet.h"
#include "../Collections/ThreadBlock.h"
#include "../../OS/Logging/Logger.h"
#include <unordered_set>


namespace Jimara {
	/// <summary>
	/// Job system for executing interdependent tasks asynchronously
	/// </summary>
	class JobSystem : public virtual Object {
	public:
		/// <summary>
		/// Job(task) within a job system
		/// </summary>
		class Job : public virtual Object {
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
		/// Executes entire job system
		/// </summary>
		/// <param name="log"> Logger for JobSystem error reporting (this logger is not accessible by jobs themselves, it's just for the system itself) </param>
		/// <returns> True, if all jobs within the system have been executed, false otherwise </returns>
		bool Execute(OS::Logger* log = nullptr);


	private:
		// Lock for AddJob/RemoveJob
		std::mutex m_dataLock;
		
		// Job collection
		ObjectSet<Job> m_jobs;

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
			mutable std::atomic<size_t> dependencies;
			mutable std::vector<Job*> dependants;

			JobWithDependencies(Job* j = nullptr);
			JobWithDependencies(JobWithDependencies&& other) noexcept;
			JobWithDependencies& operator=(JobWithDependencies&& other) noexcept;
			JobWithDependencies(const JobWithDependencies& other) noexcept;
			JobWithDependencies& operator=(const JobWithDependencies& other) noexcept;
		};

		// Execution job buffer:
		ObjectSet<Job, JobWithDependencies> m_jobBuffer;

		// Execution dependency buffer:
		std::unordered_set<Reference<Job>> m_dependencyBuffer;

		// Executable job back and front buffers:
		std::vector<Job*> m_executableJobs[2];
	};
}
