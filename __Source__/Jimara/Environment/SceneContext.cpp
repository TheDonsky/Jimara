#include "SceneContext.h"

namespace Jimara {
	SceneContext::SceneContext(AppContext* context, GraphicsContext* graphicsContext)
		: m_context(context), m_graphicsContext(graphicsContext) {}

	AppContext* SceneContext::Context()const { return m_context; }

	OS::Logger* SceneContext::Log()const { return m_context->Log(); }

	GraphicsContext* SceneContext::Graphics()const { return m_graphicsContext; }
}
