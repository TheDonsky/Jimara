#include "SceneContext.h"

namespace Jimara {
	SceneContext::SceneContext(AppContext* context, GraphicsContext* graphicsContext, PhysicsContext* physicsContext, const OS::Input* input, Audio::AudioScene* audioScene)
		: m_context(context), m_graphicsContext(graphicsContext), m_physicsContext(physicsContext), m_input(input), m_audioScene(audioScene) {}

	AppContext* SceneContext::Context()const { return m_context; }

	OS::Logger* SceneContext::Log()const { return m_context->Log(); }

	GraphicsContext* SceneContext::Graphics()const { return m_graphicsContext; }

	PhysicsContext* SceneContext::Physics()const { return m_physicsContext; }

	const OS::Input* SceneContext::Input()const { return m_input; }

	Audio::AudioScene* SceneContext::AudioScene()const { return m_audioScene; }
}
