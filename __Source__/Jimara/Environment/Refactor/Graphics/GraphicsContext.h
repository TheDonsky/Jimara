#pragma once
#include "../Scene.h"
#include "../SceneClock.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::GraphicsContext : public virtual Object {
	public:


	private:
		const Reference<Clock> m_time;
		const Reference<Graphics::GraphicsDevice> m_device;

		inline GraphicsContext(Graphics::GraphicsDevice* device)
			: m_time([]() -> Reference<Clock> { Reference<Clock> clock = new Clock(); clock->ReleaseRef(); return clock; }())
			, m_device(device) {}

		struct Data : public virtual Object {
			inline Data(Graphics::GraphicsDevice* device)
				: context([&]() -> Reference<GraphicsContext> {
				Reference<GraphicsContext> ctx = new GraphicsContext(device);
				ctx->ReleaseRef();
				return ctx;
					}()) {
				context->m_data = this;
			}

			inline virtual ~Data() {
				context->m_data = nullptr;
			}

			const Reference<GraphicsContext> context;
		};
		DataWeakReference<Data> m_data;

		friend class Scene;
	};
}
}
