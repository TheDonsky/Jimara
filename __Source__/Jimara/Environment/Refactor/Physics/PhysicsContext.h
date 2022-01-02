#pragma once
#include "../Scene.h"
#include "../SceneClock.h"
#include "../../../Core/Collections/ObjectSet.h"
#include "../../../Components/Component.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::PhysicsContext : public virtual Object {
	public:
		/// <summary>
		/// If a component needs to do some work right before each physics synch point, this is the interface to implement
		/// </summary>
		class PrePhysicsSynchUpdatingComponent : public virtual Component {
		public:
			/// <summary> Invoked by the environment right before each physics synch point </summary>
			virtual void PrePhysicsSynch() = 0;
		};

		/// <summary>
		/// If a component needs to do some work right after each physics synch point, this is the interface to implement
		/// </summary>
		class PostPhysicsSynchUpdatingComponent : public virtual Component {
		public:
			/// <summary> Invoked by the environment right after each physics synch point </summary>
			virtual void PostPhysicsSynch() = 0;
		};

	private:
		const Reference<Clock> m_time;
		const Reference<Physics::PhysicsScene> m_scene;

		inline PhysicsContext(Physics::PhysicsInstance* instance)
			: m_time([]() -> Reference<Clock> { Reference<Clock> clock = new Clock(); clock->ReleaseRef(); return clock; }())
			, m_scene(instance->CreateScene(std::thread::hardware_concurrency() / 4)) {}

		struct Data : public virtual Object {
			inline Data(Physics::PhysicsInstance* instance)
				: context([&]() -> Reference<PhysicsContext> {
				Reference<PhysicsContext> ctx = new PhysicsContext(instance);
				ctx->ReleaseRef();
				return ctx;
					}()) {
				context->m_data = this;
			}

			inline virtual ~Data() {
				context->m_data = nullptr;
			}

			const Reference<PhysicsContext> context;
			ObjectSet<PrePhysicsSynchUpdatingComponent> prePhysicsSynchUpdaters;
			ObjectSet<PostPhysicsSynchUpdatingComponent> postPhysicsSynchUpdaters;
		};
		DataWeakReference<Data> m_data;

		friend class Scene;
		friend class LogicContext;
	};
}
}
