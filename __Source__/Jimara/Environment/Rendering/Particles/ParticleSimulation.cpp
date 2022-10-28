#include "ParticleSimulation.h"


namespace Jimara {

	struct ParticleSimulation::Helpers {
		class TaskSet : public virtual Object {
		public:
			inline size_t AddTask(Task* task) {
				if (task == nullptr) return m_entries.size();
				std::unique_lock<std::shared_mutex> lock(m_lock);
				{
					decltype(m_entries)::iterator it = m_entries.find(task);
					if (it != m_entries.end()) {
						it->second++;
						return ~size_t(0u);
					}
				}
				m_entries[task] = 1u;
				m_taskList.clear();
				return m_entries.size();
			}

			inline size_t RemoveTask(Task* task) {
				if (task == nullptr) return m_entries.size();
				std::unique_lock<std::shared_mutex> lock(m_lock);
				
				decltype(m_entries)::iterator it = m_entries.find(task);
				if (it == m_entries.end()) 
					return m_entries.size();

				if (it->second <= 1) {
					m_entries.erase(it);
					m_taskList.clear();
				}
				else it->second--;

				return m_entries.size();
			}

			template<typename GetTaskFn>
			inline void GetTasks(const GetTaskFn& inspectTask) {
				std::shared_lock<std::shared_mutex> lock(m_lock);
				if (m_entries.size() != m_taskList.size()) {
					m_taskList.clear();
					for (decltype(m_entries)::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it)
						m_taskList.push_back(it->first);
				}
				size_t count = m_taskList.size();
				Task** tasks = m_taskList.data();
				for (size_t i = 0; i < count; i++)
					inspectTask(tasks[i]);
			}

		private:
			std::shared_mutex m_lock;
			std::map<Reference<Task>, size_t> m_entries;
			std::vector<Task*> m_taskList;
		};

		struct TaskWithDependencies {
			Reference<Task> task;
			mutable std::atomic<size_t> dependencies;
			mutable std::vector<Task*> dependants;

			inline TaskWithDependencies(Task* t = nullptr) 
				: task(t), dependencies(0u) { }
			inline TaskWithDependencies(TaskWithDependencies&& other) noexcept 
				: task(other.task), dependencies(other.dependencies.load()), dependants(std::move(other.dependants)) { }
			inline TaskWithDependencies& operator=(TaskWithDependencies&& other) noexcept {
				task = other.task;
				dependencies = other.dependencies.load();
				dependants = std::move(other.dependants);
				return (*this);
			}
			inline TaskWithDependencies(const TaskWithDependencies& other) noexcept 
				: task(other.task), dependencies(other.dependencies.load()), dependants(other.dependants) { }
			inline TaskWithDependencies& operator=(const TaskWithDependencies& other) noexcept {
				task = other.task;
				dependencies = other.dependencies.load();
				dependants = other.dependants;
				return (*this);
			}
		};

		class TaskCollectionJob : public virtual JobSystem::Job {
		public:
			inline TaskCollectionJob(TaskSet* taskSet) : m_taskSet(taskSet) {}

			inline virtual ~TaskCollectionJob() {}

			inline ObjectSet<Task, TaskWithDependencies>& SchedulingBuffer() { return m_taskBuffer; }

			inline const std::vector<Reference<Task>>& AllTasks()const { return m_allTasks; }

		protected:
			inline virtual void Execute() override {
				m_taskBuffer.Clear();
				m_allTasks.clear();

				// Collect all base jobs:
				m_taskSet->GetTasks([&](Task* task) { m_taskBuffer.Add(task); });

				// Add all dependencies to task buffer:
				for (size_t i = 0; i < m_taskBuffer.Size(); i++) {
					Task* task;
					{
						const TaskWithDependencies& taskData = m_taskBuffer[i];
						task = taskData.task;
						task->GetDependencies([&](Task* t) { m_dependencyBuffer.insert(t); });
						taskData.dependencies = m_dependencyBuffer.size();
					}
					for (std::unordered_set<Reference<Task>>::const_iterator it = m_dependencyBuffer.begin(); it != m_dependencyBuffer.end(); ++it) {
						Task* dependency = *it;
						const TaskWithDependencies* dep = m_taskBuffer.Find(dependency);
						if (dep != nullptr) dep->dependants.push_back(task);
						else m_taskBuffer.Add(&dependency, 1, [&](const TaskWithDependencies* added, size_t) { added->dependants.push_back(task); });
					}
					m_allTasks.push_back(task);
					m_dependencyBuffer.clear();
				}
			}

			inline virtual void CollectDependencies(Callback<Job*>) override {}

		private:
			const Reference<TaskSet> m_taskSet;

			std::vector<Reference<Task>> m_allTasks;
			ObjectSet<Task, TaskWithDependencies> m_taskBuffer;
			std::unordered_set<Reference<Task>> m_dependencyBuffer;
		};

		class RenderSchedulingJob : public virtual JobSystem::Job {
		public:
			inline RenderSchedulingJob(TaskCollectionJob* collectionJob) : m_collectionJob(collectionJob) {}

		protected:
			inline virtual void Execute() override {
				// __TODO__: Implement this crap!
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				addDependency(m_collectionJob);
			}

		private:
			const Reference<TaskCollectionJob> m_collectionJob;
		};

		class SynchJob : public virtual JobSystem::Job {
		public:
			inline SynchJob(TaskCollectionJob* collectionJob, size_t synchJobIndex, size_t synchJobCount) 
				: m_collectionJob(collectionJob), m_index(synchJobIndex), m_synchJobCount(synchJobCount) {}

			inline virtual ~SynchJob() {}

		protected:
			inline virtual void Execute() override {
				const std::vector<Reference<Task>>& tasks = m_collectionJob->AllTasks();
				const size_t tasksPerJob = (tasks.size() + m_synchJobCount - 1) / m_synchJobCount;
				
				const size_t firstTaskIndex = m_index * tasksPerJob;
				if (firstTaskIndex >= tasks.size()) return;

				const Reference<Task>* ptr = tasks.data() + firstTaskIndex;
				const Reference<Task>* const end = tasks.data() + Math::Min(firstTaskIndex + tasksPerJob, tasks.size());
				while (ptr < end) {
					(*ptr)->Synchronize();
					ptr++;
				}
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				addDependency(m_collectionJob);
			}

		private:
			const Reference<TaskCollectionJob> m_collectionJob;
			const size_t m_index;
			const size_t m_synchJobCount;
		};

		class Simulation : public virtual ObjectCache<Reference<SceneContext>>::StoredObject {
		public:
			inline Simulation(SceneContext* context) : m_context(context) {
				assert(m_context != nullptr);
			}

			inline virtual ~Simulation() {
				RemoveTask(nullptr);
			}

			inline void AddTask(Task* task) {
				std::unique_lock<std::mutex> lock(m_taskLock);
				size_t taskCount = m_taskSet->AddTask(task);
				if (taskCount != 1u) return;
				m_context->StoreDataObject(this);

				if (m_taskCollectionJob == nullptr) {
					m_taskCollectionJob = Object::Instantiate<TaskCollectionJob>(m_taskSet);
					RemoveSchedulingJob();
					RemoveSynchJobs();
				}

				while (m_synchJobs.size() < m_synchJobCount) {
					const Reference<SynchJob> job = Object::Instantiate<SynchJob>(m_taskCollectionJob, m_synchJobs.size(), m_synchJobCount);
					m_context->Graphics()->SynchPointJobs().Add(job);
					m_synchJobs.push_back(job);
				}

				if (m_schedulingJob == nullptr) {
					m_schedulingJob = Object::Instantiate<RenderSchedulingJob>(m_taskCollectionJob);
					m_context->Graphics()->SynchPointJobs().Add(m_schedulingJob);
				}
			}

			inline void RemoveTask(Task* task) {
				std::unique_lock<std::mutex> lock(m_taskLock);
				size_t taskCount = m_taskSet->AddTask(task);
				if (taskCount != 0u) return;

				RemoveSchedulingJob();
				RemoveSynchJobs();
				if (m_taskCollectionJob != nullptr) {
					m_context->Graphics()->SynchPointJobs().Remove(m_taskCollectionJob);
					m_taskCollectionJob = nullptr;
				}

				m_context->EraseDataObject(this);
			}

		private:
			const Reference<SceneContext> m_context;
			const Reference<TaskSet> m_taskSet = Object::Instantiate<TaskSet>();
			const size_t m_synchJobCount = Math::Max(std::thread::hardware_concurrency() - 1u, 1u);
			std::mutex m_taskLock;

			Reference<TaskCollectionJob> m_taskCollectionJob;
			std::vector<Reference<SynchJob>> m_synchJobs;
			Reference<RenderSchedulingJob> m_schedulingJob;

			inline void RemoveSynchJobs() {
				for (size_t i = 0; i < m_synchJobs.size(); i++)
					m_context->Graphics()->SynchPointJobs().Remove(m_synchJobs[i]);
				m_synchJobs.clear();
			}

			inline void RemoveSchedulingJob() {
				if (m_schedulingJob == nullptr) return;
				m_context->Graphics()->SynchPointJobs().Remove(m_schedulingJob);
				m_schedulingJob = nullptr;
			}
		};

		class Cache : public virtual ObjectCache<Reference<SceneContext>> {
		public:
			inline static Reference<Simulation> GetSimulation(SceneContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false, [&]()->Reference<Simulation> { return Object::Instantiate<Simulation>(context); });
			}
		};
	};

	void ParticleSimulation::AddReference(Task* task) {
		if (task == nullptr) return;
		task->AddRef();
		Reference<Helpers::Simulation> simulation = Helpers::Cache::GetSimulation(task->Buffers()->Context());
		if (simulation == nullptr) return;
		simulation->AddTask(task);
	}

	void ParticleSimulation::ReleaseReference(Task* task) {
		if (task == nullptr) return;
		Reference<Helpers::Simulation> simulation = Helpers::Cache::GetSimulation(task->Buffers()->Context());
		if (simulation != nullptr)
			simulation->RemoveTask(task);
		task->ReleaseRef();
	}
}
