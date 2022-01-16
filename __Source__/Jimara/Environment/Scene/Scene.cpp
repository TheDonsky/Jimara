#include "Scene.h"
#include "../../OS/Logging/StreamLogger.h"


namespace Jimara {
	Reference<Scene> Scene::Create(CreateArgs createArgs) {
		// Obtain logger reference:
		if (createArgs.logic.logger == nullptr) {
			Reference<OS::Logger>& result = createArgs.logic.logger;
			if (createArgs.graphics.graphicsDevice != nullptr)
				result = createArgs.graphics.graphicsDevice->Log();
			if (result == nullptr) {
				if (createArgs.physics.physicsInstance != nullptr)
					result = createArgs.physics.physicsInstance->Log();
				if (result == nullptr && createArgs.audio.audioDevice != nullptr)
					result = createArgs.audio.audioDevice->APIInstance()->Log();
			}
			if (result == nullptr)
				result = Object::Instantiate<OS::StreamLogger>();
			
			if (createArgs.createMode == CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				result->Warning("Scene::Create - Logger not provided; picked the default one!");
			else if (createArgs.createMode == CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				result->Error("Scene::Create - Logger not provided!");
				return nullptr;
			}
		}

		// Create graphics context:
		Reference<GraphicsContext::Data> graphics = GraphicsContext::Data::Create(createArgs);
		if (graphics == nullptr) {
			createArgs.logic.logger->Error("Scene::Create - Failed to create scene graphics context!");
			return nullptr;
		}
		
		// Create physics context:
		Reference<PhysicsContext::Data> physics = PhysicsContext::Data::Create(createArgs);
		if (physics == nullptr) {
			createArgs.logic.logger->Error("Scene::Create - Failed to create physics context!");
			return nullptr;
		}
		
		// Create audio context:
		Reference<AudioContext> audio = AudioContext::Create(createArgs);
		if (audio == nullptr) {
			createArgs.logic.logger->Error("Scene::Create - Failed to create audio context!");
			return nullptr;
		}
		
		// Create logic context and scene:
		Reference<LogicContext::Data> logic = LogicContext::Data::Create(createArgs, graphics->context, physics->context, audio);
		if (logic == nullptr) {
			createArgs.logic.logger->Error("Scene::Create - Failed to create logic context!");
			return nullptr;
		}

		// Create scene:
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
		: m_audioScene(audio), m_physicsScene(physics), m_graphicsScene(graphics), m_logicScene(logic) {}

	Scene::LogicContext* Scene::Context()const {
		return dynamic_cast<Scene::LogicContext::Data*>(m_logicScene.operator->())->context;
	}

	Reference<Component> Scene::RootObject()const {
		return Context()->RootObject();
	}

	void Scene::Update(float deltaTime) {
		LogicContext* context = Context();
		context->m_updating = true;

		// Sync graphics and Update logic and physics:
		{
			std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
			context->Graphics()->Sync(context);
			context->Graphics()->StartRender();

			Clock* timer = context->Time();
			timer->Update(deltaTime);
			context->m_input->Update(deltaTime);
			context->Physics()->SynchIfReady(timer->UnscaledDeltaTime(), timer->TimeScale(), context);
			context->Update(deltaTime);
			context->m_frameIndex++;
		}

		// Finish rendering:
		context->Graphics()->SyncRender();
		context->m_updating = false;
	}

	void Scene::SynchAndRender(float deltaTime) {
		LogicContext* context = Context();
		{
			std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
			context->Graphics()->Sync(context);
			context->Graphics()->StartRender();
			context->m_input->Update(deltaTime);
		}
		context->Graphics()->SyncRender();
	}
}
