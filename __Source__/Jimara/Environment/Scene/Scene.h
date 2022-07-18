#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../../Data/TypeRegistration/TypeRegistartion.h"
#include "../../Data/AssetDatabase/AssetDatabase.h"
#include "../../OS/Input/Input.h"
#include "../../Graphics/GraphicsDevice.h"
#include "../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../Physics/PhysicsInstance.h"
#include "../../Audio/AudioDevice.h"
#include <mutex>


namespace Jimara {
	class Component;
	class SceneContext;

	/// <summary>
	/// Main Scene class with full lifecycle control
	/// </summary>
	class JIMARA_API Scene : public virtual Object {
	public:
		/// <summary> Data necessary for the scene to be created </summary>
		struct JIMARA_API CreateArgs {
			/// <summary> Data necessary for the logic context to be created </summary>
			struct {
				/// <summary> Main logger to use </summary>
				Reference<OS::Logger> logger;

				/// <summary> Scene input module </summary>
				Reference<OS::Input> input;

				/// <summary> Asset database to use </summary>
				Reference<AssetDatabase> assetDatabase;
			} logic;

			/// <summary> Data necessary for the graphics context to be created </summary>
			struct {
				/// <summary> Graphics device to use </summary>
				Reference<Graphics::GraphicsDevice> graphicsDevice;

				/// <summary> Shader loader </summary>
				Reference<Graphics::ShaderLoader> shaderLoader;

				/// <summary>
				/// Maximal number of in-flight command buffers that can be executing simultaneously
				/// Note: this has to be nonzero.
				/// </summary>
				size_t maxInFlightCommandBuffers = 3;

				struct {
					/// <summary> Bindless set of structured buffers (can be nullptr; only reason this is exposed here is to share the thing between scenes) </summary>
					Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> bindlessArrays;

					/// <summary> Bindless sets of texture samplers (can be nullptr; only reason this is exposed here is to share the thing between scenes) </summary>
					Reference<Graphics::BindlessSet<Graphics::TextureSampler>> bindlessSamplers;
				} bindlessResources;

				/// <summary> Number of threads graphics synch point can utilize (0 will default to all the available threads to the system) </summary>
				size_t synchPointThreadCount = std::thread::hardware_concurrency();

				/// <summary> Number of threads graphics render job can utilize (0 will default to half of all the available threads to the system) </summary>
				size_t renderThreadCount = (std::thread::hardware_concurrency() / 2);
			} graphics;

			/// <summary> Data necessary for the physics context to be created </summary>
			struct {
				/// <summary> Physics instance to use </summary>
				Reference<Physics::PhysicsInstance> physicsInstance;

				/// <summary> Number of simulation threads to be used for physics (0 will default to a quarter of available threads to the system) </summary>
				size_t simulationThreadCount = (std::thread::hardware_concurrency() / 4);
			} physics;

			/// <summary> Data necessary for the audio context to be created </summary>
			struct {
				/// <summary> Audio device to use </summary>
				Reference<Audio::AudioDevice> audioDevice;
			} audio;

			/// <summary> Tells, what to do if any of the configuration fields are missing </summary>
			enum class CreateMode : uint8_t {
				/// <summary> Auto-fills with fallback objects and logs warnings </summary>
				CREATE_DEFAULT_FIELDS_AND_WARN,

				/// <summary> Auto-fills with fallback objects silently </summary>
				CREATE_DEFAULT_FIELDS_AND_SUPRESS_WARNINGS,

				/// <summary> Error out if any of the relevant objects is missing and fails </summary>
				ERROR_ON_MISSING_FIELDS
			} createMode = CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN;
		};

		/// <summary> Scene logic context </summary>
		typedef SceneContext LogicContext;

		/// <summary> Scene sub-context for graphics-related routines and storage </summary>
		class GraphicsContext;

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
		/// <param name="createArgs"> Creation arguments </param>
		static Reference<Scene> Create(CreateArgs createArgs);

		/// <summary> Virtual destructor </summary>
		virtual ~Scene();

		/// <summary> Scene logic context </summary>
		LogicContext* Context()const;

		/// <summary> Same as Context()->RootObject() </summary>
		Reference<Component> RootObject()const;

		/// <summary>
		/// Updates the scene
		/// </summary>
		/// <param name="deltaTime"> Elapsed time from the last update to simulate (Yep, the Scene has no internal clock) </param>
		void Update(float deltaTime);

		/// <summary>
		/// Runs graphics synch point, input and renderers only; does not update logic/physics
		/// Note: Useful for editor outside the play mode and stuff like that.
		/// </summary>
		/// <param name="deltaTime"> Elapsed time for input update </param>
		void SynchAndRender(float deltaTime);

	private:
		// Type registry for built-in types
		const Reference<const BuiltInTypeRegistrator> m_builtInTypeRegistry = BuiltInTypeRegistrator::Instance();

		// AudioContext::Data
		const Reference<Object> m_audioScene;

		// PhysicsContext::Data
		const Reference<Object> m_physicsScene;

		// GraphicsContext::Data
		const Reference<Object> m_graphicsScene;

		// LogicContext::Data
		const Reference<Object> m_logicScene;

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
}

#include "SceneClock.h"
#include "Logic/LogicContext.h"
#include "../../Components/Component.h"
