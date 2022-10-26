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
				size_t count = m_taskList.Size();
				Task** tasks = m_taskList.data();
				for (size_t i = 0; i < count; i++)
					inspectTask(tasks[i]);
			}

		private:
			std::shared_mutex m_lock;
			std::map<Reference<Task>, size_t> m_entries;
			std::vector<Task*> m_taskList;
		};

		class TaskCollectionJob : public virtual JobSystem::Job {
		public:
			inline TaskCollectionJob(TaskSet* taskSet) : m_taskSet(taskSet) {}

			inline virtual ~TaskCollectionJob() {}

		protected:
			inline virtual void Execute() override {
				// Collect all base jobs:
				m_taskSet->GetTasks([&](Task* task) {
					});

				// __TODO__: Implement this crap!
			}

			inline virtual void CollectDependencies(Callback<Job*>) override {}

		private:
			const Reference<TaskSet> m_taskSet;

		};

		class SynchJob : public virtual JobSystem::Job {
		public:
			inline SynchJob(TaskSet* taskSet) : m_taskSet(taskSet) {}

			inline virtual ~SynchJob() {}

		protected:
			inline virtual void Execute() override {
				// __TODO__: Implement this crap!
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				// __TODO__: Implement this crap!
			}

		private:
			const Reference<TaskSet> m_taskSet;
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
				if (m_synchJob == nullptr) {
					m_synchJob = Object::Instantiate<SynchJob>(m_taskSet);
					m_context->Graphics()->SynchPointJobs().Add(m_synchJob);
				}
			}

			inline void RemoveTask(Task* task) {
				std::unique_lock<std::mutex> lock(m_taskLock);
				size_t taskCount = m_taskSet->AddTask(task);
				if (taskCount != 0u) return;
				m_context->EraseDataObject(this);
				if (m_synchJob != nullptr) {
					m_context->Graphics()->SynchPointJobs().Remove(m_synchJob);
					m_synchJob = nullptr;
				}
			}

		private:
			const Reference<SceneContext> m_context;
			const Reference<TaskSet> m_taskSet = Object::Instantiate<TaskSet>();
			std::mutex m_taskLock;
			Reference<SynchJob> m_synchJob;
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

	void ParticleSimulation::AddReference(Task* task) {
		if (task == nullptr) return;
		Reference<Helpers::Simulation> simulation = Helpers::Cache::GetSimulation(task->Buffers()->Context());
		if (simulation != nullptr)
			simulation->RemoveTask(task);
		task->ReleaseRef();
	}
}
