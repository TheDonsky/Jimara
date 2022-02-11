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
	class Scene::GraphicsContext : public virtual Object {
	public:
		/// <summary>
		/// General settings for GraphicsContext
		/// Note: 
		///		This is not necessarily what one would consider to be 'graphics settings'; 
		///		this is more like a general set of preferences and parameters the graphics jobs and components might need to operate correctly.
		/// </summary>
		class ConfigurationSettings {
		public:
			/// <summary> Maximal number of in-flight command buffers that can be executing simultaneously </summary>
			inline size_t MaxInFlightCommandBufferCount()const { return m_maxInFlightCommandBuffers; }

			/// <summary> Shader loader </summary>
			inline Graphics::ShaderLoader* ShaderLoader()const { return m_shaderLoader; }

			/// <summary>
			/// Translates light type name to unique type identifier that can be used within the shaders
			/// </summary>
			/// <param name="lightTypeName"> Light type name </param>
			/// <param name="lightTypeId"> Reference to store light type identifier at </param>
			/// <returns> True, if light type was found </returns>
			bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const;

			/// <summary> Maximal size of a single light data buffer </summary>
			inline size_t PerLightDataSize()const { return m_perLightDataSize; }

		private:
			// Maximal number of in-flight command buffers that can be executing simultaneously
			const size_t m_maxInFlightCommandBuffers;

			// Shader loader
			const Reference<Graphics::ShaderLoader> m_shaderLoader;

			// Light type name to typeId mapping
			const std::unordered_map<std::string, uint32_t> m_lightTypeIds;

			// Maximal size of a single light data buffer
			const size_t m_perLightDataSize;

			// Only the graphics context can access the constructor
			ConfigurationSettings(const CreateArgs& createArgs);
			friend class GraphicsContext;
		};

		/// <summary> General settings for GraphicsContext </summary>
		inline ConfigurationSettings& Configuration() { return m_configuration; }

		/// <summary> Graphics device </summary>
		inline Graphics::GraphicsDevice* Device()const { return m_device; }

		/// <summary>
		/// Graphics command buffer with in-flight index for a worker thread
		/// Notes:
		///		0. Calling this is valid from PreGraphicsSynch event, SynchPointJobs job system, OnGraphicsSynch event and RenderJobs job system 
		///		(the last one includes the render stack);
		///		1. Each command buffer returned by this function will be initialized and in a recording state;
		///		2. Each command buffer will be automatically submitted after the corresponding event or the job system iteration;
		///		3. Submitting the command buffer from the worker thread will result in unsafe state, causing a bounch of errors/crashes and generally bad stuff. So do not do it!
		///		4. Command buffers are guaranteed to work fine for the job/event handler that got it int only for the duration of said event/job iteration; 
		///		5. Saving these command buffers or using them from asynchronous threads is completely unsafe and should not be done unless you seek trouble;
		///		6. For each event iteration, a single command buffer will be reused with a single initialization/reset on the first call and a single submition at the end;
		///		7. For each job system worker thread iteration, a single command buffer will be used just like the events and each job thread will have it's own command buffer;
		///		8. Job systems will submit command buffers after each iteration, returning new instances once the "job waves" change;
		///		9. inFlightBufferId will stay constant each update cycle, but the client code should really not concern itself too much with that curiosity;
		///		10. inFlightBufferId will be in range : [0; Configuration().MaxInFlightCommandBufferCount()).
		/// </summary>
		Graphics::Pipeline::CommandBufferInfo GetWorkerThreadCommandBuffer();

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
		///		3. One can add/remove jobs any time, but the changes will take effect only after the sync point;
		///		4. Render stack runs as a part of the render job system, so in case your job is only relevant to the renderers from the stack, 
		///		their dependencies will be more than enough and there's no need to add those jobs here.
		/// </summary>
		JobSystem::JobSet& RenderJobs();

		/// <summary>
		/// Abstract renderer for final image generation
		/// Note: These renderers normally run as a part of the renderer stack in a well-defined order
		/// </summary>
		class Renderer : public virtual Object {
		public:
			/// <summary>
			/// Should render the image to the given texture
			/// Note:
			///		0. targetTexture can remain the same over all frames, can vary between several swap chain images or be set to something random each frame;
			///		1. Taking the above into consideration, one can feel free to store a limited number of frame buffers, just in case there is a need to reuse,
			///		but it is extremly unlikely for it to be more target textures rotating around than the in-flight command buffers (so that should be your target cache size).
			///		2. RenderStack executes Renderers one after the another, passing the 'results' based on the category and the priority; 
			///		this means that not all renderers should be clearing the screen (overlays and postFX should definately do no such thing, for example).
			/// </summary>
			/// <param name="commandBufferInfo"> Command buffer and in-flight buffer index </param>
			/// <param name="targetTexture"> Texture, to render to </param>
			virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, Graphics::TextureView* targetTexture) = 0;

			/// <summary>
			/// RenderStack gets executed as a Job in RenderJobs system; if any of the renderers that are part of it jave some jobs they depend on,
			/// they can report those through this callback
			/// </summary>
			/// <param name="report"> Dependencies can be reported through this callback </param>
			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) { Unused(report); }

			/// <summary>
			/// Renderer 'category'
			/// Notes:
			///		0. Lower category renderers will be executed first, followed by the higher category ones;
			///		1. Global user interface may expose categories by something like an enumeration, 
			///		containing 'Camera/Geometry', 'PostFX', 'UI/Overlay' and such, but the engine internals do not care about any such thing;
			///		2. If the categories match, higher Priority renderers will be called first;
			///		3. Priorities are just numbers both in code and from UI;
			///		4. If both the category and priority are the same, rendering order is undefined
			/// </summary>
			inline uint32_t Category()const { return m_category.load(); }

			/// <summary>
			/// Sets the renderer category
			/// Note: Render job system will aknoweledge the change only after the graphics synch point
			/// </summary>
			/// <param name="category"> New category to assign this renderer to </param>
			inline void SetCategory(uint32_t category) { m_category = category; }

			/// <summary>
			/// Renderer 'priority' inside the same category
			/// Notes:
			///		0. Lower category renderers will be executed first, followed by the higher category ones;
			///		1. Global user interface may expose categories by something like an enumeration, 
			///		containing 'Camera/Geometry', 'PostFX', 'UI/Overlay' and such, but the engine internals do not care about any such thing;
			///		2. If the categories match, higher Priority renderers will be called first;
			///		3. Priorities are just numbers both in code and from UI;
			///		4. If both the category and priority are the same, rendering order is undefined
			/// </summary>
			inline uint32_t Priority()const { return m_priority.load(); }

			/// <summary>
			/// Sets the renderer priority inside the same category
			/// Note: Render job system will aknoweledge the change only after the graphics synch point
			/// </summary>
			/// <param name="priority">  New priority to assign this renderer to </param>
			inline void SetPriority(uint32_t priority) { m_priority = priority; }

		private:
			// Category
			std::atomic<uint32_t> m_category = 0;

			// Priority
			std::atomic<uint32_t> m_priority = 0;
		};

		/// <summary>
		/// Renderer stack for generating final output
		/// Notes:
		///		0. RenderStack renders the image by sequentially invoking Render() function from each Renderer added to it;
		///		1. RenderStack is not responsible for clearing the textures, or creating frame buffers;
		///		2. Renderer-s are sorted by their Category() and Priority() after each graphics synch point.
		/// </summary>
		class RenderStack {
		public:
			/// <summary>
			/// Adds a renderer to the stack
			/// Note: This takes effect after the graphics synch point.
			/// </summary>
			/// <param name="renderer"> Remderer to add </param>
			void AddRenderer(Renderer* renderer);

			/// <summary>
			/// Removes a renderer from the stack
			/// Note: This takes effect after the graphics synch point.
			/// </summary>
			/// <param name="renderer"> Remderer to remove </param>
			void RemoveRenderer(Renderer* renderer);

			/// <summary> 
			/// Target texture view
			/// Note: 
			///		Value will be consistent wih the latest SetTargetTexture() call, 
			///		but the stack will always render to the one that was set before the last graphics synch point.
			/// </summary>
			Reference<Graphics::TextureView> TargetTexture()const;

			/// <summary>
			/// Sets target texture view
			/// Note: 
			///		Value will be consistent wih the latest SetTargetTexture() call, 
			///		but the stack will always render to the one that was set before the last graphics synch point.
			/// </summary>
			/// <param name="targetTexture"> Texture to render to </param>
			void SetTargetTexture(Graphics::TextureView* targetTexture);

		private:
			// Owner
			GraphicsContext* const m_context;

			// Current texture and it's lock
			mutable SpinLock m_currentTargetTextureLock;
			Reference<Graphics::TextureView> m_currentTargetTexture;

			// Only the graphics context can create the stack
			inline RenderStack(GraphicsContext* context) : m_context(context) {}
			friend class GraphicsContext;
		};

		/// <summary> Renderer stack </summary>
		inline RenderStack& Renderers() { return m_rendererStack; }

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

		// Configuration
		ConfigurationSettings m_configuration;

		// Renderer stack
		RenderStack m_rendererStack;

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

		// Job system, that operates with some delay
		struct DelayedJobSystem : public virtual JobSystem::JobSet {
			JobSystem jobSystem;
			std::mutex setLock;
			DelayedObjectSet<JobSystem::Job> jobSet;
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

			std::mutex rendererLock;
			DelayedObjectSet<Renderer> rendererSet;
			struct RendererStackEntry {
				Reference<Renderer> renderer;
				uint64_t priority = 0;
				inline RendererStackEntry() {}
				inline RendererStackEntry(Renderer* r) : renderer(r) {
					if (r != nullptr) priority =
						(static_cast<uint64_t>(r->Category()) << 32) |
						static_cast<uint64_t>((~uint32_t(0)) - r->Priority());
				}
				inline bool operator<(const RendererStackEntry& other)const { 
					return priority < other.priority || (priority == other.priority && renderer < other.renderer); 
				}
			};
			std::vector<RendererStackEntry> rendererStack;
			Reference<Graphics::TextureView> rendererTargetTexture;

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
