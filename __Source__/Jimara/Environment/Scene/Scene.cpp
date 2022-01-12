#include "Scene.h"
#include "../../OS/Logging/StreamLogger.h"


namespace Jimara {
	Reference<Scene> Scene::Create(
		OS::Input* inputModule,
		GraphicsConstants* graphicsConfiguration,
		Physics::PhysicsInstance* physicsInstance,
		Audio::AudioDevice* audioDevice) {
		// Obtain logger reference:
		Reference<OS::Logger> logger = [&]() -> Reference<OS::Logger> {
			Reference<OS::Logger> result;
			if (graphicsConfiguration != nullptr && graphicsConfiguration->graphicsDevice != nullptr)
				result = graphicsConfiguration->graphicsDevice->Log();
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
		Reference<GraphicsContext::Data> graphics = GraphicsContext::Data::Create(graphicsConfiguration, logger);
		if (graphics == nullptr) {
			logger->Error("Scene::Create - Failed to create scene graphics context!");
			return nullptr;
		}
		
		// Create physics context:
		Reference<PhysicsContext::Data> physics = Object::Instantiate<PhysicsContext::Data>(physicsInstance, logger);
		
		// Create audio context:
		Reference<AudioContext> audio = AudioContext::Create(audioDevice, logger);
		if (audio == nullptr) {
			logger->Error("Scene::Create - Failed to create audio context!");
			return nullptr;
		}
		
		// Create logic context and scene:
		Reference<LogicContext::Data> logic = Object::Instantiate<LogicContext::Data>(logger, inputModule, graphics->context, physics->context, audio);
		Reference<Scene> scene = new Scene(logic, graphics, physics, audio);
		scene->ReleaseRef();
		return scene;
	}

	Scene::~Scene() {
		LogicContext* context = Context();
		std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
		context->Cleanup();
	}

	Scene::Scene(Reference<Object> logic, Reference<Object> graphics, Reference<Object> physics, Reference<Object> audio)
		: m_logicScene(logic), m_graphicsScene(graphics), m_physicsScene(physics), m_audioScene(audio) {}

	Scene::LogicContext* Scene::Context()const {
		return dynamic_cast<Scene::LogicContext::Data*>(m_logicScene.operator->())->context;
	}

	Reference<Component> Scene::RootObject()const {
		return Context()->RootObject();
	}

	void Scene::Update(float deltaTime) {
		LogicContext* context = Context();

		// Sync graphics and Update logic and physics:
		{
			std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
			context->Graphics()->Sync();
			context->Graphics()->StartRender();

			Clock* timer = context->Time();
			timer->Update(deltaTime);
			context->m_input->Update();
			context->Physics()->SynchIfReady(timer->UnscaledDeltaTime(), timer->TimeScale());
			context->Update(deltaTime);
			context->FlushComponentSets();
			context->m_frameIndex++;
		}

		// Finish rendering:
		context->Graphics()->SyncRender();
	}
}
