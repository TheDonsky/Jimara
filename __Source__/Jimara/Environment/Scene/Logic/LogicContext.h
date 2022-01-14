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
	class SceneContext : public virtual Object {
	public:
		/// <summary> Scene update cycle clock </summary>
		inline Scene::Clock* Time()const { return m_time; }

		/// <summary> Index of the current update cycle since start </summary>
		inline uint64_t FrameIndex()const { return m_frameIndex.load(); }

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
		/// Root component 
		/// Notes:
		///		0. Any component will have this one on top of their parent heirarchy, unless the user creates some custom object that is not a 'normal' part of the heirarchy;
		///		1. Destroying the root object while the scene still exists will cause the entire tree to be deleted and the root being replaced.
		/// </summary>
		Reference<Component> RootObject()const;

		/// <summary>
		/// Component that updates each scene update cycle
		/// </summary>
		class UpdatingComponent : public virtual Component {
		protected:
			/// <summary> Updates component </summary>
			virtual void Update() = 0;

		private:
			// Only the Logic context is allowed to invoke Update() method
			friend class SceneContext;
		};

		/// <summary> Invoked right after UpdatingComponents get updated </summary>
		inline Event<>& OnUpdate() { return m_onUpdate; }

		/// <summary>
		/// Executes arbitrary callback after Update and OnUpdate() events
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

		// OnUpdate() event
		EventInstance<> m_onUpdate;

		// Flushes any new/removed/enabled/disabled component
		void FlushComponentSets();

		// Updates component
		void Update(float deltaTime);

		// Invoked by each component when it gets created
		void ComponentCreated(Component* component);

		// Invoked by each component when it gets destroyed
		void ComponentDestroyed(Component* component);

		// Invoked by each component when it gets enabled or disabled
		void ComponentEnabledStateDirty(Component* component);

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
		struct Data : public virtual Object {
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

			SynchronousActionQueue<> postUpdateActions;

			mutable std::mutex dataObjectLock;
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
