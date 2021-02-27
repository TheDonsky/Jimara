#include "Scene.h"


namespace Jimara {
	Scene::Scene(AppContext* context)
		: m_context(context), m_graphicsPipelineSet(Object::Instantiate<Graphics::GraphicsObjectSet>()) {}

	AppContext* Scene::Context()const { return m_context; }

	OS::Logger* Scene::Log()const { return m_context->Log(); }

	Graphics::GraphicsDevice* Scene::GraphicsDevice()const { return m_context->GraphicsDevice(); }

	Graphics::GraphicsObjectSet* Scene::GraphicsPipelineSet()const { return m_graphicsPipelineSet; }
}
