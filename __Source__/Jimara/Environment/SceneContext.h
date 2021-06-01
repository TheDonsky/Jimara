#pragma once
#include "AppContext.h"
#include "PhysicsContext.h"
#include "GraphicsContext/GraphicsContext.h"

namespace Jimara {
	class Component;

	class SceneContext : public virtual Object {
	public:
		SceneContext(AppContext* Context, GraphicsContext* graphicsContext, PhysicsContext* physicsContext, const OS::Input* input);

		AppContext* Context()const;

		OS::Logger* Log()const;

		GraphicsContext* Graphics()const;

		PhysicsContext* Physics()const;

		const OS::Input* Input()const;


	private:
		const Reference<AppContext> m_context;
		const Reference<GraphicsContext> m_graphicsContext;
		const Reference<PhysicsContext> m_physicsContext;
		const Reference<const OS::Input> m_input;

	protected:
		virtual void ComponentInstantiated(Component* component) = 0;

		friend class Component;
	};
}
