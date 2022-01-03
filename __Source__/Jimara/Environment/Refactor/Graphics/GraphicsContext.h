#pragma once
#include "../Scene.h"
#include "../SceneClock.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Core/Systems/Event.h"
#include "../../../Core/Systems/JobSystem.h"
#include "../../../Core/Synch/Semaphore.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	/// <summary>
	/// Scene sub-context for graphics-related routines and storage
	/// </summary>
	class Scene::GraphicsContext : public virtual Object {
	public:
		/// <summary> Graphics device </summary>
		inline Graphics::GraphicsDevice* Device()const { return m_device; }

		/// <summary> 
		///	Event, fired right before SyncPointJobs() get executed
		/// Notes: 
		///		0. Logic update lock will be held during this callback's execution, so Component modification is possible;
		///		1. Jobs added/removed from here will take effect on the same frame, but Component addition/removal will not be flushed till the next frame;
		///		2. This could be an ideal point in time to, for example, refine the final Camera position and such, but general object displacement 
		///		is not adviced anywhere inside this context.
		/// </summary>
		Event<>& PreGraphicsSynch();

		/// <summary>
		/// JobSet from the job system executed on graphics sync point
		/// Notes:
		///		0. 'Graphics synch point' is a process that is responsible for transferring scene data to the graphics objects that are being used during rendering;
		///		1. 'Graphics synch point' always executes right before the physics and logic updates and does not overlap with them.
		///		2. The system executing these jobs is multithreaded and, therefore, some caution is adviced when accessing the component data 
		///		(ideally, Components should be treated as read-only by the jobs, even if logic update lock will be held during the execution)
		///		3. Put only the work that has to do with buffer upates here for optimal performance. Any compute/rendering should probably be executed as a part of RenderJobs;
		///		4. Render job addition/removal from this system will effect corresponding job system execution for the same frame.
		/// </summary>
		JobSystem::JobSet& SynchPointJobs();

		/// <summary> 
		/// Event, fired right after SyncPointJobs() get executed
		/// Notes: 
		///		0. Logic update lock will be held during this callback's execution, so Component modification is possible, but not adviced;
		///		1. RenderJobs added from here will be executed for the same frame;
		///		2. This will be a common place for the new scene objects like the geometry and light collections to be flushed and become visible to the renderers.
		/// </summary>
		Event<>& OnGraphicsSynch();

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

		/// <summary>
		/// Event, invoked after the render job is done and the final image is calculated
		/// Notes: 
		///		0. Invoked from render thread, after it's job from the frame is done;
		///		1. Since this one runs in parallel to the logic loop and the physics synch point, it is not safe to alter or even read Component data from here;
		///		2. This could be useful to the window to blit the rendered image on demand and stuff like that. Component runtime should probably restrain itself from touching this one.
		/// </summary>
		Event<>& OnRenderFinished();

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// COnstructor
		inline GraphicsContext(Graphics::GraphicsDevice* device)
			: m_device(device) {}

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
			virtual void OnOutOfScope()const final override;

			const Reference<GraphicsContext> context;
			EventInstance<> onPreSynch;
			JobSystem synchJob;
			EventInstance<> onSynch;
			DelayedJobSystem renderJob;
			EventInstance<> onRenderFinished;
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
