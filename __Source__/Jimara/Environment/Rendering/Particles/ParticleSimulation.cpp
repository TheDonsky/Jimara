#include "ParticleSimulation.h"
#include "../../../Core/Collections/ThreadPool.h"


namespace Jimara {
	struct ParticleSimulation::Helpers {
		/// <summary>
		/// During the normal Update cycle, Components can add or remove tasks to the particle simulation;
		/// This is the collection that holds references to the particle simulation tasks per scene and is update immediately on add/remove
		/// </summary>
		class TaskSet : public virtual Object {
		public:
			/// <summary>
			/// Adds a task to the collection
			/// </summary>
			/// <param name="task"> Task to add </param>
			/// <returns> 1 if the task is inserted and number of current entries is 1 (otherwise undefined) </returns>
			inline size_t AddTask(Task* task) {
				if (task == nullptr) return ~size_t(0u);
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

			/// <summary>
			/// Removes a task from the simulation
			/// </summary>
			/// <param name="task"> Task to remove </param>
			/// <returns> Number of remaining elements inside the collection </returns>
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

			/// <summary>
			/// Iterates over all tasks within the collection
			/// </summary>
			/// <typeparam name="GetTaskFn"> Any function that receives tasks array and count as arguments </typeparam>
			/// <param name="inspectTasks"></param>
			template<typename GetTaskFn>
			inline void GetTasks(const GetTaskFn& inspectTasks) {
				std::shared_lock<std::shared_mutex> lock(m_lock);
				if (m_entries.size() != m_taskList.size()) {
					m_taskList.clear();
					for (decltype(m_entries)::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it)
						m_taskList.push_back(it->first);
				}
				size_t count = m_taskList.size();
				Task* const* tasks = m_taskList.data();
				inspectTasks(tasks, count);
			}

		private:
			std::shared_mutex m_lock;
			std::map<Reference<Task>, size_t> m_entries;
			std::vector<Task*> m_taskList;
		};





		/// <summary>
		/// A simple description of a task, alongside it's dependants and number of dependencies
		/// </summary>
		struct TaskWithDependencies {
			// Task reference
			Reference<Task> task;

			// Number of 'unscheduled' dependencies of the task
			mutable size_t dependencies = 0u;

			inline TaskWithDependencies(Task* t = nullptr) : task(t) { }
		};

		typedef ObjectSet<Task, TaskWithDependencies> TaskBuffer;
		typedef std::vector<std::vector<size_t>> DependantsBuffer;



		/// <summary>
		/// Each graphics synch point, the first step taken by particle simulation takes all tasks from the scene
		/// and collects them alongside their recursive dependencies inside a single buffer which later steps use and operate on.
		/// </summary>
		class TaskCollectionJob : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="taskSet"> TaskSet of some scene </param>
			inline TaskCollectionJob(TaskSet* taskSet) : m_taskSet(taskSet) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~TaskCollectionJob() {}

			/// <summary> Task buffer that is regenerated after each job execution </summary>
			inline const TaskBuffer& SchedulingBuffer()const { return m_taskBuffer; }

			/// <summary> Dependants for each entry in the task buffer </summary>
			inline const DependantsBuffer& Dependants()const { return m_dependants; }

		protected:
			/// <summary>
			/// Regenerates the task buffer
			/// </summary>
			inline virtual void Execute() override {
				m_taskBuffer.Clear();

				// Collect all base jobs:
				m_taskSet->GetTasks([&](Task* const* tasks, size_t count) {
					m_taskBuffer.Add(tasks, count);
					});

				// Set dependants indices:
				{
					if (m_dependants.size() < m_taskBuffer.Size())
						m_dependants.resize(m_taskBuffer.Size());
					for (size_t i = 0; i < m_taskBuffer.Size(); i++)
						m_dependants[i].clear();
				}

				// Add all dependencies to task buffer:
				for (size_t taskId = 0; taskId < m_taskBuffer.Size(); taskId++) {
					{
						const TaskWithDependencies& taskData = m_taskBuffer[taskId];
						taskData.task->GetDependencies([&](Task* t) { m_dependencyBuffer.insert(t); });
						taskData.dependencies = m_dependencyBuffer.size();
					}
					for (std::unordered_set<Reference<Task>>::const_iterator it = m_dependencyBuffer.begin(); it != m_dependencyBuffer.end(); ++it) {
						Task* dependency = *it;
						const TaskWithDependencies* dep = m_taskBuffer.Find(dependency);
						if (dep != nullptr) 
							m_dependants[dep - m_taskBuffer.Data()].push_back(taskId);
						else m_taskBuffer.Add(&dependency, 1, [&](const TaskWithDependencies* added, size_t) { 
							size_t dependantIndex = (added - m_taskBuffer.Data());
							while (dependantIndex >= m_dependants.size())
								m_dependants.push_back({});
							std::vector<size_t>& dependants = m_dependants[dependantIndex];
							dependants.clear();
							dependants.push_back(taskId);
						});
					}
					m_dependencyBuffer.clear();
				}
			}

			/// <summary>
			/// This is the first synch point step and does not have any dependencies
			/// </summary>
			inline virtual void CollectDependencies(Callback<Job*>) override {}

		private:
			const Reference<TaskSet> m_taskSet;

			TaskBuffer m_taskBuffer;
			DependantsBuffer m_dependants;
			std::unordered_set<Reference<Task>> m_dependencyBuffer;
		};





		/// <summary>
		/// Once the tasks are collected by TaskCollectionJob above, they need to be synchronized with the scene state.
		/// To do that, we have a 'swarm' of SynchJob objects that will collectively invoke Task::Synchronize() for all tasks found by TaskCollectionJob.
		/// </summary>
		class SynchJob : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collectionJob"> TaskCollectionJob that collects tasks </param>
			/// <param name="synchJobIndex"> Index of this SynchJob within the 'swarm' </param>
			/// <param name="synchJobCount"> Number of SynchJob objects within the 'swarm' </param>
			inline SynchJob(TaskCollectionJob* collectionJob, size_t synchJobIndex, size_t synchJobCount) 
				: m_collectionJob(collectionJob), m_index(synchJobIndex), m_synchJobCount(synchJobCount) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~SynchJob() {}

		protected:
			/// <summary>
			/// Each SynchJob within the swarm invokes Synchronize() method for an even subset of the tasks found by the TaskCollectionJob
			/// </summary>
			inline virtual void Execute() override {
				const TaskBuffer& tasks = m_collectionJob->SchedulingBuffer();
				const size_t tasksPerJob = (tasks.Size() + m_synchJobCount - 1) / m_synchJobCount;
				
				const size_t firstTaskIndex = m_index * tasksPerJob;
				if (firstTaskIndex >= tasks.Size()) return;

				const TaskWithDependencies* ptr = tasks.Data() + firstTaskIndex;
				const TaskWithDependencies* const end = tasks.Data() + Math::Min(firstTaskIndex + tasksPerJob, tasks.Size());
				while (ptr < end) {
					ptr->task->Synchronize();
					ptr++;
				}
			}

			/// <summary>
			/// SynchJob-s should get executed after TaskCollectionJob
			/// </summary>
			/// <param name="addDependency"> TaskCollectionJob is reported through this callback </param>
			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				addDependency(m_collectionJob);
			}

		private:
			const Reference<TaskCollectionJob> m_collectionJob;
			const size_t m_index;
			const size_t m_synchJobCount;
		};





		/// <summary>
		/// Each SimulationStep consists of one or more SimulationKernel jobs that are created per ParticleKernel;
		/// SimulationKernels run directly after the previous simulation steps and before the one that created them.
		/// </summary>
		class SimulationKernel : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="previousSimulationStep"> Previous SimulationStep (nullptr for the first one) </param>
			/// <param name="context"> Scene context </param>
			/// <param name="kernel"> Particle kernel </param>
			inline SimulationKernel(JobSystem::Job* previousSimulationStep, SceneContext* context, const ParticleKernel* kernel)
				: m_context(context), m_particleKernel(kernel), m_previousStep(previousSimulationStep) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~SimulationKernel() {}

			/// <summary> Removes all tasks </summary>
			inline void Clear() { m_tasks.clear(); }

			/// <summary> Adds a task for future execution (invoked by SimulationStep) </summary>
			inline void AddTask(Task* task) { m_tasks.push_back(task); }

			/// <summary> Retrieves current number of tasks </summary>
			inline size_t TaskCount()const { return m_tasks.size(); }

		protected:
			/// <summary>
			/// Executes underlying ParticleKernel::Instance
			/// </summary>
			inline virtual void Execute() override {
				if (m_kernelInstance == nullptr) {
					if (m_particleKernel == nullptr) return;
					m_kernelInstance = m_particleKernel->CreateInstance(m_context);
					if (m_kernelInstance == nullptr) return;
				}
				m_kernelInstance->Execute(m_context->Graphics()->GetWorkerThreadCommandBuffer(), m_tasks.data(), m_tasks.size());
			}

			/// <summary>
			/// Reports previous SimulationStep as dependency
			/// </summary>
			/// <param name="addDependency"> Previous SimulationStep is reported through this callback </param>
			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				if (m_previousStep != nullptr)
					addDependency(m_previousStep);
			}

		private:
			const Reference<SceneContext> m_context;
			const Reference<const ParticleKernel> m_particleKernel;
			const Reference<JobSystem::Job> m_previousStep;
			Reference<ParticleKernel::Instance> m_kernelInstance;
			std::vector<Task*> m_tasks;
		};

		/// <summary>
		/// Particle simulation is done on graphics render job system in several simulation steps;
		/// Each simulation step comes right after the one before it and consists of multiple unrelated simulation kernel executions that are reported as task dependencies;
		/// RenderSchedulingJob is a synch point job responsible for creating and managing simulation steps.
		/// </summary>
		class SimulationStep : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			/// <param name="previous"> Previous simulation step (nullptr for the first one) </param>
			inline SimulationStep(SceneContext* context, SimulationStep* previous) 
				: m_context(context), m_previous(previous) {}

			/// <summary>
			/// Virtual destructor
			/// </summary>
			inline virtual ~SimulationStep() {}

			/// <summary>
			/// Removes tasks from the internal task list used for scheduling (invoked by RenderSchedulingJob)
			/// </summary>
			inline void Clear() { m_tasks.clear(); }

			/// <summary>
			/// Adds task to the internal task list used for scheduling (invoked by RenderSchedulingJob)
			/// </summary>
			/// <param name="task"> Task to include </param>
			inline void AddTask(Task* task) { m_tasks.push_back(task); }

			/// <summary>
			/// Analizes internal task list and creates or populates simulation kernels which are later reported as dependencies;
			/// Executed on a separate thread right after the internal task list is filled by RenderSchedulingJob.
			/// </summary>
			inline void ScheduleKernelSubtasks() {
				CleanupKernels();
				const Reference<Task>* ptr = m_tasks.data();
				const Reference<Task>* const end = ptr + m_tasks.size();
				while (ptr < end) {
					Task* task = *ptr;
					const ParticleKernel* kernel = task->Kernel();
					Reference<SimulationKernel> job;
					{
						decltype(m_particleKernels)::iterator it = m_particleKernels.find(kernel);
						if (it != m_particleKernels.end()) job = it->second;
						else {
							job = Object::Instantiate<SimulationKernel>(m_previous, m_context, kernel);
							m_particleKernels[kernel] = job;
						}
					}
					job->AddTask(task);
					break;
				}
				m_kernelsCleared = false;
			}

		protected:
			/// <summary>
			/// Cleans up the task list, but could also do nothing, as far as I'm concerned;
			/// Invoked by the graphics render job system, right after the previous simulation step and the kernels generated by ScheduleKernelSubtasks() call
			/// </summary>
			inline virtual void Execute() override {
				CleanupKernels();
				m_tasks.clear();
			}

			/// <summary>
			/// Reports the kernels scheduled by ScheduleKernelSubtasks as dependencies
			/// </summary>
			/// <param name="addDependency"> Kernel jobs are reported through this callback </param>
			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				for (decltype(m_particleKernels)::const_iterator it = m_particleKernels.begin(); it != m_particleKernels.end(); ++it)
					addDependency(it->second);
			}

		private:
			const Reference<SceneContext> m_context;
			const Reference<SimulationStep> m_previous;
			std::vector<Reference<Task>> m_tasks;
			std::unordered_map<Reference<const ParticleKernel>, Reference<SimulationKernel>> m_particleKernels;
			std::vector<Reference<const ParticleKernel>> m_removedKernelBuffer;
			std::atomic<bool> m_kernelsCleared = true;

			void CleanupKernels() {
				if (m_kernelsCleared) return;
				for (decltype(m_particleKernels)::const_iterator it = m_particleKernels.begin(); it != m_particleKernels.end(); ++it) {
					if (it->second->TaskCount() <= 0)
						m_removedKernelBuffer.push_back(it->first);
					else it->second->Clear();
				}
				for (size_t i = 0; i < m_removedKernelBuffer.size(); i++)
					m_particleKernels.erase(m_removedKernelBuffer[i]);
				m_removedKernelBuffer.clear();
				m_kernelsCleared = true;
			}
		};





		/// <summary>
		/// After TaskCollectionJob collects all tasks and their dependencies into a nice buffer,
		/// SynchJob swarm and RenderSchedulingJob are executed;
		/// RenderSchedulingJob, specifically, is responsible for creating a job graph that will be executed on the render job system;
		/// As discussed in the description of SimulationStep, this one divides tasks into interdependent simulation steps and adds those steps to the render job system.
		/// </summary>
		class RenderSchedulingJob : public virtual JobSystem::Job {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="collectionJob"> Task collection job after which the RenderSchedulingJob is executed </param>
			/// <param name="context"> Scene context </param>
			inline RenderSchedulingJob(TaskCollectionJob* collectionJob, SceneContext* context) 
				: m_collectionJob(collectionJob), m_context(context) {}

			/// <summary>
			/// Destructor;
			/// Removes all simulation steps from the render job.
			/// </summary>
			inline virtual ~RenderSchedulingJob() {
				while (!m_simulationSteps.empty()) {
					m_context->Graphics()->RenderJobs().Remove(m_simulationSteps.back());
					m_simulationSteps.pop_back();
				}
			}

		protected:
			/// <summary>
			/// Builds the sequence of simulation steps from the scheduling buffer constructed by TaskCollectionJob;
			/// For further clarity, the tasks that have no dependencies are put in the firts simulation step; 
			/// Ones dependent only on the tasks from the firts step are combined into the second step;
			/// Tasks that depend only on the ones from the first two steps are put into the third one and so on... 
			/// </summary>
			inline virtual void Execute() override {
				const TaskBuffer& taskBuffer = m_collectionJob->SchedulingBuffer();
				const DependantsBuffer& dependants = m_collectionJob->Dependants();
				m_stepTaskBuffer.clear();
				m_stepTaskBackBuffer.clear();
				size_t numSimulationSteps = 0u;
				size_t tasksToExecute = taskBuffer.Size();

				// Find the initial jobs to execute:
				{
					const TaskWithDependencies* ptr = taskBuffer.Data();
					const TaskWithDependencies* const end = ptr + taskBuffer.Size();
					while (ptr < end) {
						if (ptr->dependencies <= 0u)
							m_stepTaskBuffer.push_back(ptr);
						ptr++;
					}
				}

				// Find next layers iteratively:
				while (tasksToExecute > 0) {
					// Terminate early if there are circular dependencies:
					if (m_stepTaskBuffer.size() <= 0) {
						m_context->Log()->Error(
							"ParticleSimulation::Helpers::RenderSchedulingJob::Execute",
							" - Task graph contains circular dependencies! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						break;
					}

					// Get new render step to execute:
					SimulationStep* simulationStep = [&]() {
						while (m_simulationSteps.size() <= numSimulationSteps) {
							SimulationStep* previous;
							if (m_simulationSteps.size() > 0u)
								previous = m_simulationSteps.back();
							else previous = nullptr;
							Reference<SimulationStep> step = Object::Instantiate<SimulationStep>(m_context, previous);
							m_context->Graphics()->RenderJobs().Add(step);
							m_simulationSteps.push_back(step);
						}
						SimulationStep* step = m_simulationSteps[numSimulationSteps];
						step->Clear();
						numSimulationSteps++;
						return step;
					}();

					// Find new jobs to execute:
					{
						const TaskWithDependencies* const* ptr = m_stepTaskBuffer.data();
						const TaskWithDependencies* const* const end = ptr + m_stepTaskBuffer.size();
						while (ptr < end) {
							const TaskWithDependencies* task = *ptr;
							simulationStep->AddTask(task->task);

							const std::vector<size_t>& dep = dependants[task - taskBuffer.Data()];
							const size_t* depPtr = dep.data();
							const size_t* const depEnd = depPtr + dep.size();
							while (depPtr != depEnd) {
								const TaskWithDependencies& dep = taskBuffer[*depPtr];
								dep.dependencies--;
								if (dep.dependencies <= 0)
									m_stepTaskBackBuffer.push_back(&dep);
								depPtr++;
							}

							ptr++;
						}
						tasksToExecute -= Math::Min(m_stepTaskBuffer.size(), tasksToExecute);
					}

					// Schedule step kernel assignment:
					m_stepSchedulingPool.Schedule(
						Callback<Object*>(&RenderSchedulingJob::ScheduleKernelSubtasks, this),
						simulationStep);

					// Swap buffers:
					{
						m_stepTaskBuffer.clear();
						std::swap(m_stepTaskBackBuffer, m_stepTaskBuffer);
					}
				}

				// Remove extra render steps we no longer need:
				while (m_simulationSteps.size() > numSimulationSteps) {
					m_context->Graphics()->RenderJobs().Remove(m_simulationSteps.back());
					m_simulationSteps.pop_back();
				}

				// Synchronize with step kernel assignment:
				while (numSimulationSteps > 0u) {
					m_stepSchedulingSemaphore.wait();
					numSimulationSteps--;
				}
			}

			/// <summary>
			/// Reports TaskCollectionJob as it's only dependency
			/// </summary>
			/// <param name="addDependency"> TaskCollectionJob is reported through this callback </param>
			inline virtual void CollectDependencies(Callback<Job*> addDependency) override {
				addDependency(m_collectionJob);
			}

		private:
			const Reference<TaskCollectionJob> m_collectionJob;
			const Reference<SceneContext> m_context;
			std::vector<Reference<SimulationStep>> m_simulationSteps;
			std::vector<const TaskWithDependencies*> m_stepTaskBuffer;
			std::vector<const TaskWithDependencies*> m_stepTaskBackBuffer;
			Semaphore m_stepSchedulingSemaphore = Semaphore(0u);
			ThreadPool m_stepSchedulingPool = ThreadPool(1u);

			void ScheduleKernelSubtasks(Object* simualtionStepPtr) {
				dynamic_cast<SimulationStep*>(simualtionStepPtr)->ScheduleKernelSubtasks();
				m_stepSchedulingSemaphore.post();
			}
		};





		/// <summary>
		/// Particle simulation is created per scene and consists of these elements:
		/// 0. TaskSet that contains all manually registered tasks within the simulation system;
		/// 1. TaskCollectionJob that reads TaskSet, extracts all dependencies recursively and provides the buffer to the other synch point jobs;
		/// 2. A 'swarm' of SynchJob-s that execute after TaskCollectionJob on the graphics synch point and synchronize all the tasks found by it;
		/// 3. RenderSchedulingJob that executes after TaskCollectionJob alongside SynchJob-s and constructs a chain of simulation steps for the graphics render job system;
		/// 4. Multiple SimulationStep-s generated and managed internally by RenderSchedulingJob that execute after each other on the graphics render job system;
		/// 5. SimulationKernel objects created and managed internally by each SimulationStep per individual simulation kernel that execute right before each SimulationStep.
		/// </summary>
		class Simulation : public virtual ObjectCache<Reference<SceneContext>>::StoredObject {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			inline Simulation(SceneContext* context) : m_context(context) {
				assert(m_context != nullptr);
			}

			/// <summary>
			/// Virtual destructor
			/// </summary>
			inline virtual ~Simulation() {
				RemoveTask(nullptr);
				RemoveAllJobs();
			}

			/// <summary>
			/// Adds simulation task and if it's the first one, creates and activates the synch point jobs
			/// </summary>
			/// <param name="task"> Task to add to the system </param>
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
					m_schedulingJob = Object::Instantiate<RenderSchedulingJob>(m_taskCollectionJob, m_context);
					m_context->Graphics()->SynchPointJobs().Add(m_schedulingJob);
				}
			}

			/// <summary>
			/// Removes a task from the system and Destroys all synch point jobs if the task set becomes empty
			/// </summary>
			/// <param name="task"> Task to remove </param>
			inline void RemoveTask(Task* task) {
				std::unique_lock<std::mutex> lock(m_taskLock);
				size_t taskCount = m_taskSet->RemoveTask(task);
				if (taskCount == 0u)
					RemoveAllJobs();
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

			inline void RemoveAllJobs() {
				RemoveSchedulingJob();
				RemoveSynchJobs();
				if (m_taskCollectionJob != nullptr) {
					m_context->Graphics()->SynchPointJobs().Remove(m_taskCollectionJob);
					m_taskCollectionJob = nullptr;
				}
				m_context->EraseDataObject(this);
			}
		};

		/// <summary>
		/// Cache of Simulation instances per SceneContext
		/// </summary>
		class Cache : public virtual ObjectCache<Reference<SceneContext>> {
		public:
			inline static Reference<Simulation> GetSimulation(SceneContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false, [&]()->Reference<Simulation> { return Object::Instantiate<Simulation>(context); });
			}
		};
	};


	void ParticleSimulation::AddTask(Task* task) {
		if (task == nullptr) return;
		Reference<Helpers::Simulation> simulation = Helpers::Cache::GetSimulation(task->Buffers()->Context());
		if (simulation != nullptr)
			simulation->AddTask(task);
	}

	void ParticleSimulation::RemoveTask(Task* task) {
		if (task == nullptr) return;
		Reference<Helpers::Simulation> simulation = Helpers::Cache::GetSimulation(task->Buffers()->Context());
		if (simulation != nullptr)
			simulation->RemoveTask(task);
	}

	void ParticleSimulation::AddReference(Task* task) {
		if (task == nullptr) return;
		task->AddRef();
		AddTask(task);
	}

	void ParticleSimulation::ReleaseReference(Task* task) {
		if (task == nullptr) return;
		RemoveTask(task);
		task->ReleaseRef();
	}
}
