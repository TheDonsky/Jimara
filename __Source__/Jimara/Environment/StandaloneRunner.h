#pragma once
#include "../Core/JimaraApi.h"
#include "../OS/IO/Path.h"
#include "../Core/EnumClassBooleanOperands.h"


namespace Jimara {
	/// <summary> Standalone runner, containing the necessary boilerplate to run the game without editor </summary>
	class JIMARA_API StandaloneRunner {
	public:
		/// <summary> Runner flags </summary>
		enum class JIMARA_API Flags : uint32_t {
			/// <summary> Empty bitmask </summary>
			NONE = 0u,

			/// <summary> If set, this flag forces console window creation if it does not exist </summary>
			CREATE_CONSOLE_IF_NOT_PRESENT = (1 << 0),

			/// <summary> If set, this flag makes console visible if any initialization error occures </summary>
			SHOW_CONSOLE_ON_INITIALIZATION_ERRORS = (1 << 1),

			/// <summary> If set, this flag makes console visible if anything gets logged with LogLevel::LOG_INFO </summary>
			SHOW_CONSOLE_ON_LOG_INFOS = (1 << 2),

			/// <summary> If set, this flag makes console visible if anything gets logged with LogLevel::LOG_WARNING </summary>
			SHOW_CONSOLE_ON_LOG_WARNINGS = (1 << 3),

			/// <summary> If set, this flag makes console visible if anything gets logged with LogLevel::LOG_ERROR or LogLevel::LOG_FATAL </summary>
			SHOW_CONSOLE_ON_LOG_ERRORS = (1 << 4),

			/// <summary> If set, this flag will enable toggling console window on and off by clicking F10 button </summary>
			TOGGLE_CONSOLE_ON_F10 = (1 << 5),

			/// <summary> If set, this flag will cause the game window to start in full-screen mode </summary>
			FULLSCREEN_ON_START = (1 << 8),

			/// <summary> If set, this flag will enable toggling window full-screen mode on and off by clicking F10 button </summary>
			TOGGLE_FULLSCREEN_ON_F11 = (1 << 9),

			/// <summary> If set, this flag will lock cursor inside window right out of the gate (Input::SetCursorLockMode(OS::Input::CursorLock::LOCK_INSIDE)) </summary>
			LOCK_MOUSE_INSIDE_WINDOW_WHEN_FOCUSED = (1 << 16),

			/// <summary> If set, this flag will allow toggling cursor-lock-inside-window on and off by clicking F9 button </summary>
			TOGGLE_MOUSE_LOCK_INSIDE_WINDOW_ON_F9 = (1 << 17),

			/// <summary> If set, this flag will cause smooth framerate to be shown as a part of the window title </summary>
			SHOW_SMOOTH_FRAMERATE_IN_WINDOW_NAME = (1 << 24),

			/// <summary> If set, this flag will cause smooth frame-time to be shown as a part of the window title </summary>
			SHOW_SMOOTH_FRAMETIME_IN_WINDOW_NAME = (1 << 25),

			/// <summary> If set, MSAA will be turned-off on start in case the GPU is integrated </summary>
			TURN_MSAA_OFF_IF_GPU_IS_INTEGRATED = (1 << 28),

			/// <summary> If set, MSAA will be turned-off on start in case the GPU is discrete </summary>
			TURN_MSAA_OFF_IF_GPU_IS_DISCRETE = (1 << 29),


			/// <summary> Default flags </summary>
			DEFAULT = (
				CREATE_CONSOLE_IF_NOT_PRESENT |
				SHOW_CONSOLE_ON_INITIALIZATION_ERRORS |
				TOGGLE_CONSOLE_ON_F10 |
				FULLSCREEN_ON_START |
				TOGGLE_FULLSCREEN_ON_F11 |
				LOCK_MOUSE_INSIDE_WINDOW_WHEN_FOCUSED |
				TOGGLE_MOUSE_LOCK_INSIDE_WINDOW_ON_F9 |
				SHOW_SMOOTH_FRAMERATE_IN_WINDOW_NAME |
				SHOW_SMOOTH_FRAMETIME_IN_WINDOW_NAME |
				TURN_MSAA_OFF_IF_GPU_IS_INTEGRATED)
		};


		/// <summary> Arguments for running the game </summary>
		struct JIMARA_API Args {
			/// <summary> Name of the window </summary>
			std::string windowName = "Game";

			/// <summary> Path to the compiled SPIR-V directory (shader output path) </summary>
			OS::Path shaderPath = OS::Path("Game/Shaders/");

			/// <summary> Path to the game dll files </summary>
			OS::Path gameDLLPath = OS::Path("Game/");

			/// <summary> Path to the asset directory </summary>
			OS::Path assetDirectory = OS::Path("Assets/");

			/// <summary> Path to the main scene to load (relative to the asset directory) </summary>
			OS::Path mainScenePath = OS::Path("Scenes/Main.jimara");

			/// <summary> Runner flags </summary>
			Flags flags = Flags::DEFAULT;
		};


		/// <summary> Error codes </summary>
		enum class JIMARA_API Error : int {
			/// <summary> No error </summary>
			NONE = 0,

			/// <summary> Graphics::GraphicsInstance::Create() == nullptr </summary>
			GRAPHICS_INSTANCE_CREATION_FAILED = 1,

			/// <summary> Returned if no display-capable graphics devie was found </summary>
			COMPATIBLE_GRAPOHICS_DEVICE_NOT_FOUND = 2,

			/// <summary> Graphics::PhysicalDevice::CreateLogicalDevice() == nullptr </summary>
			GRAPHICS_DEVICE_CREATION_FAILED = 3,

			/// <summary> OS::Window::Create() == nullptr </summary>
			WINDOW_CREATION_FAILED = 4,

			/// <summary> Graphics::GraphicsInstance::CreateRenderSurface() == nullptr </summary>
			RENDER_SURFACE_CREATION_FAILED = 5,

			/// <summary> Graphics::GraphicsDevice::CreateRenderEngine() == nullptr </summary>
			RENDER_SURFACE_ENGINE_CREATION_FAILED = 6,

			/// <summary> FileSystemShaderLibrary::Create() == nullptr </summary>
			SHADER_LIBRARY_CREATION_FAILED = 7,

			/// <summary> Graphics::GraphicsDevice::CreateArrayBufferBindlessSet() == nullptr </summary>
			BINDLESS_BUFFER_SET_CREATION_FAILED = 8,

			/// <summary> Graphics::GraphicsDevice::CreateTextureSamplerBindlessSet() == nullptr </summary>
			BINDLESS_SAMPLER_SET_CREATION_FAILED = 9,

			/// <summary> Physics::PhysicsInstance::Create() == nullptr </summary>
			PHYSICS_API_CREATION_FAILED = 10,

			/// <summary> Audio::AudioInstance::Create() == nullptr </summary>
			AUDIO_API_CREATION_FAILED = 11,

			/// <summary> Audio::AudioInstance::DefaultDevice() == nullptr </summary>
			DEFAULT_AUDIO_DEVICE_NOT_FOUND = 12,

			/// <summary> Audio::PhysicalAudioDevice::CreateLogicalDevice() == nullptr  </summary>
			AUDIO_DEVICE_CREATION_FAILED = 13,

			/// <summary> FileSystemDatabase::Create() == nullptr </summary>
			ASSET_DATABASE_CREATION_FAILED = 14,

			/// <summary> OS::Window::CreateInputModule() == nullptr </summary>
			INPUT_MODULE_CREATION_FAILED = 15,

			/// <summary> Scene::Create() == nullptr </summary>
			SCENE_CREATION_FAILED = 16,

			/// <summary> Main scene (Args::mainScenePath) not found </summary>
			MAIN_SCENE_ASSET_NOT_FOUND = 17,

			/// <summary> Main scene (Args::mainScenePath) resource could not be loaded </summary>
			MAIN_SCENE_COULD_LOAD_FAILED = 18,

			/// <summary> Unknown error </summary>
			UNKNOWN = ~0
		};


		/// <summary>
		/// Performs necessary boilerplate to create context and runs the game
		/// <para/> This function retunrs once the window gets closed.
		/// </summary>
		/// <param name="args"> Arguments for running the game </param>
		/// <returns> Error code if something failed, Error::NONE otherwise. </returns>
		static Error RunGame(const Args& args);


	private:
		// Standalone runner is not a class that we can create an object of..
		inline StandaloneRunner() {}
	};

	// Define boolean operands for the flags:
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(StandaloneRunner::Flags);
}
