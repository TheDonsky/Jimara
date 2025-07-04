#include "StandaloneRunner.h"
#include "../Core/TypeRegistration/TypeRegistration.h"
#include "../OS/Logging/StreamLogger.h"
#include "../OS/System/DynamicLibrary.h"
#include "../Environment/Scene/Scene.h"
#include "../Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"
#include "../Data/ComponentHierarchySpowner.h"
#include "../Environment/Rendering/RenderStack.h"
#include "../Core/Stopwatch.h"
#ifdef _WIN32
#include <Windows.h>
#endif


namespace Jimara {
	StandaloneRunner::Error StandaloneRunner::RunGame(const Args& args) {
		class Logger : public virtual OS::StreamLogger {
		private:
			const Flags m_flags;

		public:
			inline Logger(Flags flags) : m_flags(flags) {}

			inline virtual ~Logger() {};

		protected:
			inline virtual void Log(const LogInfo& info) override {
#ifdef _WIN32
				if (((m_flags & Flags::SHOW_CONSOLE_ON_LOG_INFOS) != Flags::NONE && info.level == OS::Logger::LogLevel::LOG_INFO) ||
					((m_flags & Flags::SHOW_CONSOLE_ON_LOG_WARNINGS) != Flags::NONE && info.level == OS::Logger::LogLevel::LOG_WARNING) ||
					((m_flags & Flags::SHOW_CONSOLE_ON_LOG_ERRORS) != Flags::NONE && info.level >= OS::Logger::LogLevel::LOG_ERROR)) {
					HWND wnd = ::GetConsoleWindow();
					if (wnd != static_cast<HWND>(0u))
						::ShowWindow(wnd, SW_SHOW);
				}
#endif
				OS::StreamLogger::Log(info);
			}
		};

#ifdef _WIN32
		{
			HWND wnd = ::GetConsoleWindow();
			if (wnd == static_cast<HWND>(0u)) {
				AllocConsole();
				::ShowWindow(::GetConsoleWindow(), SW_HIDE);
				FILE* fp;
				freopen_s(&fp, "CONOUT$", "w", stdout);
			}
		}
#endif

		const Reference<OS::Logger> logger = Object::Instantiate<Logger>(args.flags);
		auto error = [&](Error code, const auto&... message) -> Error {
#ifdef _WIN32
			HWND wnd = ::GetConsoleWindow();
			if (wnd != static_cast<HWND>(0u))
				::ShowWindow(wnd, SW_SHOW);
#endif
			logger->Error(message...);
			return code;
			};

		const Reference<BuiltInTypeRegistrator> builtInTypes = BuiltInTypeRegistrator::Instance();

		std::vector<Reference<OS::DynamicLibrary>> gameCode;
		OS::Path::IterateDirectory(args.gameDLLPath, [&](const OS::Path& path) {
			std::string extension = path.extension().string();
			for (size_t i = 0u; i < extension.length(); i++)
				extension[i] = std::toupper(extension[i]);
			if (extension != ".DLL")
				return true;
			const Reference<OS::DynamicLibrary> library = OS::DynamicLibrary::Load(path, nullptr);
			if (library != nullptr)
				gameCode.push_back(library);
			return true;
			});

		const Reference<const Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>();

		const Reference<Graphics::GraphicsInstance> graphicsAPI = Graphics::GraphicsInstance::Create(logger, appInfo);
		if (graphicsAPI == nullptr)
			return error(Error::GRAPHICS_INSTANCE_CREATION_FAILED, "Graphics instance could not be created!");

		const Reference<OS::Window> window = Jimara::OS::Window::Create(logger, args.windowName);
		if (window == nullptr)
			return error(Error::WINDOW_CREATION_FAILED, "Window could not be created!");
		else window->SetFullscreen((args.flags & Flags::FULLSCREEN_ON_START) != Flags::NONE);

		const Reference<Graphics::RenderSurface> surface = graphicsAPI->CreateRenderSurface(window);
		if (surface == nullptr)
			return error(Error::RENDER_SURFACE_CREATION_FAILED, "Render surface could not be created!");

		Graphics::PhysicalDevice* const physicalGPU = surface->PrefferedDevice();
		if (physicalGPU == nullptr)
			return error(Error::COMPATIBLE_GRAPOHICS_DEVICE_NOT_FOUND, "Compatible device not found!");

		const Reference<Graphics::GraphicsDevice> graphicsDevice = physicalGPU->CreateLogicalDevice();
		if (graphicsDevice == nullptr)
			return error(Error::GRAPHICS_DEVICE_CREATION_FAILED, "Graphics device could not be created!");

		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> bindlessBuffers = graphicsDevice->CreateArrayBufferBindlessSet();
		if (bindlessBuffers == nullptr)
			return error(Error::BINDLESS_BUFFER_SET_CREATION_FAILED, "Bindless buffer set could not be created!");

		const Reference<Graphics::BindlessSet<Graphics::TextureSampler>> bindlessSamplers = graphicsDevice->CreateTextureSamplerBindlessSet();
		if (bindlessSamplers == nullptr)
			return error(Error::BINDLESS_SAMPLER_SET_CREATION_FAILED, "Bindless texture sampler set could not be created!");

		const Reference<Graphics::RenderEngine> surfaceRenderEngine = graphicsDevice->CreateRenderEngine(surface);
		if (surfaceRenderEngine == nullptr)
			return error(Error::RENDER_SURFACE_ENGINE_CREATION_FAILED, "Surface render engine could not be created!");

		const Reference<ShaderLibrary> shaderLibrary = FileSystemShaderLibrary::Create(args.shaderPath, logger);
		if (shaderLibrary == nullptr)
			return error(Error::SHADER_LIBRARY_CREATION_FAILED, "Shader library could not be created!");

		const Reference<Physics::PhysicsInstance> physicsAPI = Physics::PhysicsInstance::Create(logger);
		if (physicsAPI == nullptr)
			return error(Error::PHYSICS_API_CREATION_FAILED, "Physics API could not be created!");

		const Reference<Audio::AudioInstance> audioAPI = Audio::AudioInstance::Create(logger);
		if (audioAPI == nullptr)
			return error(Error::AUDIO_API_CREATION_FAILED, "Audio API could not be created!");

		const Reference<Audio::PhysicalAudioDevice> physicalAudioDevice = audioAPI->DefaultDevice();
		if (physicalAudioDevice == nullptr)
			return error(Error::DEFAULT_AUDIO_DEVICE_NOT_FOUND, "Default audio device could not found!");

		const Reference<Audio::AudioDevice> audioDevice = physicalAudioDevice->CreateLogicalDevice();
		if (audioDevice == nullptr)
			return error(Error::AUDIO_DEVICE_CREATION_FAILED, "Audio device could not be created!");

		FileSystemDatabase::CreateArgs databaseCreateArgs = {};
		{
			databaseCreateArgs.logger = logger;
			databaseCreateArgs.graphicsDevice = graphicsDevice;
			databaseCreateArgs.bindlessBuffers = bindlessBuffers;
			databaseCreateArgs.bindlessSamplers = bindlessSamplers;
			databaseCreateArgs.shaderLibrary = shaderLibrary;
			databaseCreateArgs.physicsInstance = physicsAPI;
			databaseCreateArgs.audioDevice = audioDevice;
			databaseCreateArgs.assetDirectory = OS::Path(args.assetDirectory);
			databaseCreateArgs.previousImportDataCache = OS::Path("JimaraDatabaseCache.json");
		}
		const Reference<FileSystemDatabase> assetDatabase = FileSystemDatabase::Create(databaseCreateArgs);
		if (assetDatabase == nullptr)
			return error(Error::ASSET_DATABASE_CREATION_FAILED, "Asset database could not be created!");

		const Reference<OS::Input> inputModule = window->CreateInputModule();
		if (inputModule == nullptr)
			return error(Error::INPUT_MODULE_CREATION_FAILED, "Input module could not be created!");
		else if ((args.flags & Flags::LOCK_MOUSE_INSIDE_WINDOW_WHEN_FOCUSED) != Flags::NONE)
			inputModule->SetCursorLockMode(OS::Input::CursorLock::LOCK_INSIDE);

		Scene::CreateArgs sceneCreateArgs = {};
		{
			sceneCreateArgs.logic.logger = logger;
			sceneCreateArgs.logic.input = inputModule;
			sceneCreateArgs.logic.assetDatabase = assetDatabase;
			sceneCreateArgs.graphics.graphicsDevice = graphicsDevice;
			sceneCreateArgs.graphics.shaderLibrary = shaderLibrary;
			sceneCreateArgs.graphics.bindlessResources.bindlessArrays = bindlessBuffers;
			sceneCreateArgs.graphics.bindlessResources.bindlessSamplers = bindlessSamplers;
			sceneCreateArgs.physics.physicsInstance = physicsAPI;
			sceneCreateArgs.audio.audioDevice = audioDevice;
		}
		const Reference<Scene> scene = Scene::Create(sceneCreateArgs);
		if (scene == nullptr)
			return error(Error::SCENE_CREATION_FAILED, "Scene could not be created!");

		{
			Reference<Asset> mainSceneAsset;
			assetDatabase->GetAssetsFromFile<ComponentHierarchySpowner>(
				args.assetDirectory / args.mainScenePath,
				[&](const FileSystemDatabase::AssetInformation& assetInfo) {
					mainSceneAsset = assetInfo.AssetRecord();
				});
			if (mainSceneAsset == nullptr)
				return error(Error::MAIN_SCENE_ASSET_NOT_FOUND, "Main scene could not be found!");
			const Reference<ComponentHierarchySpowner> mainScene = mainSceneAsset->LoadAs<ComponentHierarchySpowner>();
			if (mainScene == nullptr)
				return error(Error::MAIN_SCENE_COULD_LOAD_FAILED, "Main scene could not be loaded!");
			mainScene->SpownHierarchy(scene->RootObject());
		}

		struct ImageRenderer : public virtual Graphics::ImageRenderer {
			const Reference<RenderStack> renderStack;

			inline ImageRenderer(SceneContext* context) : renderStack(RenderStack::Main(context)) {}

			virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override { return engineInfo; }

			virtual void Render(Object* engineData, Graphics::InFlightBufferInfo bufferInfo) override {
				Graphics::RenderEngineInfo* engineInfo = dynamic_cast<Graphics::RenderEngineInfo*>(engineData);
				const Reference<RenderImages> images = (renderStack != nullptr) ? renderStack->Images() : nullptr;
				const RenderImages::Image* const image = (images != nullptr) ? images->GetImage(RenderImages::MainColor()) : nullptr;
				if (image != nullptr && image->Resolve() != nullptr)
					engineInfo->Image(bufferInfo)->Blit(bufferInfo, image->Resolve()->TargetTexture());
				renderStack->SetResolution(engineInfo->ImageSize());
			}
		};
		const Reference<ImageRenderer> renderer = Object::Instantiate<ImageRenderer>(scene->Context());
		surfaceRenderEngine->AddRenderer(renderer);


		Stopwatch frameTimer;
		float smoothFrameTime = 0.0f;
		auto updateScene = [&](auto*) {
			// Frame time:
			const float frameTime = frameTimer.Reset();
			smoothFrameTime = Math::Lerp(smoothFrameTime, frameTime, 0.01f);

			// Update:
			scene->Update(frameTime);
			surfaceRenderEngine->Update();

			// Name:
			if ((args.flags & (Flags::SHOW_SMOOTH_FRAMERATE_IN_WINDOW_NAME | Flags::SHOW_SMOOTH_FRAMETIME_IN_WINDOW_NAME)) != Flags::NONE) {
				std::stringstream name_stream;
				name_stream << "Game ";
				std::string startSym = "[";
				if ((args.flags & Flags::SHOW_SMOOTH_FRAMERATE_IN_WINDOW_NAME) != Flags::NONE) {
					name_stream << startSym << std::setprecision(4u) << (1.0f / smoothFrameTime) << " fps";
					startSym = "; ";
				}
				if ((args.flags & Flags::SHOW_SMOOTH_FRAMETIME_IN_WINDOW_NAME) != Flags::NONE) {
					name_stream << startSym << std::setprecision(4u) << (smoothFrameTime * 1000.0f) << " ms";
					startSym = "; ";
				}
				if (startSym != "[")
					name_stream << "]";
				window->SetName(name_stream.str());
			}

			// Fullscreen:
			if (((args.flags & Flags::TOGGLE_FULLSCREEN_ON_F11) != Flags::NONE) && inputModule->KeyDown(OS::Input::KeyCode::F11))
				window->SetFullscreen(!window->IsFullscreen());

			// Cursor-Lock:
			if (((args.flags & Flags::TOGGLE_MOUSE_LOCK_INSIDE_WINDOW_ON_F9) != Flags::NONE) && inputModule->KeyDown(OS::Input::KeyCode::F9))
				inputModule->SetCursorLockMode((inputModule->CursorLockMode() == OS::Input::CursorLock::NONE)
					? OS::Input::CursorLock::LOCK_INSIDE
					: OS::Input::CursorLock::NONE);

#ifdef _WIN32
			// Console:
			if (((args.flags & Flags::TOGGLE_CONSOLE_ON_F10) != Flags::NONE) && inputModule->KeyDown(OS::Input::KeyCode::F10)) {
				HWND wnd = ::GetConsoleWindow();
				if (wnd != static_cast<HWND>(0u))
					::ShowWindow(wnd, ::IsWindowVisible(wnd) ? SW_HIDE : SW_SHOW);
			}
#endif
			};
		window->OnUpdate() += Callback<OS::Window*>::FromCall(&updateScene);

		window->WaitTillClosed();
		return Error::NONE;
	}
}
