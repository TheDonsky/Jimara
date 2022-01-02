#pragma once
#include "../Scene.h"
#include "../SceneClock.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Core/Systems/Event.h"
#include "../../../Core/Systems/JobSystem.h"
#include "../../../Core/Synch/Semaphore.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::GraphicsContext : public virtual Object {
	public:
		/// <summary>
		/// JobSet from the job system executed on graphics sync point
		/// Notes:
		///		0. 'Graphics sync point' is a process that is responsible for transferring scene data to the graphics objects that are being used during rendering;
		///		1. 'Graphics sync point' always executes right before the physics and logic updates and does not overlap with them.
		///		2. The system executing these jobs is multithreaded and, therefore, some caution is adviced when accessing the component data 
		///		(ideally, Components should be treated as read-only by the jobs)
		///		3. Put only the work that has to do with buffer upates here for optimal performance. Any compute/rendering should probably be executed as a part of RenderJobs;
		///		4. Render job addition/removal from this system will effect corresponding job system execution for the same frame.
		/// </summary>
		JobSystem::JobSet& SyncPointJobs();

		// __TODO__: Add scene object add/remove/listen functionality

		/// <summary>
		/// JobSet from the job system executed parallel to the logic update routines
		/// Notes:
		///		0. 'Render job' is a general purpose job system, but it's intended use case is rendering graphics and running non-synchronous compute pipelines;
		///		1. This system runs in parallel to the logic and physics update cycles and, therefore, accessing Component data from these is, generally, not safe;
		///		2. One should use SyncPointJobs to transfer relevant data to the GPU and/or the jobs from the 'Render Job' system 
		///		instead of reading anything from the component heirarchy during rendering;
		///		3. One can add/remove jobs any time, but the changes will take effect only after the sync point.
		/// </summary>
		JobSystem::JobSet& RenderJobs();

		// __TODO__: Add 'global compositor' for target image management and alike

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// COnstructor
		inline GraphicsContext(Graphics::GraphicsDevice* device) : m_device(device) {}

		// Executes graphics sync point
		void Sync();

		// Starts Render Job
		void StartRender();

		// Waits till the Render job is finished
		void SyncRender();

		// Job system, that operates with some delay
		struct DelayedJobSystem : public virtual JobSystem::JobSet {
			JobSystem jobSystem;
			DelayedObjectSet<JobSystem::Job> jobSet;
			inline virtual void Add(JobSystem::Job* job) final override { jobSet.ScheduleAdd(job); }
			inline virtual void Remove(JobSystem::Job* job) final override { jobSet.ScheduleRemove(job); }
		};

		// Graphics scene data
		struct Data : public virtual Object {
			static Reference<Data> Create(Graphics::GraphicsDevice* device, OS::Logger* logger);
			Data(Graphics::GraphicsDevice* device);
			virtual ~Data();

			const Reference<GraphicsContext> context;
			JobSystem syncJob;
			DelayedJobSystem renderJob;
		};
		DataWeakReference<Data> m_data;

		// External thread for render jobs
		struct {
			std::mutex renderLock;
			std::atomic<bool> rendering = false;
			Semaphore startSemaphore;
			Semaphore doneSemaphore;
			std::thread renderThread;
		} m_renderThread;

		// Only the scene can create instances and invoke lifecycle functions
		friend class Scene;
	};
}
}
