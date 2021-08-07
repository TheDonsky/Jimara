#include "JobSystem.h"


namespace Jimara {
	JobSystem::JobSystem(size_t maxThreads, size_t threadThreshold) 
		: m_maxThreads(maxThreads), m_threadThreshold(threadThreshold) {}

	JobSystem::~JobSystem() {}

	void JobSystem::Add(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Add(job);
	}

	void JobSystem::Remove(Job* job) {
		std::unique_lock<std::mutex> lock(m_dataLock);
		m_jobs.Remove(job);
	}



	namespace {
		struct DependencyRecorder {
			std::unordered_set<Reference<JobSystem::Job>>* dependencies;

			void Record(JobSystem::Job* job)const { if (job != nullptr) dependencies->insert(job); }
		};
	}


	bool JobSystem::Execute(OS::Logger* log) {
		std::unique_lock<std::mutex> executionLock(m_executionLock);

		// Transfer system contents to jobs:
		{
			std::unique_lock<std::mutex> lock(m_dataLock);
			m_jobBuffer.Add(m_jobs.Data(), m_jobs.Size());
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
				if (dep != nullptr) dep->dependants.push_back(job);
				else m_jobBuffer.Add(&dependency, 1, [&](const JobWithDependencies* added, size_t) { added->dependants.push_back(job); });
			}
			m_dependencyBuffer.clear();
		}

		// Find jobs that are ready to execute:
		std::vector<Job*>* executableJobsBack = m_executableJobs;
		std::vector<Job*>* executableJobsFront = m_executableJobs + 1;
		for (size_t i = 0; i < m_jobBuffer.Size(); i++) {
			const JobWithDependencies& job = m_jobBuffer[i];
			if (job.dependencies <= 0) executableJobsBack->push_back(job.job);
		}

		// Execute jobs iteratively:
		while (m_jobBuffer.Size() > 0) {
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
				const JobWithDependencies& job = *m_jobBuffer.Find(executableJobsBack->operator[](i));
				for (size_t depId = 0; depId < job.dependants.size(); depId++) {
					const JobWithDependencies* dep = m_jobBuffer.Find(job.dependants[depId]);
					dep->dependencies--;
					if (dep->dependencies <= 0)
						executableJobsFront->push_back(dep->job);
				}
			}

			// Remove completed jobs and swap buffers:
			m_jobBuffer.Remove(executableJobsBack->data(), executableJobsBack->size());
			std::swap(executableJobsBack, executableJobsFront);
		}

		// Clear runtime collections:
		executableJobsBack->clear();
		executableJobsFront->clear();
		if (m_jobBuffer.Size() <= 0) return true;
		else {
			m_jobBuffer.Clear();
			return false;
		}
	}



	JobSystem::JobWithDependencies::JobWithDependencies(Job* j) : job(j), dependencies(0) {}

	JobSystem::JobWithDependencies::JobWithDependencies(JobWithDependencies&& other) noexcept
		: job(other.job), dependencies(other.dependencies.load()), dependants(std::move(other.dependants)) { }

	JobSystem::JobWithDependencies& JobSystem::JobWithDependencies::operator=(JobWithDependencies&& other) noexcept {
		job = other.job;
		dependencies = other.dependencies.load();
		dependants = std::move(other.dependants);
		return (*this);
	}

	JobSystem::JobWithDependencies::JobWithDependencies(const JobWithDependencies& other) noexcept
		: job(other.job), dependencies(other.dependencies.load()), dependants(other.dependants) { }

	JobSystem::JobWithDependencies& JobSystem::JobWithDependencies::operator=(const JobWithDependencies& other) noexcept {
		job = other.job;
		dependencies = other.dependencies.load();
		dependants = other.dependants;
		return (*this);
	}
}
