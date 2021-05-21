#include "SceneContext.h"

namespace Jimara {
	SceneContext::SceneContext(AppContext* context, GraphicsContext* graphicsContext, const OS::Input* input)
		: m_context(context), m_graphicsContext(graphicsContext), m_input(input) {}

	AppContext* SceneContext::Context()const { return m_context; }

	OS::Logger* SceneContext::Log()const { return m_context->Log(); }

	GraphicsContext* SceneContext::Graphics()const { return m_graphicsContext; }

	const OS::Input* SceneContext::Input()const { return m_input; }
}
