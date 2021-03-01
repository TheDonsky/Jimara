#include "SceneContext.h"

namespace Jimara {
	SceneContext::SceneContext(AppContext* context)
		: m_context(context), m_graphicsPipelineSet(Object::Instantiate<Graphics::GraphicsObjectSet>()) {}

	AppContext* SceneContext::Context()const { return m_context; }

	OS::Logger* SceneContext::Log()const { return m_context->Log(); }

	Graphics::GraphicsDevice* SceneContext::GraphicsDevice()const { return m_context->GraphicsDevice(); }

	Graphics::GraphicsMeshCache* SceneContext::GraphicsMeshCache()const { return m_context->GraphicsMeshCache(); }

	Graphics::GraphicsObjectSet* SceneContext::GraphicsPipelineSet()const { return m_graphicsPipelineSet; }
}
