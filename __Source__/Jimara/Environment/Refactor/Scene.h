#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../../Data/TypeRegistration/TypeRegistartion.h"
#include "../../OS/Input/Input.h"
#include "../../Graphics/GraphicsDevice.h"
#include "../../Physics/PhysicsInstance.h"
#include "../../Audio/AudioDevice.h"
#include <mutex>

#include "../SceneConfig.h"

namespace Jimara {
	class Component;

#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	class SceneContext;

	class Scene : public virtual Object {
	public:
		typedef SceneContext LogicContext;

		class GraphicsContext;

		struct GraphicsConstants;

		class PhysicsContext;

		class AudioContext;

		class Clock;

	public:
		static Reference<Scene> Create(
			OS::Input* inputModule,
			GraphicsConstants* graphicsConfiguration,
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice);

		LogicContext* Context()const;

		Reference<Component> RootObject()const;

		void Update(float deltaTime);

	private:
		const Reference<const BuiltInTypeRegistrator> m_builtInTypeRegistry = BuiltInTypeRegistrator::Instance();
		const Reference<Object> m_logicScene;
		const Reference<Object> m_graphicsScene;
		const Reference<Object> m_physicsScene;
		const Reference<Object> m_audioScene;

		template<typename DataType>
		struct DataWeakReference {
			mutable SpinLock lock;
			std::atomic<DataType*> data = nullptr;

			inline operator Reference<DataType>()const {
				std::unique_lock<SpinLock> dataLock(lock);
				Reference<DataType> value = data.load();
				return value;
			}
		};

		Scene(Reference<Object> logic, Reference<Object> graphics, Reference<Object> physics, Reference<Object> audio);

		friend class SceneContext;
	};

#ifndef USE_REFACTORED_SCENE
}
#endif
}

#include "SceneClock.h"
#include "Logic/LogicContext.h"
#include "../../Components/Component.h"

