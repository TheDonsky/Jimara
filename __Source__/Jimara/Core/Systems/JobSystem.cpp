#include "JobSystem.h"


namespace Jimara {
	JobSystem::JobSystem(size_t maxThreads, size_t threadThreshold) 
		: m_maxThreads(maxThreads), m_threadThreshold(threadThreshold) {}

	JobSystem::~JobSystem() {}

	void JobSystem::Add(Job* job) {
		m_jobs.Add(job);
	}

	void JobSystem::Remove(Job* job) {
		m_jobs.Remove(job);
	}

	void JobSystem::InternalJobSet::Add(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Add(job);
	}

	void JobSystem::InternalJobSet::Remove(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Remove(job);
	}

	JobSystem::JobSet& JobSystem::Jobs() { return m_jobs; }



	namespace {
		struct DependencyRecorder {
			std::unordered_set<Reference<JobSystem::Job>>* dependencies;

			void Record(JobSystem::Job* job)const { if (job != nullptr) dependencies->insert(job); }
		};
	}


	bool JobSystem::Execute(OS::Logger* log, const Callback<>& onIterationComplete) {
		std::unique_lock<std::mutex> executionLock(m_executionLock);

		// Transfer system contents to jobs:
		{
			std::unique_lock<std::mutex> lock(m_jobs.m_dataLock);
			m_jobBuffer.Add(m_jobs.m_jobs.Data(), m_jobs.m_jobs.Size());
		}

		// Clear dependants:
		{
			if (m_dependants.size() < m_jobBuffer.Size())
				m_dependants.resize(m_jobBuffer.Size());
			for (size_t i = 0; i < m_jobBuffer.Size(); i++)
				m_dependants[i].clear();
		}

		// Extract all dependencies:
		for (size_t jobId = 0; jobId < m_jobBuffer.Size(); jobId++) {
			const DependencyRecorder recorder = { &m_dependencyBuffer };
			const Callback<Job*> recordDependency(&DependencyRecorder::Record, recorder);
			Job* job;
			{
				const JobWithDependencies& jobData = m_jobBuffer[jobId];
				job = jobData.job;
				job->CollectDependencies(recordDependency);
				jobData.dependencies = m_dependencyBuffer.size();
			}
			for (std::unordered_set<Reference<Job>>::const_iterator it = m_dependencyBuffer.begin(); it != m_dependencyBuffer.end(); ++it) {
				Job* dependency = *it;
				const JobWithDependencies* dep = m_jobBuffer.Find(dependency);
				if (dep != nullptr) m_dependants[dep - m_jobBuffer.Data()].push_back(jobId);
				else m_jobBuffer.Add(&dependency, 1, [&](const JobWithDependencies* added, size_t) {
					size_t index = (added - m_jobBuffer.Data());
					while (m_dependants.size() <= index) 
						m_dependants.push_back({});
					std::vector<size_t>& dependants = m_dependants[index];
					dependants.clear();
					dependants.push_back(jobId);
					});
			}
			m_dependencyBuffer.clear();
		}

		// Find jobs that are ready to execute:
		ExecutableJobs* executableJobsBack = m_executableJobs;
		ExecutableJobs* executableJobsFront = m_executableJobs + 1;
		for (size_t i = 0; i < m_jobBuffer.Size(); i++) {
			const JobWithDependencies& job = m_jobBuffer[i];
			if (job.dependencies <= 0) executableJobsBack->jobs.push_back(job.job);
		}

		// Execute jobs iteratively:
		size_t unexecutedJobsLeft = m_jobBuffer.Size();
		while (unexecutedJobsLeft > 0) {
			if (executableJobsBack->jobs.size() <= 0) {
				if (log != nullptr) log->Error("JobSystem::Execute - Job graph has circular dependencies!");
				break;
			}

			// Execute jobs asynchronously:
			const size_t threadThreshold = std::max(m_threadThreshold.load(), (size_t)1u);
			const size_t numThreads = std::min((executableJobsBack->jobs.size() + threadThreshold - 1) / threadThreshold, m_maxThreads.load());
			executableJobsBack->executionIndex = 0u;
			auto executeJob = [](ThreadBlock::ThreadInfo info, void* jobsPtr) {
				ExecutableJobs& executables = *((ExecutableJobs*)jobsPtr);
				while (true) {
					size_t index = executables.executionIndex.fetch_add(1u);
					if (index >= executables.jobs.size()) break;
					executables.jobs[index]->Execute();
				}
			};
			if (numThreads > 1)
				m_threadBlock.Execute(numThreads, executableJobsBack, Callback<ThreadBlock::ThreadInfo, void*>(executeJob));
			else {
				ThreadBlock::ThreadInfo info;
				info.threadCount = 1;
				info.threadId = 0;
				executeJob(info, executableJobsBack);
			}
			onIterationComplete();
			
			// Find new executable jobs:
			executableJobsFront->jobs.clear();
			executableJobsFront->executionIndex = 0u;
			for (size_t i = 0; i < executableJobsBack->jobs.size(); i++) {
				const JobWithDependencies* job = m_jobBuffer.Find(executableJobsBack->jobs[i]);
				const std::vector<size_t>& dependants = m_dependants[job - m_jobBuffer.Data()];
				for (size_t depId = 0; depId < dependants.size(); depId++) {
					const JobWithDependencies& dep = m_jobBuffer[dependants[depId]];
					dep.dependencies--;
					if (dep.dependencies <= 0)
						executableJobsFront->jobs.push_back(dep.job);
				}
			}

			// Remove completed jobs and swap buffers:
			unexecutedJobsLeft -= std::min(executableJobsBack->jobs.size(), unexecutedJobsLeft);
			std::swap(executableJobsBack, executableJobsFront);
		}

		// Clear runtime collections:
		executableJobsBack->jobs.clear();
		executableJobsBack->executionIndex = 0u;
		executableJobsFront->jobs.clear();
		executableJobsFront->executionIndex = 0u;
		m_jobBuffer.Clear();
		return (unexecutedJobsLeft <= 0);
	}



	JobSystem::JobWithDependencies::JobWithDependencies(Job* j) : job(j), dependencies(0) {}
}
