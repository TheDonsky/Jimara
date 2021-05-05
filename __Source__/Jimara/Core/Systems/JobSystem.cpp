#include "JobSystem.h"
#include <unordered_set>


namespace Jimara {
	JobSystem::JobSystem(size_t maxThreads, size_t threadThreshold) 
		: m_maxThreads(maxThreads), m_threadThreshold(threadThreshold) {}

	JobSystem::~JobSystem() {}

	void JobSystem::AddJob(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Add(job);
	}

	void JobSystem::RemoveJob(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Remove(job);
	}



	namespace {
		struct JobWithDependencies {
			Reference<JobSystem::Job> job;
			mutable std::atomic<size_t> dependencies;
			mutable std::vector<JobSystem::Job*> dependants;

			inline JobWithDependencies(JobSystem::Job* j = nullptr) : job(j), dependencies(0) {}

			inline JobWithDependencies(JobWithDependencies&& other) noexcept
				: job(other.job), dependencies(other.dependencies.load()), dependants(std::move(other.dependants)) { }

			inline JobWithDependencies& operator=(JobWithDependencies&& other) noexcept {
				job = other.job;
				dependencies = other.dependencies.load();
				dependants = std::move(other.dependants);
				return (*this);
			}

			inline JobWithDependencies(const JobWithDependencies& other) noexcept
				: job(other.job), dependencies(other.dependencies.load()), dependants(other.dependants) { }

			inline JobWithDependencies& operator=(const JobWithDependencies& other) noexcept {
				job = other.job;
				dependencies = other.dependencies.load();
				dependants = other.dependants;
				return (*this);
			}
		};
	}


	bool JobSystem::Execute(OS::Logger* log) {
		std::unique_lock<std::mutex> executionLock(m_executionLock);
		
		// Define active jobs:
		static thread_local ObjectSet<Job, JobWithDependencies> THREAD_JOBS;
		ObjectSet<Job, JobWithDependencies>& jobs = THREAD_JOBS;

		// Transfer system contents to jobs:
		{
			std::unique_lock<std::mutex> lock(m_dataLock);
			jobs.Add(m_jobs.Data(), m_jobs.Size());
		}

		// Extract all dependencies:
		for (size_t jobId = 0; jobId < jobs.Size(); jobId++) {
			static thread_local std::vector<Reference<Job>> dependencies;
			static const Callback<Job*> recordDependency([](Job* job) { dependencies.push_back(job); });
			Job* job;
			{
				const JobWithDependencies& jobData = jobs[jobId];
				job = jobData.job;
				job->CollectDependencies(recordDependency);
				jobData.dependencies = dependencies.size();
			}
			for (size_t depId = 0; depId < dependencies.size(); depId++) {
				Job* dependency = dependencies[depId];
				const JobWithDependencies* dep = jobs.Find(dependency);
				if (dep != nullptr) dep->dependants.push_back(job);
				else jobs.Add(&dependency, 1, [&](const JobWithDependencies* added, size_t) { added->dependants.push_back(job); });
			}
			dependencies.clear();
		}

		// Find jobs that are ready to execute:
		static thread_local std::vector<Job*> EXECUTABLE_JOBS[2];
		std::vector<Job*>* executableJobsBack = EXECUTABLE_JOBS;
		std::vector<Job*>* executableJobsFront = EXECUTABLE_JOBS + 1;
		for (size_t i = 0; i < jobs.Size(); i++) {
			const JobWithDependencies& job = jobs[i];
			if (job.dependencies <= 0) executableJobsBack->push_back(job.job);
		}

		// Execute jobs iteratively:
		while (jobs.Size() > 0) {
			if (executableJobsBack->size() <= 0) {
				if (log != nullptr) log->Error("JobSystem::Execute - Job graph has circular dependencies!");
				break;
			}

			// Execute jobs asynchronously:
			const size_t threadThreshold = std::max(m_threadThreshold.load(), (size_t)1u);
			const size_t numThreads = std::min((executableJobsBack->size() + threadThreshold - 1) / threadThreshold, m_maxThreads.load());
			auto executeJob = [](ThreadBlock::ThreadInfo info, void* jobsPtr) {
				std::vector<Job*>& executables = *((std::vector<Job*>*)jobsPtr);
				for (size_t i = info.threadId; i < executables.size(); i += info.threadCount)
					executables[i]->Execute();
			};
			if (numThreads > 1)
				m_threadBlock.Execute(numThreads, executableJobsBack, Callback<ThreadBlock::ThreadInfo, void*>(executeJob));
			else {
				ThreadBlock::ThreadInfo info;
				info.threadCount = 1;
				info.threadId = 0;
				executeJob(info, executableJobsBack);
			}
			
			// Find new executable jobs:
			executableJobsFront->clear();
			for (size_t i = 0; i < executableJobsBack->size(); i++) {
				const JobWithDependencies& job = *jobs.Find(executableJobsBack->operator[](i));
				for (size_t depId = 0; depId < job.dependants.size(); depId++) {
					const JobWithDependencies* dep = jobs.Find(job.dependants[depId]);
					dep->dependencies--;
					if (dep->dependencies <= 0)
						executableJobsFront->push_back(dep->job);
				}
			}

			// Remove completed jobs and swap buffers:
			jobs.Remove(executableJobsBack->data(), executableJobsBack->size());
			std::swap(executableJobsBack, executableJobsFront);
		}

		// Clear runtime collections:
		executableJobsBack->clear();
		executableJobsFront->clear();
		if (jobs.Size() <= 0) return true;
		else {
			jobs.Clear();
			return false;
		}
	}
}
