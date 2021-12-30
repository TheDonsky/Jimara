#include "JimaraEditor.h"
#include "../GUI/ImGuiRenderer.h"
#include "../GUI/Utils/DrawMenuAction.h"
#include "../__Generated__/JIMARA_EDITOR_LIGHT_IDENTIFIERS.h"
#include <OS/Logging/StreamLogger.h>
#include <Core/Stopwatch.h>
#include <map>

namespace Jimara {
	namespace Editor {
		AppContext* EditorContext::ApplicationContext()const { return m_appContext; }

		Graphics::ShaderLoader* EditorContext::ShaderBinaryLoader()const { return m_shaderLoader; }

		OS::Input* EditorContext::InputModule()const { return m_inputModule; }

		EditorContext::SceneLightTypes EditorContext::LightTypes()const { 
			EditorContext::SceneLightTypes types = {};
			types.lightTypeIds = &LightRegistry::JIMARA_EDITOR_LIGHT_IDENTIFIERS.typeIds;
			types.perLightDataSize = LightRegistry::JIMARA_EDITOR_LIGHT_IDENTIFIERS.perLightDataSize;
			return types;
		}

		LightingModel* EditorContext::DefaultLightingModel()const { return ForwardLightingModel::Instance(); }

		void EditorContext::AddRenderJob(JobSystem::Job* job) { 
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Add(job);
		}

		void EditorContext::RemoveRenderJob(JobSystem::Job* job) {
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Remove(job);
		}

		EditorContext::EditorContext(AppContext* appContext, Graphics::ShaderLoader* shaderLoader, OS::Input* inputModule, FileSystemDatabase* database)
			: m_appContext(appContext), m_shaderLoader(shaderLoader), m_inputModule(inputModule), m_fileSystemDB(database) { }
		
		Reference<EditorScene> EditorContext::GetScene()const {
			std::unique_lock<SpinLock> lock(m_editorLock);
			return m_editor != nullptr ? m_editor->m_scene : nullptr;
		}

		void EditorContext::SetScene(EditorScene* scene) {
			Reference<EditorScene> newScene;
			{
				std::unique_lock<SpinLock> lock(m_editorLock);
				if (m_editor != nullptr) {
					if (m_editor->m_scene == scene) return;
					m_editor->m_scene = scene;
					newScene = scene;
				}
				else return;
			}
			m_onSceneChanged(newScene, this);
		}

		Event<Reference<EditorScene>, const EditorContext*>& EditorContext::OnSceneChanged()const { return m_onSceneChanged; }


		namespace {
			class JimaraEditorRenderer : public virtual Graphics::ImageRenderer, public virtual JobSystem::Job {
			private:
				const Reference<EditorContext> m_editorContext;
				const Reference<ImGuiDeviceContext> m_deviceContext;
				const Callback<> m_executeRenderJobs;

			public:
				inline JimaraEditorRenderer(EditorContext* editorContext, ImGuiDeviceContext* deviceContext, const Callback<>& executeRenderJobs)
					: m_editorContext(editorContext), m_deviceContext(deviceContext), m_executeRenderJobs(executeRenderJobs) {}

				// Graphics::ImageRenderer:
				inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
					const Reference<ImGuiRenderer> renderer = m_deviceContext->CreateRenderer(engineInfo);
					if (renderer == nullptr)
						m_deviceContext->GraphicsDevice()->Log()->Error("JimaraEditorRenderer::CreateEngineData - Failed to create ImGuiRenderer!");
					else renderer->AddRenderJob(this);
					return renderer;
				}

				inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) override {
					ImGuiRenderer* renderer = dynamic_cast<ImGuiRenderer*>(engineData);
					if (renderer != nullptr) renderer->Render(bufferInfo);
					else m_deviceContext->GraphicsDevice()->Log()->Error("JimaraEditorRenderer::Render - Invalid engine data!");
				}


				// JobSystem::Job:
				virtual void Execute() override {
					ImGui::DockSpaceOverViewport();
					ImGui::BeginMainMenuBar();
					EditorMainMenuAction::RegistryEntry::GetAll([&](const EditorMainMenuAction* action) {
						if (DrawMenuAction(action->MenuPath(), action))
							action->Execute(m_editorContext);
						});
					ImGui::EndMainMenuBar();
					m_executeRenderJobs();
					static bool show_demo_window = true;
					ImGui::ShowDemoWindow(&show_demo_window);
				}

				virtual void CollectDependencies(Callback<Job*> addDependency) override {}
			};
		}

		Reference<JimaraEditor> JimaraEditor::Create(
			Graphics::GraphicsInstance* graphicsInstance, 
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice, 
			OS::Window* targetWindow) {
			// Logger:
			const Reference<OS::Logger> logger = (
				graphicsInstance != nullptr ? Reference<OS::Logger>(graphicsInstance->Log()) :
				physicsInstance != nullptr ? Reference<OS::Logger>(physicsInstance->Log()) :
				audioDevice != nullptr ? Reference<OS::Logger>(audioDevice->APIInstance()->Log()) :
				targetWindow != nullptr ? Reference<OS::Logger>(targetWindow->Log()) :
				Reference<OS::Logger>(Object::Instantiate<OS::StreamLogger>()));
			if (logger == nullptr) return nullptr;

			auto error = [&](const char* message) {
				logger->Error(message);
				return nullptr;
			};

			Stopwatch totalTime;
			Stopwatch stopwatch;

			// Application info:
			const Reference<const Application::AppInformation> appInfo = (
				graphicsInstance != nullptr ? Reference<const Application::AppInformation>(graphicsInstance->AppInfo()) :
				Reference<const Application::AppInformation>(Object::Instantiate<Application::AppInformation>("Jimara Editor", Application::AppVersion(0, 0, 1))));
			if (appInfo == nullptr) 
				return error("JimaraEditor::Create - AppInfo could not be created!");

			// Graphics instance:
			const Reference<Graphics::GraphicsInstance> graphics = (
				graphicsInstance != nullptr ? Reference<Graphics::GraphicsInstance>(graphicsInstance) :
				Graphics::GraphicsInstance::Create(logger, appInfo, Graphics::GraphicsInstance::Backend::VULKAN));
			if (graphics == nullptr)
				return error("JimaraEditor::Create - Graphics instance could not be created!");
			logger->Debug("JimaraEditor::Create - GraphicsInstance created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor window:
			const Reference<OS::Window> window = (
				targetWindow != nullptr ? Reference<OS::Window>(targetWindow) :
				OS::Window::Create(logger, "Jimara Editor", Size2(1280, 720), true, OS::Window::Backend::GLFW));
			if (window == nullptr)
				return error("JimaraEditor::Create - Editor window instance could not be created!");
			logger->Debug("JimaraEditor::Create - Window created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Render surface:
			const Reference<Graphics::RenderSurface> surface = graphics->CreateRenderSurface(window);
			if (surface == nullptr)
				return error("JimaraEditor::Create - Render surface could not be created!");
			logger->Debug("JimaraEditor::Create - RenderSurface created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Graphics Device:
			const Reference<Graphics::GraphicsDevice> graphicsDevice = [&]() ->Reference<Graphics::GraphicsDevice> {
				Graphics::PhysicalDevice* physicalDevice = surface->PrefferedDevice();
				if (physicalDevice == nullptr)
					return error("JimaraEditor::Create - Render surface has no compatible physical device!");
				else {
					Reference<Graphics::GraphicsDevice> logicalDevice = physicalDevice->CreateLogicalDevice();
					if (logicalDevice == nullptr) logger->Error("JimaraEditor::Create - Failed to create logical graphics device!");
					return logicalDevice;
				}
			}();
			if (graphicsDevice == nullptr) return nullptr;
			logger->Debug("JimaraEditor::Create - GraphicsDevice created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Physics instance:
			const Reference<Physics::PhysicsInstance> physics = (
				physicsInstance != nullptr ? Reference<Physics::PhysicsInstance>(physicsInstance) :
				Physics::PhysicsInstance::Create(logger, Physics::PhysicsInstance::Backend::NVIDIA_PHYSX));
			if (physics == nullptr) return error("JimaraEditor::Create - Failed to create physics instance!");
			logger->Debug("JimaraEditor::Create - PhysicsInstance created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Audio device:
			const Reference<Audio::AudioDevice> audio = (
				audioDevice != nullptr ? Reference<Audio::AudioDevice>(audioDevice) : [&]()->Reference<Audio::AudioDevice> {
					const Reference<Audio::AudioInstance> audioInstance = Audio::AudioInstance::Create(logger, Audio::AudioInstance::Backend::OPEN_AL);
					if (audioInstance == nullptr) return error("JimaraEditor::Create - Failed to create audio instance!");
					{
						const Reference<Audio::PhysicalAudioDevice> defaultDevice = audioInstance->DefaultDevice();
						if (defaultDevice != nullptr) {
							const Reference<Audio::AudioDevice> au = defaultDevice->CreateLogicalDevice();
							if (au == nullptr) logger->Warning("JimaraEditor::Create - Failed to create logical device for the default audio device!");
							else return au;
						}
						else logger->Warning("JimaraEditor::Create - No default audio device available!");
					}
					for (size_t i = 0; i < audioInstance->PhysicalDeviceCount(); i++) {
						const Reference<Audio::PhysicalAudioDevice> physDevice = audioInstance->PhysicalDevice(i);
						if (physDevice == nullptr) {
							logger->Warning("JimaraEditor::Create - Physical audio device ", i, " is nullptr!");
							continue;
						}
						else {
							const Reference<Audio::AudioDevice> au = physDevice->CreateLogicalDevice();
							if (au == nullptr) logger->Warning("JimaraEditor::Create - Physical audio device ", i, "<", physDevice->Name(), "> failed to create a logical device!");
							else return au;
						}
					}
					return error("JimaraEditor::Create - Failed to create an audio device!");
				}());
			if (audio == nullptr)
				return error("JimaraEditor::Create - Failed to create AudioDevice!");
			logger->Debug("JimaraEditor::Create - AudioDevice created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// App Context:
			const Reference<AppContext> appContext = Object::Instantiate<AppContext>(graphicsDevice, physics, audio);
			if (appContext == nullptr)
				return error("JimaraEditor::Create - Failed to create the application context!");


			// Render Engine:
			const Reference<Graphics::RenderEngine> renderEngine = graphicsDevice->CreateRenderEngine(surface);
			if (renderEngine == nullptr)
				return error("JimaraEditor::Create - Failed to create render engine!");
			logger->Debug("JimaraEditor::Create - RenderEngine created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");


			// ImGui API context:
			const Reference<ImGuiAPIContext> imGuiContext = Object::Instantiate<ImGuiAPIContext>(logger);
			if (imGuiContext == nullptr)
				return error("JimaraEditor::Create - Failed to get ImGuiAPIContext!");

			// ImGui device context:
			const Reference<ImGuiDeviceContext> imGuiDeviceContext = imGuiContext->CreateDeviceContext(graphicsDevice, window);
			if (imGuiDeviceContext == nullptr)
				return error("JimaraEditor::Create - Failed to create ImGui device context!");
			logger->Debug("JimaraEditor::Create - ImGuiDeviceContext created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Engine type registry:
			const Reference<BuiltInTypeRegistrator> builtInTypeRegistry = BuiltInTypeRegistrator::Instance();
			if (builtInTypeRegistry == nullptr)
				return error("JimaraEditor::Create - Failed to retrieve built in type registry!");

			// Editor type registry:
			const Reference<JimaraEditorTypeRegistry> editorTypeRegistry = JimaraEditorTypeRegistry::Instance();
			if (editorTypeRegistry == nullptr)
				return error("JimaraEditor::Create - Failed to retrieve editor type registry!");
			logger->Debug("JimaraEditor::Create - Type registries created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Shader binary loader:
			const Reference<Graphics::ShaderLoader> shaderLoader = Object::Instantiate<Graphics::ShaderDirectoryLoader>("Shaders/", logger);
			if (shaderLoader == nullptr)
				return error("JimaraEditor::Create - Failed to create shader binary loader!");
			logger->Debug("JimaraEditor::Create - Shader loader created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Input module:
			const Reference<OS::Input> inputModule = window->CreateInputModule();
			if (inputModule == nullptr)
				return error("JimaraEditor::Create - Failed to create an input module!");
			logger->Debug("JimaraEditor::Create - Input module created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// File system database:
			const Reference<FileSystemDatabase> fileSystemDB = FileSystemDatabase::Create(graphicsDevice, audio, "Assets/", [&](size_t processed, size_t total) {
				static thread_local Stopwatch stopwatch;
				if (stopwatch.Elapsed() > 0.5f) {
					stopwatch.Reset();
					logger->Info("FileSystemDatabase - Files processed: ", processed, '/', total,
						" (", (static_cast<float>(processed) / static_cast<float>(total) * 100.0f), "%)", processed == total ? "" : "...");
				}
				});
			if (fileSystemDB == nullptr)
				return error("JimaraEditor::Create - Failed to create FileSystemDatabase!");
			logger->Debug("JimaraEditor::Create - FileSystemDatabase created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor context:
			const Reference<EditorContext> editorContext = new EditorContext(appContext, shaderLoader, inputModule, fileSystemDB);
			if (editorContext == nullptr)
				return error("JimaraEditor::Create - Failed to create editor context!");
			else editorContext->ReleaseRef();
			logger->Debug("JimaraEditor::Create - Editor context created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// EditorRenderer:
			void(*invokeJobs)(EditorContext*) = [](EditorContext* context) {
				Reference<JimaraEditor> editor;
				{
					std::unique_lock<SpinLock> lock(context->m_editorLock);
					editor = context->m_editor;
				}
				if (editor != nullptr) context->m_editor->m_jobs.Execute(context->ApplicationContext()->Log());
			};
			const Reference<Graphics::ImageRenderer> editorRenderer = Object::Instantiate<JimaraEditorRenderer>(
				editorContext, imGuiDeviceContext, Callback<>(invokeJobs, editorContext.operator->()));
			if (editorRenderer == nullptr)
				return error("JimaraEditor::Create - Failed to create editor renderer!");
			logger->Debug("JimaraEditor::Create - Editor renderer created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor instance:
			const Reference<JimaraEditor> editor = new JimaraEditor(
				std::vector<Reference<Object>>({ Reference<Object>(builtInTypeRegistry), Reference<Object>(editorTypeRegistry) }),
				editorContext, window, renderEngine, editorRenderer);
			if (editor == nullptr)
				return error("JimaraEditor::Create - Failed to create editor instance!");
			else {
				std::unique_lock<SpinLock> lock(editorContext->m_editorLock);
				editorContext->m_editor = editor;
				editor->ReleaseRef();
				return editor;
			}
		}

		JimaraEditor::~JimaraEditor() {
			m_window->OnUpdate() -= Callback<OS::Window*>(&JimaraEditor::OnUpdate, this);
			m_renderEngine->RemoveRenderer(m_renderer);
			std::unique_lock<SpinLock> lock(m_context->m_editorLock);
			m_context->m_editor = nullptr;
		}

		void JimaraEditor::WaitTillClosed()const {
			m_window->WaitTillClosed();
		}

		JimaraEditor::JimaraEditor(
			std::vector<Reference<Object>>&& typeRegistries, EditorContext* context, OS::Window* window,
			Graphics::RenderEngine* renderEngine, Graphics::ImageRenderer* renderer)
			: m_typeRegistries(std::move(typeRegistries)), m_context(context), m_window(window), m_renderEngine(renderEngine), m_renderer(renderer) {
			m_renderEngine->AddRenderer(m_renderer);
			m_window->OnUpdate() += Callback<OS::Window*>(&JimaraEditor::OnUpdate, this);
		}

		void JimaraEditor::OnUpdate(OS::Window*) {
			m_renderEngine->Update();
		}


		EditorMainMenuAction::EditorMainMenuAction(const std::string_view& menuPath) : m_path(menuPath) {}

		const std::string& EditorMainMenuAction::MenuPath()const { return m_path; }

		EditorMainMenuAction::RegistryEntry::RegistryEntry(const EditorMainMenuAction* instancer) { (*this) = instancer; }

		EditorMainMenuAction::RegistryEntry::~RegistryEntry() { (*this) = nullptr; }

		namespace {
			inline static std::recursive_mutex& MainMenuActionRegistryLock() {
				static std::recursive_mutex lock;
				return lock;
			}

			typedef std::map<std::string_view, std::map<const EditorMainMenuAction*, size_t>> MainMenuActionRegistry;
			inline static MainMenuActionRegistry& MainMenuActionMap() {
				static MainMenuActionRegistry registry;
				return registry;
			}
		}

		EditorMainMenuAction::RegistryEntry::operator const EditorMainMenuAction* ()const { return m_action; }

		EditorMainMenuAction::RegistryEntry& EditorMainMenuAction::RegistryEntry::operator=(const EditorMainMenuAction* action) {
			std::unique_lock<std::recursive_mutex> lock(MainMenuActionRegistryLock());
			if (m_action == action) return *this;
			if (action != nullptr) {
				std::map<const EditorMainMenuAction*, size_t>& samePathInstancers = MainMenuActionMap()[action->MenuPath()];
				std::map<const EditorMainMenuAction*, size_t>::iterator it = samePathInstancers.find(action);
				if (it != samePathInstancers.end()) it->second++;
				else samePathInstancers.insert(std::make_pair(action, 1));
			}
			if (m_action != nullptr) {
				std::map<const EditorMainMenuAction*, size_t>& samePathInstancers = MainMenuActionMap()[m_action->MenuPath()];
				std::map<const EditorMainMenuAction*, size_t>::iterator it = samePathInstancers.find(m_action);
				it->second--;
				if (it->second <= 0) {
					samePathInstancers.erase(it);
					if (samePathInstancers.empty()) MainMenuActionMap().erase(m_action->MenuPath());
				}
			}
			m_action = action;
			return *this;
		}

		EditorMainMenuAction::RegistryEntry& EditorMainMenuAction::RegistryEntry::operator=(const EditorMainMenuAction::RegistryEntry& entry) {
			return (*this) = entry.m_action;
		}

		void EditorMainMenuAction::RegistryEntry::GetAll(Callback<const EditorMainMenuAction*> recordEntry) {
			static thread_local std::vector<Reference<const EditorMainMenuAction>> entries;
			size_t startId, endId;
			{
				std::unique_lock<std::recursive_mutex> lock(MainMenuActionRegistryLock());
				startId = entries.size();
				for (MainMenuActionRegistry::const_iterator it = MainMenuActionMap().begin(); it != MainMenuActionMap().end(); ++it)
					for (std::map<const EditorMainMenuAction*, size_t>::const_iterator i = it->second.begin(); i != it->second.end(); ++i)
						entries.push_back(i->first);
				endId = entries.size();
			}
			for (size_t i = startId; i < endId; i++) recordEntry(entries[i]);
			entries.resize(startId);
		}


		EditorMainMenuCallback::EditorMainMenuCallback(const std::string_view& menuPath, const Callback<EditorContext*>& action)
			: EditorMainMenuAction(menuPath), m_action(action) {}

		void EditorMainMenuCallback::Execute(EditorContext* context)const { m_action(context); }
	}
}
