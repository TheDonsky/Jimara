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

	/// <summary>
	/// Main Scene class with full lifecycle control
	/// </summary>
	class Scene : public virtual Object {
	public:
		/// <summary> Scene logic context </summary>
		typedef SceneContext LogicContext;

		/// <summary> Scene sub-context for graphics-related routines and storage </summary>
		class GraphicsContext;

		/// <summary> Data necessary for the graphics context to be created </summary>
		struct GraphicsConstants;

		/// <summary> Scene sub-context for physics-related routines and storage </summary>
		class PhysicsContext;

		/// <summary> Scene sub-context for audio-related routines and storage </summary>
		class AudioContext;

		/// <summary> Simple clock for various scene contexts </summary>
		class Clock;

	public:
		/// <summary>
		/// Creates a new instance of a scene
		/// Note: Most arguments can be null, but it's highly adviced to provide each and every one of them for safety.
		/// </summary>
		/// <param name="inputModule"> Input module for the scene context </param>
		/// <param name="graphicsConfiguration"> Data for graphics context creation </param>
		/// <param name="physicsInstance"> Physics instance to use </param>
		/// <param name="audioDevice"> Audio device to use </param>
		/// <returns> New instance of a scene if successful, nullptr otherwise </returns>
		static Reference<Scene> Create(
			OS::Input* inputModule,
			GraphicsConstants* graphicsConfiguration,
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice);

		/// <summary> Scene logic context </summary>
		LogicContext* Context()const;

		/// <summary> Same as Context()->RootObject() </summary>
		Reference<Component> RootObject()const;

		/// <summary>
		/// Updates the scene
		/// </summary>
		/// <param name="deltaTime"> Elapsed time from the last update to simulate (Yep, the Scene has no internal clock) </param>
		void Update(float deltaTime);

	private:
		// Type registry for built-in types
		const Reference<const BuiltInTypeRegistrator> m_builtInTypeRegistry = BuiltInTypeRegistrator::Instance();

		// LogicContext::Data
		const Reference<Object> m_logicScene;

		// GraphicsContext::Data
		const Reference<Object> m_graphicsScene;

		// PhysicsContext::Data
		const Reference<Object> m_physicsScene;

		// AudioContext::Data
		const Reference<Object> m_audioScene;

		// Basic pointer and lock for data-context relationships
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

		// Constructor
		Scene(Reference<Object> logic, Reference<Object> graphics, Reference<Object> physics, Reference<Object> audio);

		// So far, SceneContext is not a nested class and it needs to access the internals...
		friend class SceneContext;
	};

#ifndef USE_REFACTORED_SCENE
}
#endif
}

#include "SceneClock.h"
#include "Logic/LogicContext.h"
#include "../../Components/Component.h"
