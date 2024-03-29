#pragma once
#include "../Graphics/GraphicsContext.h"
#include "../Physics/PhysicsContext.h"
#include "../Audio/AudioContext.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Core/Systems/ActionQueue.h"
#include "../../../Data/AssetDatabase/AssetDatabase.h"
#include "../../../Components/Component.h"
#include <mutex>

namespace Jimara {
	/// <summary>
	/// Main scene context
	/// Note: 
	///		Gives access to the engine internals to the components, 
	///		without exposing anything that would harm the runtime and main update cycle
	/// </summary>
	class JIMARA_API SceneContext : public virtual Object {
	public:
		/// <summary> Scene update cycle clock </summary>
		inline Scene::Clock* Time()const { return m_time; }

		/// <summary> Index of the current frame since start (not to be confused with UpdateIndex(), which does not count SynchAndRender calls) </summary>
		inline uint64_t FrameIndex()const { return m_frameIndex.load(); }

		/// <summary> Index of the current update cycle since start (not to be confused with FrameIndex(), which also counts SynchAndRender calls) </summary>
		inline uint64_t UpdateIndex()const { return m_updateIndex.load(); }

		/// <summary> Main logger </summary>
		inline OS::Logger* Log()const { return m_logger; }

		/// <summary> Input module </summary>
		inline const OS::Input* Input()const { return m_input; }

		/// <summary> Asset database </summary>
		inline AssetDatabase* AssetDB()const { return m_assetDatabase; }

		/// <summary> Sub-context for graphics-related stuff </summary>
		inline Scene::GraphicsContext* Graphics()const { return m_graphics; }

		/// <summary> Sub-context for physics-related stuff </summary>
		inline Scene::PhysicsContext* Physics()const { return m_physics; }

		/// <summary> Sub-context for audio-related stuff </summary>
		inline Scene::AudioContext* Audio()const { return m_audio; }

		/// <summary>
		/// Update lock
		/// Notes:
		///		0. This lock is automatically aquired during the update callbacks, physics uppdate and graphics synch point;
		///		1. Any of the asynchronous job systems that are part of the synch points or the main update cycle will naturally deadlock if they try to use this lock;
		///		2. Under most circumstances no component, job or any other scene object should access this lock; it's mostly for the external runtime stuff, 
		///		like the editor and what not;
		///		3. Object creation/destruction, as well as enabling/disabling will lock this mutex, so doing those from job threads is also not viable.
		/// </summary>
		inline std::recursive_mutex& UpdateLock()const { return m_updateLock; }

		/// <summary> 
		/// True, when update loop is running as a part of Scene::Update()
		/// Notes:
		///		0. This has little meaning outside the main update thread;
		///		1. This will be true during the entirety of Scene::Update() call;
		///		2. Scene::SynchAndRender will not set this flag to let the system know the logic/physics engine is not actually running.
		/// </summary>
		inline bool Updating()const { return m_updating.load(); }

		/// <summary> 
		/// Root component 
		/// Notes:
		///		0. Any component will have this one on top of their parent heirarchy, unless the user creates some custom object that is not a 'normal' part of the heirarchy;
		///		1. Destroying the root object while the scene still exists will cause the entire tree to be deleted and the root being replaced.
		/// </summary>
		Reference<Component> RootObject()const;

		/// <summary>
		/// Component that updates each scene update cycle
		/// </summary>
		class JIMARA_API UpdatingComponent : public virtual Component {
		protected:
			/// <summary> Updates component </summary>
			virtual void Update() = 0;

		private:
			// Only the Logic context is allowed to invoke Update() method
			friend class SceneContext;
		};

		/// <summary> Invoked right before UpdatingComponents get updated </summary>
		inline Event<>& OnPreUpdate() { return m_onPreUpdate; }

		/// <summary> Invoked right after UpdatingComponents get updated </summary>
		inline Event<>& OnUpdate() { return m_onUpdate; }

		/// <summary> Invoked right after OnUpdate() if the scene is updated, as well as during a scne synch-and-render </summary>
		inline Event<>& OnSynchOrUpdate() { return m_onSynchOrUpdate; }

		/// <summary> Invoked right after a new Component gets initialized </summary>
		inline Event<Component*>& OnComponentCreated() { return m_onComponentCreated; }

		/// <summary>
		/// Executes arbitrary callback after OnPreUpdate(), Update and OnUpdate() events
		/// Note: Takes effect on the same frame; schedules from graphics synch point or queued callbacks will be executed on the next frame
		/// </summary>
		/// <param name="callback"> Callback to execute </param>
		/// <param name="userData"> Arbitrary object to keep alive while the callback is queued (it will be passed back as the callback's argument) </param>
		void ExecuteAfterUpdate(const Callback<Object*>& callback, Object* userData = nullptr);

		/// <summary>
		/// Executes arbitrary callback after Update and OnUpdate() events
		/// Note: Takes effect on the same frame; schedules from graphics synch point or queued callbacks will be executed on the next frame
		/// </summary>
		/// <typeparam name="CallbackType"> Arbitrary function that can be directly passed to the Callback<Object*>'s constructor </typeparam>
		/// <param name="callback"> Callback to execute </param>
		/// <param name="userData"> Arbitrary object to keep alive while the callback is queued (it will be passed back as the callback's argument) </param>
		template<typename CallbackType>
		inline void ExecuteAfterUpdate(const CallbackType& callback, Object* userData = nullptr) {
			ExecuteAfterUpdate(Callback<Object*>(callback), userData);
		}

		/// <summary>
		/// Stores arbitrary object as a part of the scene data
		/// Note: This is mostly useful to keep references alive, since there's no way to get the objects stored here.
		/// </summary>
		/// <param name="object"> Object to keep alive </param>
		void StoreDataObject(const Object* object);

		/// <summary>
		/// Removes arbitrary object stored as a part of the scene data
		/// Note: Use this to undo previous StoreDataObject() calls
		/// </summary>
		/// <param name="object"> Object to keep alive </param>
		void EraseDataObject(const Object* object);


	private:
		// Scene update cycle clock
		const Reference<Scene::Clock> m_time;

		// Current frame index
		std::atomic<uint64_t> m_frameIndex = 0;

		// Current update index
		std::atomic<uint64_t> m_updateIndex = 0u;

		// Main logger
		const Reference<OS::Logger> m_logger;

		// Main input module
		const Reference<OS::Input> m_input;

		// Asset database
		const Reference<AssetDatabase> m_assetDatabase;

		// Graphics context
		const Reference<Scene::GraphicsContext> m_graphics;

		// Physics context
		const Reference<Scene::PhysicsContext> m_physics;

		// Audio context
		const Reference<Scene::AudioContext> m_audio;
		
		// Update lock
		mutable std::recursive_mutex m_updateLock;

		// True, while logic/physics is updating
		std::atomic<bool> m_updating = false;

		// OnPreUpdate() event
		EventInstance<> m_onPreUpdate;

		// OnUpdate() event
		EventInstance<> m_onUpdate;

		// OnSynchOrUpdate() event
		EventInstance<> m_onSynchOrUpdate;

		// OnComponentCreated() event
		EventInstance<Component*> m_onComponentCreated;

		// Flushes any new/removed/enabled/disabled component
		void FlushComponentSets();

		// Flushes execute after update queue
		void FlushQueues();

		// Updates component
		void Update(float deltaTime);

		// Invoked by each component when it gets created
		void ComponentCreated(Component* component);

		// Invoked by each component when it gets destroyed
		void ComponentDestroyed(Component* component);

		// Invoked by each component when it gets enabled, disabled or it's parent changed
		void ComponentStateDirty(Component* component, bool parentHierarchyChanged);

		// Invoked when scene goes out of scope
		void Cleanup();

		// Constructor
		inline SceneContext(
			const Scene::CreateArgs& createArgs,
			Scene::GraphicsContext* graphics,
			Scene::PhysicsContext* physics,
			Scene::AudioContext* audio)
			: m_time([]() -> Reference<Scene::Clock> { Reference<Scene::Clock> clock = new Scene::Clock(); clock->ReleaseRef(); return clock; }())
			, m_logger(createArgs.logic.logger), m_input(createArgs.logic.input), m_assetDatabase(createArgs.logic.assetDatabase)
			, m_graphics(graphics), m_physics(physics), m_audio(audio) {}

		// Scene data that lives only while the scene itself is alive and well
		struct JIMARA_API Data : public virtual Object {
			Data(Scene::LogicContext* ctx);
			static Reference<Data> Create(
				Scene::CreateArgs& createArgs,
				Scene::GraphicsContext* graphics,
				Scene::PhysicsContext* physics, 
				Scene::AudioContext* audio);
			virtual void OnOutOfScope()const final override;
			void FlushComponentSet();
			void FlushComponentStates();
			void UpdateUpdatingComponents();

			const Reference<Scene::LogicContext> context;
			DelayedObjectSet<Component> allComponents;
			DelayedObjectSet<Component> enabledComponents;
			ObjectSet<UpdatingComponent> updatingComponents;

			std::unordered_set<Reference<Component>> dirtyParentChains;

			SynchronousActionQueue<> postUpdateActions;

			mutable std::recursive_mutex dataObjectLock;
			std::atomic<bool> dataObjectsDestroyed = false;
			mutable ObjectSet<const Object> dataObjects;

			mutable Reference<Component> rootObject;
		};
		Scene::DataWeakReference<Data> m_data;

		// Scene has to be able to create and update the context
		friend class Scene;

		// Component has to be able to inform the context that it has been created
		friend class Component;
	};
}
