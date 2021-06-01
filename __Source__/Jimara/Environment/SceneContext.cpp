#include "SceneContext.h"

namespace Jimara {
	SceneContext::SceneContext(AppContext* context, GraphicsContext* graphicsContext, PhysicsContext* physicsContext, const OS::Input* input)
		: m_context(context), m_graphicsContext(graphicsContext), m_physicsContext(physicsContext), m_input(input) {}

	AppContext* SceneContext::Context()const { return m_context; }

	OS::Logger* SceneContext::Log()const { return m_context->Log(); }

	GraphicsContext* SceneContext::Graphics()const { return m_graphicsContext; }

	PhysicsContext* SceneContext::Physics()const { return m_physicsContext; }

	const OS::Input* SceneContext::Input()const { return m_input; }
}
