#include "Scene.h"
#include "../../OS/Logging/StreamLogger.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {
	Reference<Scene> Scene::Create(
		OS::Input* inputModule,
		Graphics::GraphicsDevice* graphicsDevice,
		Physics::PhysicsInstance* physicsInstance,
		Audio::AudioDevice* audioDevice) {
		// Obtain logger reference:
		Reference<OS::Logger> logger = [&]() -> Reference<OS::Logger> {
			Reference<OS::Logger> result;
			if (graphicsDevice != nullptr)
				result = graphicsDevice->Log();
			if (result == nullptr) {
				if (physicsInstance != nullptr)
					result = physicsInstance->Log();
				if (result == nullptr && audioDevice != nullptr)
					result = audioDevice->APIInstance()->Log();
			}
			if (result == nullptr)
				return Object::Instantiate<OS::StreamLogger>();
			else return result;
		}();
		assert(logger != nullptr);

		// Create graphics context:
		Reference<GraphicsContext::Data> graphics = GraphicsContext::Data::Create(graphicsDevice, logger);
		if (graphics == nullptr) {
			logger->Error("Scene::Create - Failed to create scene graphics context!");
			return nullptr;
		}
		
		// Create physics context:
		Reference<Physics::PhysicsInstance> physicsInstanceReference = physicsInstance; // __TODO__: Create the instance if null
		if (physicsInstanceReference == nullptr) {
			logger->Error("Scene::Create - Null physics instance!");
			return nullptr;
		}
		Reference<PhysicsContext::Data> physics = Object::Instantiate<PhysicsContext::Data>(physicsInstanceReference);
		
		// Create audio context:
		Reference<AudioContext> audio = AudioContext::Create(audioDevice, logger);
		if (audio == nullptr) {
			logger->Error("Scene::Create - Failed to create audio context!");
			return nullptr;
		}
		
		// Create logic context and scene:
		Reference<OS::Input> input = inputModule; // __TODO__: Create a placeholder input module if null
		if (input == nullptr) {
			logger->Error("Scene::Create - Null input module!");
			return nullptr;
		}
		Reference<LogicContext::Data> logic = Object::Instantiate<LogicContext::Data>(logger, input, graphics->context, physics->context, audio);
		Reference<Scene> scene = new Scene(logic, graphics, physics, audio);
		scene->ReleaseRef();
		return scene;
	}

	Scene::Scene(Reference<Object> logic, Reference<Object> graphics, Reference<Object> physics, Reference<Object> audio)
		: m_logicScene(logic), m_graphicsScene(graphics), m_physicsScene(physics), m_audioScene(audio) {}

	Scene::LogicContext* Scene::Context()const {
		return dynamic_cast<Scene::LogicContext::Data*>(m_logicScene.operator->())->context;
	}

	void Scene::Update(float deltaTime) {
		LogicContext* context = Context();

		// Sync graphics and start rendering:
		context->Graphics()->Sync();
		context->Graphics()->StartRender();

		// Update logic and physics:
		{
			std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
			Clock* timer = context->Time();
			timer->Update(deltaTime);
			context->Physics()->SynchIfReady(timer->UnscaledDeltaTime(), timer->TimeScale());
			context->Update(deltaTime);
			context->FlushComponentSets();
		}

		// Finish rendering:
		context->Graphics()->SyncRender();
	}
}
}
