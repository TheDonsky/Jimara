#pragma once
#include "../Scene.h"
#include "../SceneClock.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Core/Systems/Event.h"
#include "../../../Core/Systems/JobSystem.h"
#include "../../../Core/Synch/Semaphore.h"


namespace Jimara {
	/// <summary>
	/// Scene sub-context for graphics-related routines and storage
	/// </summary>
	class JIMARA_API Scene::GraphicsContext : public virtual Object {
	public:
		/// <summary>
		/// General settings for GraphicsContext
		/// <para/> Note: 
		///		<para/> This is not necessarily what one would consider to be 'graphics settings'; 
		///		<para/> this is more like a general set of preferences and parameters the graphics jobs and components might need to operate correctly.
		/// </summary>
		class JIMARA_API ConfigurationSettings {
		public:
			/// <summary> Maximal number of in-flight command buffers that can be executing simultaneously </summary>
			inline size_t MaxInFlightCommandBufferCount()const { return m_maxInFlightCommandBuffers; }

			/// <summary> Shader loader </summary>
			inline Graphics::ShaderLoader* ShaderLoader()const { return m_shaderLoader; }

		private:
			// Maximal number of in-flight command buffers that can be executing simultaneously
			const size_t m_maxInFlightCommandBuffers;

			// Shader loader
			const Reference<Graphics::ShaderLoader> m_shaderLoader;

			// Only the graphics context can access the constructor
			ConfigurationSettings(const CreateArgs& createArgs);
			friend class GraphicsContext;
		};

		/// <summary>
		/// Collection of bindless resources that can and will be used by the whole render job system
		/// </summary>
		class JIMARA_API BindlessSets {
		public:
			/// <summary> Bindless set of structured buffers </summary>
			inline Graphics::BindlessSet<Graphics::ArrayBuffer>* Buffers()const { return m_bindlessArrays; }

			/// <summary> Bindless sets of texture samplers </summary>
			inline Graphics::BindlessSet<Graphics::TextureSampler>* Samplers()const { return m_bindlessSamplers; }

			/// <summary> Main instance of Buffers </summary>
			inline Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance* BufferBinding()const { return m_bindlessArrayInstance; }

			/// <summary> Main instance of Samplers </summary>
			inline Graphics::BindlessSet<Graphics::TextureSampler>::Instance* SamplerBinding()const { return m_bindlessSamplerInstance; }

		private:
			// Bindless buffer set
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> m_bindlessArrays;

			// Bindless sampler set
			const Reference<Graphics::BindlessSet<Graphics::TextureSampler>> m_bindlessSamplers; 

			// Main instance of m_bindlessArrays
			const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> m_bindlessArrayInstance;

			// Main instance of m_bindlessSamplers
			const Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Instance> m_bindlessSamplerInstance;

			// Only the graphics context can access the constructor
			friend class GraphicsContext;
			BindlessSets(
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>>& bindlessArrays,
				const Reference<Graphics::BindlessSet<Graphics::TextureSampler>>& bindlessSamplers,
				const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>& bindlessArrayInstance,
				const Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>& bindlessSamplerInstance,
				size_t inFlightBufferCount);
			BindlessSets(const CreateArgs& createArgs);

			// No copy/move or copy/move-constructions
			inline BindlessSets(const BindlessSets&) = delete;
			inline BindlessSets(BindlessSets&&) = delete;
			inline BindlessSets& operator=(const BindlessSets&) = delete;
			inline BindlessSets& operator=(BindlessSets&&) = delete;
		};

		/// <summary> General settings for GraphicsContext </summary>
		inline ConfigurationSettings& Configuration() { return m_configuration; }

		/// <summary> Globally available bindless sets </summary>
		inline BindlessSets& Bindless() { return m_bindlessSets; }

		/// <summary> Graphics device </summary>
		inline Graphics::GraphicsDevice* Device()const { return m_device; }

		/// <summary>
		/// Graphics command buffer with in-flight index for a worker thread
		/// <para/> Notes:
		///		<para/> 0. Calling this is valid from PreGraphicsSynch event, SynchPointJobs job system, OnGraphicsSynch event and RenderJobs job system 
		///		(the last one includes the render stack entries);
		///		<para/> 1. Each command buffer returned by this function will be initialized and in a recording state;
		///		<para/> 2. Each command buffer will be automatically submitted after the corresponding event or the job system iteration;
		///		<para/> 3. Submitting the command buffer from the worker thread will result in unsafe state, causing a bounch of errors/crashes and generally bad stuff. So do not do it!
		///		<para/> 4. Command buffers are guaranteed to work fine for the job/event handler that got it int only for the duration of said event/job iteration; 
		///		<para/> 5. Saving these command buffers or using them from asynchronous threads is completely unsafe and should not be done unless you seek trouble;
		///		<para/> 6. For each event iteration, a single command buffer will be reused with a single initialization/reset on the first call and a single submition at the end;
		///		<para/> 7. For each job system worker thread iteration, a single command buffer will be used just like the events and each job thread will have it's own command buffer;
		///		<para/> 8. Job systems will submit command buffers after each iteration, returning new instances once the "job waves" change;
		///		<para/> 9. inFlightBufferId will stay constant each update cycle, but the client code should really not concern itself too much with that curiosity;
		///		<para/> 10. inFlightBufferId will be in range : [0; Configuration().MaxInFlightCommandBufferCount()).
		/// </summary>
		Graphics::Pipeline::CommandBufferInfo GetWorkerThreadCommandBuffer();

		/// <summary> 
		///	Event, fired right before SyncPointJobs() get executed
		/// <para/> Notes: 
		///		<para/> 0. Logic update lock will be held during this callback's execution, so Component modification is possible;
		///		<para/> 1. Jobs added/removed from here will take effect on the same frame, but Component addition/removal will not be flushed till the next frame;
		///		<para/> 2. This could be an ideal point in time to, for example, refine the final Camera position and such, but general object displacement 
		///		is not adviced anywhere inside this context.
		/// </summary>
		Event<>& PreGraphicsSynch();

		/// <summary>
		/// JobSet from the job system executed on graphics sync point
		/// <para/> Notes:
		///		<para/> 0. 'Graphics synch point' is a process that is responsible for transferring scene data to the graphics objects that are being used during rendering;
		///		<para/> 1. 'Graphics synch point' always executes right before the physics and logic updates and does not overlap with them.
		///		<para/> 2. The system executing these jobs is multithreaded and, therefore, some caution is adviced when accessing the component data 
		///		(ideally, Components should be treated as read-only by the jobs, even if logic update lock will be held during the execution)
		///		<para/> 3. Put only the work that has to do with buffer upates here for optimal performance. Any compute/rendering should probably be executed as a part of RenderJobs;
		///		<para/> 4. Render job addition/removal from this system will effect corresponding job system execution for the same frame.
		/// </summary>
		JobSystem::JobSet& SynchPointJobs();

		/// <summary> 
		/// Event, fired right after SyncPointJobs() get executed
		/// <para/> Notes: 
		///		<para/> 0. Logic update lock will be held during this callback's execution, so Component modification is possible, but not adviced;
		///		<para/> 1. RenderJobs added from here will be executed for the same frame;
		///		<para/> 2. This will be a common place for the new scene objects like the geometry and light collections to be flushed and become visible to the renderers.
		/// </summary>
		Event<>& OnGraphicsSynch();

		/// <summary>
		/// JobSet from the job system executed parallel to the logic update routines
		/// <para/> Notes:
		///		<para/> 0. 'Render job' is a general purpose job system, but it's intended use case is rendering graphics and running non-synchronous compute pipelines;
		///		<para/> 1. This system runs in parallel to the logic and physics update cycles and, therefore, accessing Component data from these is, generally, not safe;
		///		<para/> 2. One should use SyncPointJobs to transfer relevant data to the GPU and/or the jobs from the 'Render Job' system 
		///		instead of reading anything from the component heirarchy during rendering;
		///		<para/> 3. One can add/remove jobs any time, but the changes will take effect only after the sync point;
		///		<para/> 4. Render stack runs as a part of the render job system, so in case your job is only relevant to the renderers from the stack, 
		///		their dependencies will be more than enough and there's no need to add those jobs here.
		/// </summary>
		JobSystem::JobSet& RenderJobs();

		/// <summary>
		/// Event, invoked after the render job is done and the final image is calculated
		/// <para/> Notes: 
		///		<para/> 0. Invoked from render thread, after it's job from the frame is done;
		///		<para/> 1. Since this one runs in parallel to the logic loop and the physics synch point, it is not safe to alter or even read Component data from here;
		///		<para/> 2. This could be useful to the window to blit the rendered image on demand and stuff like that. Component runtime should probably restrain itself from touching this one.
		/// </summary>
		Event<>& OnRenderFinished();

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Configuration
		ConfigurationSettings m_configuration;

		// Bindless sets
		BindlessSets m_bindlessSets;

		// Frame/iteration data
		struct {
			std::atomic<size_t> inFlightWorkerCommandBufferId = ~size_t(0);
			std::atomic<bool> canGetWorkerCommandBuffer = false;
		} m_frameData;

		// Constructor
		inline GraphicsContext(const CreateArgs& createArgs);

		// Executes graphics sync point
		void Sync(LogicContext* context);

		// Starts Render Job
		void StartRender();

		// Waits till the Render job is finished
		void SyncRender();

		// Job system, that is tied to synch point jobs
		struct SynchPointJobSystem : public virtual JobSystem::JobSet {
			GraphicsContext* context = nullptr;

			inline virtual void Add(JobSystem::Job* job) final override {
				Reference<Data> data = context->m_data;
				if (data != nullptr) data->synchJob.Add(job);
			}
			inline virtual void Remove(JobSystem::Job* job) final override {
				Reference<Data> data = context->m_data;
				if (data != nullptr) data->synchJob.Remove(job);
			}
		} m_synchPointJobs;

		// Job system, that is tied to render jobs
		struct RenderJobSystem : public virtual JobSystem::JobSet {
			GraphicsContext* context = nullptr;

			inline virtual void Add(JobSystem::Job* job) final override {
				Reference<Data> data = context->m_data;
				if (data != nullptr) data->renderJob.Add(job);
			}
			inline virtual void Remove(JobSystem::Job* job) final override {
				Reference<Data> data = context->m_data;
				if (data != nullptr) data->renderJob.Remove(job);
			}
		} m_renderJobs;

		// Job system, that operates with some delay
		struct DelayedJobSystem : public virtual JobSystem::JobSet {
			JobSystem jobSystem;
			std::mutex setLock;
			DelayedObjectSet<JobSystem::Job> jobSet;
			std::vector<Reference<JobSystem::Job>> removedJobBuffer;
			inline DelayedJobSystem(size_t threadCount) : jobSystem(threadCount) {}
			inline virtual void Add(JobSystem::Job* job) final override {
				std::unique_lock<std::mutex> lock(setLock);
				jobSet.ScheduleAdd(job); 
			}
			inline virtual void Remove(JobSystem::Job* job) final override {
				std::unique_lock<std::mutex> lock(setLock);
				jobSet.ScheduleRemove(job); 
			}
		};

		// Graphics scene data
		struct Data : public virtual Object {
			static Reference<Data> Create(CreateArgs& createArgs);
			Data(const CreateArgs& createArgs);
			virtual void OnOutOfScope()const final override;

			const Reference<GraphicsContext> context;
			EventInstance<> onPreSynch;
			JobSystem synchJob;
			EventInstance<> onSynch;
			DelayedJobSystem renderJob;
			EventInstance<> onRenderFinished;

			SpinLock workerCleanupLock;
			std::vector<std::pair<Reference<Object>, Callback<>>> workerCleanupJobs;
			typedef std::pair<Reference<Object>, Reference<Graphics::PrimaryCommandBuffer>> CMD_PoolAndBuffer;
			typedef std::pair<CMD_PoolAndBuffer, Callback<Graphics::PrimaryCommandBuffer*>> CMD_BufferReleaseCall;
			std::vector<std::vector<CMD_BufferReleaseCall>> inFlightBufferCleanupJobs;
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
