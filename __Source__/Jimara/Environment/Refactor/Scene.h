#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../../Data/TypeRegistration/TypeRegistartion.h"
#include "../../OS/Input/Input.h"
#include "../../Graphics/GraphicsDevice.h"
#include "../../Physics/PhysicsInstance.h"
#include "../../Audio/AudioDevice.h"
#include <mutex>


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene : public virtual Object {
	public:
		class LogicContext;

		class GraphicsContext;

		class PhysicsContext;

		class AudioContext;

		class Clock;

	public:
		Reference<Scene> Create(
			OS::Input* inputModule,
			Graphics::GraphicsDevice* graphicsDevice,
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice);

		LogicContext* Context()const;

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
	};
}

template<> inline void TypeIdDetails::GetParentTypesOf<Refactor_TMP_Namespace::Scene>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}

#include "SceneClock.h"
#include "Logic/LogicContext.h"

