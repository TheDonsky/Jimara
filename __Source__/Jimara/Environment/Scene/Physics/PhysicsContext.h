#pragma once
#include "../Scene.h"
#include "../SceneClock.h"
#include "../../../Core/Collections/ObjectSet.h"
#include "../../../Physics/PhysicsInstance.h"
#include "../../../Components/Component.h"


namespace Jimara {
	class Collider;
	/// <summary>
	/// Result of a raycast query
	/// </summary>
	struct RaycastHit {
		/// <summary> Collider, that got hit </summary>
		Collider* collider = nullptr;

		/// <summary> Hit point </summary>
		Vector3 point = Vector3(0.0f, 0.0f, 0.0f);

		/// <summary> Collider's normal at the hit point </summary>
		Vector3 normal = Vector3(0.0f, 0.0f, 0.0f);

		/// <summary> Distance, the query traveled for </summary>
		float distance = 0;
	};

	/// <summary>
	/// Scene sub-context for physics-related routines and storage
	/// </summary>
	class Scene::PhysicsContext : public virtual Object {
	public:
		/// <summary> Scene-wide gravity </summary>
		Vector3 Gravity()const;

		/// <summary>
		/// Sets scene-wide gravity
		/// </summary>
		/// <param name="value"> Gravity to use </param>
		void SetGravity(const Vector3& value);

		/// <summary>
		/// Tells, if two collider layers interact
		/// </summary>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <returns> True, if the colliders from given layers interact </returns>
		bool LayersInteract(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b)const;

		/// <summary>
		/// Tells, if two collider layers interact
		/// </summary>
		/// <typeparam name="LayerType"> Some enumeration, that can be cast to Physics::PhysicsCollider::Layer(ei uint8_t) </typeparam>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <returns> True, if the colliders from given layers interact </returns>
		template<typename LayerType>
		inline bool LayersInteract(const LayerType& a, const LayerType& b) {
			return LayersInteract(static_cast<Physics::PhysicsCollider::Layer>(a), static_cast<Physics::PhysicsCollider::Layer>(b));
		}

		/// <summary>
		/// Marks, whether or not the colliders on given layers should interact
		/// </summary>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <param name="enableIntaraction"> True, if the colliders from given layers should interact </param>
		void FilterLayerInteraction(Physics::PhysicsCollider::Layer a, Physics::PhysicsCollider::Layer b, bool enableIntaraction);

		/// <summary>
		/// Marks, whether or not the colliders on given layers should interact
		/// </summary>
		/// <typeparam name="LayerType"> Some enumeration, that can be cast to Physics::PhysicsCollider::Layer(ei uint8_t) </typeparam>
		/// <param name="a"> First layer </param>
		/// <param name="b"> Second layer </param>
		/// <param name="enableIntaraction"> True, if the colliders from given layers should interact </param>
		template<typename LayerType>
		inline void FilterLayerInteraction(const LayerType& a, const LayerType& b, bool enableIntaraction) {
			FilterLayerInteraction(static_cast<Physics::PhysicsCollider::Layer>(a), static_cast<Physics::PhysicsCollider::Layer>(b), enableIntaraction);
		}

		/// <summary>
		/// Adds a dynamic body to physics simulation and returns the instance 
		/// </summary>
		/// <param name="transform"> Transformation matrix (without scale; rotation&translation only) </param>
		/// <param name="enabled"> If true, the body will start off enabled </param>
		/// <returns> New dynamic body inside the physics representation of the scene </returns>
		Reference<Physics::DynamicBody> AddRigidBody(const Matrix4& transform, bool enabled = true);

		/// <summary>
		/// Adds a static body to physics simulation and returns the instance
		/// </summary>
		/// <param name="transform"> Transformation matrix (without scale; rotation&translation only) </param>
		/// <param name="enabled"> If true, the body will start off enabled </param>
		/// <returns> New static body inside the physics representation of the scene </returns>
		Reference<Physics::StaticBody> AddStaticBody(const Matrix4& transform, bool enabled = true);

		/// <summary>
		/// Casts a ray into the scene and reports what it manages to hit
		/// </summary>
		/// <param name="origin"> Ray origin </param>
		/// <param name="direction"> Ray direction </param>
		/// <param name="maxDistance"> Max distance, the ray is allowed to travel </param>
		/// <param name="onHitFound"> If the ray hits something, this callback will be invoked with the hit info </param>
		/// <param name="layerMask"> Layer mask, containing the set of layers, we are interested in (defaults to all layers) </param>
		/// <param name="flags"> Query flags for high level query options </param>
		/// <param name="preFilter"> Custom filtering function, that lets us ignore colliders before reporting hits (Optionally invoked after layer check) </param>
		/// <param name="postFilter"> Custom filtering function, that lets us ignore hits before reporting in onHitFound (Optionally invoked after preFilter) </param>
		/// <returns> Number of reported RaycastHit-s </returns>
		size_t Raycast(const Vector3& origin, const Vector3& direction, float maxDistance
			, const Callback<const RaycastHit&>& onHitFound
			, const Physics::PhysicsCollider::LayerMask& layerMask = Physics::PhysicsCollider::LayerMask::All(), Physics::PhysicsScene::QueryFlags flags = 0
			, const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>* preFilter = nullptr
			, const Function<Physics::PhysicsScene::QueryFilterFlag, const RaycastHit&>* postFilter = nullptr)const;

		/// <summary> Physics API instance </summary>
		Physics::PhysicsInstance* APIInstance()const;

		/// <summary> Physics update rate per simulated second (not effected by logic time scale; naturally, not the same as the framerate or logic update rate) </summary>
		float UpdateRate()const;

		/// <summary>
		/// Sets physics upodate rate (numbers greater than the framerate or logic update rate will likely fail to hit the mark)
		/// </summary>
		/// <param name="rate"> Update rate </param>
		void SetUpdateRate(float rate);

		/// <summary> 
		/// Physics update clock 
		/// Notes: 
		///		0. Physics clock depends on the LogicContext time scale, as well as it's own. 
		///		Think of it like "fast/slow motion" with LogicContext and fast/slow physics simulation relative to the rest of the scene;
		///		1. Physics clock will try to 'tick' at a relatively constant rate, but do not overrely on it being one and the same each frame, because it's just not happening.
		/// </summary>
		inline Clock* Time()const { return m_time; }

		/// <summary>
		/// If a component needs to do some work right before each physics synch point, this is the interface to implement
		/// </summary>
		class PrePhysicsSynchUpdatingComponent : public virtual Component {
		public:
			/// <summary> Invoked by the environment right before each physics synch point </summary>
			virtual void PrePhysicsSynch() = 0;
		};

		/// <summary> Invoked, after physics simulation, right before PostPhysicsSynch() (but after PrePhysicsSynch() and the physics sync point) </summary>
		Event<>& OnPhysicsSynch();

		/// <summary>
		/// If a component needs to do some work right after each physics synch point, this is the interface to implement
		/// </summary>
		class PostPhysicsSynchUpdatingComponent : public virtual Component {
		public:
			/// <summary> Invoked by the environment right after each physics synch point </summary>
			virtual void PostPhysicsSynch() = 0;
		};

	private:
		// Timer
		const Reference<Clock> m_time;

		// Underlying physics scene
		const Reference<Physics::PhysicsScene> m_scene;

		// Target update rate
		std::atomic<float> m_updateRate = 60.0f;

		// Invoked after synching the physics simulation
		EventInstance<> m_onPostPhysicsSynch;

		// Recorded elapsed time since the last update
		std::atomic<float> m_elapsed = 0.0f;

		// Recorded scaled elapsed time since the last update
		std::atomic<float> m_scaledElapsed = 0.0f;

		// Synchronizes physics if 
		void SynchIfReady(float deltaTime, float timeScale);

		// Constructor
		inline PhysicsContext(Physics::PhysicsInstance* instance)
			: m_time([]() -> Reference<Clock> { Reference<Clock> clock = new Clock(); clock->ReleaseRef(); return clock; }())
			, m_scene(instance->CreateScene(std::thread::hardware_concurrency() / 4)) {
			m_scene->SimulateAsynch(0.01f);
		}

		// Data (to avoid having strong references to the components)
		struct Data : public virtual Object {
			Data(Physics::PhysicsInstance* instance, OS::Logger* logger);
			virtual void OnOutOfScope()const final override;
			void ComponentEnabled(Component* component);
			void ComponentDisabled(Component* component);

			const Reference<PhysicsContext> context;
			ObjectSet<PrePhysicsSynchUpdatingComponent> prePhysicsSynchUpdaters;
			ObjectSet<PostPhysicsSynchUpdatingComponent> postPhysicsSynchUpdaters;
		};
		DataWeakReference<Data> m_data;

		// Only scene can manage the lifecycle
		friend class Scene;

		// Only logic context can add/remove component records
		friend class SceneContext;
	};

	typedef Scene::PhysicsContext PhysicsContext;
}
