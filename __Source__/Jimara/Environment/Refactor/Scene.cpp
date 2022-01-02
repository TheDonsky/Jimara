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
		Reference<Graphics::GraphicsDevice> graphicsDeviceReference = graphicsDevice; // __TODO__: Create the device if null
		if (graphicsDeviceReference == nullptr) {
			logger->Error("Scene::Create - Null graphics device!");
			return nullptr;
		}
		Reference<GraphicsContext::Data> graphics = Object::Instantiate<GraphicsContext::Data>(graphicsDeviceReference);
		
		// Create physics context:
		Reference<Physics::PhysicsInstance> physicsInstanceReference = physicsInstance; // __TODO__: Create the instance if null
		if (physicsInstanceReference == nullptr) {
			logger->Error("Scene::Create - Null physics instance!");
			return nullptr;
		}
		Reference<PhysicsContext::Data> physics = Object::Instantiate<PhysicsContext::Data>(physicsInstanceReference);
		
		// Create audio context:
		Reference<Audio::AudioDevice> audioDeviceReference = audioDevice; // __TODO__: Create the device if null
		if (audioDeviceReference == nullptr) {
			logger->Error("Scene::Create - Null audio device!");
			return nullptr;
		}
		Reference<AudioContext> audio = AudioContext::Create(audioDeviceReference);
		
		// Create logic context and scene:
		Reference<OS::Input> input = inputModule; // __TODO__: Create a placeholder input module if null
		if (input == nullptr) {
			logger->Error("Scene::Create - Null input module!");
			return nullptr;
		}
		Reference<LogicContext::Data> logic = Object::Instantiate<LogicContext::Data>(input, graphics->context, physics->context, audio);
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
		// __TODO__: Synch graphics!

		// __TODO__: Let graphics run it's render threads in parallel!

		// __TODO__: Synch physics!

		// __TODO__: Let physics run it's substeps in parallel!

		Context()->Update(deltaTime);

		// __TODO__: (Maybe) Synch graphics render threads for safety!
		
		// __NOT_TODO__: Ignore synching physics threads to maintain optimal performance!
	}
}
}
