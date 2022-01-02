#pragma once
#include "../Scene.h"
#include "../SceneClock.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::PhysicsContext : public virtual Object {
	public:

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
		};
		DataWeakReference<Data> m_data;

		friend class Scene;
	};
}
}
