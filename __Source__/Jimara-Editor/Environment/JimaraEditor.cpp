#include "JimaraEditor.h"
#include "EditorStorage.h"
#include "../GUI/ImGuiRenderer.h"
#include "../GUI/Utils/DrawMenuAction.h"
#include "../ActionManagement/HotKey.h"
#include "../__Generated__/JIMARA_EDITOR_LIGHT_IDENTIFIERS.h"
#include <OS/Logging/StreamLogger.h>
#include <Core/Stopwatch.h>
#include <Data/Serialization/Helpers/SerializeToJson.h>
#include <Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h>
#include <fstream>
#include <map>

namespace Jimara {
	namespace Editor {
		Reference<EditorInput> EditorContext::CreateInputModule()const {
			Reference<JimaraEditor> editor = [&]() -> Reference<JimaraEditor> {
				std::unique_lock<SpinLock> lock(m_editorLock);
				Reference<JimaraEditor> rv = m_editor;
				return rv;
			}();
			if (editor != nullptr) {
				Reference<OS::Input> baseInput = editor->m_window->CreateInputModule();
				return Object::Instantiate<EditorInput>(baseInput);
			}
			else return nullptr;
		}

		EditorContext::SceneLightTypes EditorContext::LightTypes()const { 
			EditorContext::SceneLightTypes types = {};
			types.lightTypeIds = &LightRegistry::JIMARA_EDITOR_LIGHT_IDENTIFIERS.typeIds;
			types.perLightDataSize = LightRegistry::JIMARA_EDITOR_LIGHT_IDENTIFIERS.perLightDataSize;
			return types;
		}

		LightingModel* EditorContext::DefaultLightingModel()const { return ForwardLightingModel::Instance(); }

		FileSystemDatabase* EditorContext::EditorAssetDatabase()const { return m_fileSystemDB; }

		void EditorContext::AddRenderJob(JobSystem::Job* job) { 
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Add(job);
		}

		void EditorContext::RemoveRenderJob(JobSystem::Job* job) {
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Remove(job);
		}

		EditorContext::EditorContext(
			OS::Logger* logger,
			Graphics::GraphicsDevice* graphicsDevice, Graphics::ShaderLoader* shaderLoader,
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice,
			OS::Input* inputModule,
			FileSystemDatabase* database)
			: m_logger(logger)
			, m_graphicsDevice(graphicsDevice)
			, m_shaderLoader(shaderLoader)
			, m_physicsInstance(physicsInstance)
			, m_audioDevice(audioDevice)
			, m_inputModule(inputModule)
			, m_fileSystemDB(database) { }
		
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

		void EditorContext::AddUndoAction(UndoStack::Action* action)const {
			Reference<JimaraEditor> editor = [&]() -> Reference<JimaraEditor> {
				std::unique_lock<SpinLock> lock(m_editorLock);
				Reference<JimaraEditor> rv = m_editor;
				return rv;
			}();
			if (editor != nullptr)
				editor->m_undoActions.push_back(action);
		}

		void EditorContext::AddStorageObject(Object* object) {
			if (object == nullptr) return;
			Reference<JimaraEditor> editor = [&]() -> Reference<JimaraEditor> {
				std::unique_lock<SpinLock> lock(m_editorLock);
				Reference<JimaraEditor> rv = m_editor;
				return rv;
			}();
			if (editor != nullptr) 
				editor->m_editorStorage.insert(object);
		}

		void EditorContext::RemoveStorageObject(Object* object) {
			if (object == nullptr) return;
			Reference<JimaraEditor> editor = [&]() -> Reference<JimaraEditor> {
				std::unique_lock<SpinLock> lock(m_editorLock);
				Reference<JimaraEditor> rv = m_editor;
				return rv;
			}();
			if (editor != nullptr) 
				editor->m_editorStorage.erase(object);
		}


		namespace {
			struct EditorPersistentData {
				std::unordered_set<Reference<Object>>* objects = nullptr;
				EditorContext* context = nullptr;
			};

			class EditorDataSerializer : public virtual Serialization::SerializerList::From<EditorPersistentData> {
			private:
				typedef std::pair<Reference<const EditorStorageSerializer>, Reference<Object>> Entry;
				struct EntryData {
					Entry entry;
					EditorContext* context = nullptr;
					const EditorStorageSerializer::Set* serializers = nullptr;
				};

				class DataSerializer : public virtual Serialization::SerializerList::From<Entry> {
				public:
					inline DataSerializer() : Serialization::ItemSerializer("Data") {}

					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Entry* target)const final override {
						if (target->first != nullptr && target->second != nullptr)
							target->first->GetFields(recordElement, target->second);
					}
				};

				class EntryDataSerializer : public virtual Serialization::SerializerList::From<EntryData> {
				public:
					inline EntryDataSerializer() : Serialization::ItemSerializer("Entry") {}

					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EntryData* target)const final override {
						// Serialize type:
						{
							static const Reference<const Serialization::ItemSerializer::Of<EntryData>> serializer =
								Serialization::StringViewSerializer::For<EntryData>("Type", "Type of the storage",
									[](EntryData* entry) -> std::string_view {
										return entry->entry.first == nullptr ? std::string_view("") : entry->entry.first->StorageType().Name();
									}, [](const std::string_view& value, EntryData* entry) {
										entry->entry.first = entry->serializers->FindSerializerOf(value);
									});
							recordElement(serializer->Serialize(target));
						}

						// Update reference:
						{
							if (target->serializers->FindSerializerOf(target->entry.second) != target->entry.first) target->entry.second = nullptr;
							if (target->entry.second == nullptr && target->entry.first != nullptr && target->context != nullptr)
								target->entry.second = target->entry.first->CreateObject(target->context);
						}

						// Serialize data:
						{
							static const DataSerializer serializer;
							recordElement(serializer.Serialize(target->entry));
						}
					}
				};

			public:
				inline EditorDataSerializer() : Serialization::ItemSerializer("EditorStorage") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EditorPersistentData* target)const final override {
					const Reference<const EditorStorageSerializer::Set> serializers = EditorStorageSerializer::Set::All();
					std::vector<Entry> data;
					
					// 'Assemble' data
					{
						for (auto item : *target->objects) {
							const Entry entry(serializers->FindSerializerOf(item), item);
							if (entry.first != nullptr && entry.second != nullptr)
								data.push_back(entry);
						}
						for (size_t i = 0; i < data.size(); i++)
							target->objects->erase(data[i].second);
					}

					// Serialize number of entries
					{
						static const Reference<const Serialization::ItemSerializer::Of<decltype(data)>> serializer =
							Serialization::Uint32Serializer::For<decltype(data)>("Count", "Number of entries",
								[](decltype(data)* data) -> uint32_t { return static_cast<uint32_t>(data->size()); },
								[](const uint32_t& count, decltype(data)* data) { data->resize(count); });
						recordElement(serializer->Serialize(data));
					}

					// Serialize data
					{
						static const EntryDataSerializer serializer;
						EntryData entry;
						entry.context = target->context;
						entry.serializers = serializers;
						for (size_t i = 0; i < data.size(); i++) {
							entry.entry = data[i];
							recordElement(serializer.Serialize(entry));
							data[i] = entry.entry;
						}
					}

					// Store data back to target
					for (size_t i = 0; i < data.size(); i++) {
						Object* entry = data[i].second;
						if (entry != nullptr)
							target->objects->insert(entry);
					}
				}

				static const EditorDataSerializer* Instance() {
					static const EditorDataSerializer instance;
					return &instance;
				}

				static const OS::Path& StoragePath() {
					static const OS::Path path("JimaraEditorData");
					return path;
				}

				inline static void Load(std::unordered_set<Reference<Object>>& objects, EditorContext* context) {
					if (context == nullptr) return;
					const Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(StoragePath());
					if (mapping == nullptr) return; // No error needed... We might not need to load anything...
					nlohmann::json json;
					try {
						MemoryBlock block(*mapping);
						json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
					}
					catch (nlohmann::json::parse_error& err) {
						context->Log()->Error("EditorDataSerializer::Load - Could not parse file: \"", StoragePath(), "\"! [Error: <", err.what(), ">]");
						return;
					}
					EditorPersistentData target{ &objects, context };
					if (!Serialization::DeserializeFromJson(Instance()->Serialize(target), json, context->Log(),
						[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
							context->Log()->Warning("EditorDataSerializer::Load - Object pointer storage not [yet] supported! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return true;
						})) context->Log()->Error("EditorDataSerializer::Load - Serialization error occured! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				inline static void Store(std::unordered_set<Reference<Object>>& objects, EditorContext* context) {
					if (context == nullptr) return;
					EditorPersistentData target{ &objects, context };
					bool error = false;
					const nlohmann::json json = Serialization::SerializeToJson(Instance()->Serialize(target), context->Log(), error,
						[&](const Serialization::SerializedObject&, bool&) -> const nlohmann::json {
							context->Log()->Warning("EditorDataSerializer::Store - Object pointer storage not [yet] supported! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return {};
						});
					if (error) {
						context->Log()->Error("EditorDataSerializer::Store - Serialization error occured! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
					std::ofstream stream(StoragePath());
					if (!stream.good()) {
						context->Log()->Error("EditorDataSerializer::Store - Failed to open file: '", StoragePath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
					stream << json.dump(1, '\t') << std::endl;
					stream.close();
				}
			};

			class JimaraEditorRenderer : public virtual Graphics::ImageRenderer, public virtual JobSystem::Job {
			private:
				const Reference<EditorContext> m_editorContext;
				const Reference<ImGuiDeviceContext> m_deviceContext;
				const Callback<> m_executeRenderJobs;
				Stopwatch m_frameTimer;

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
					// Update Input:
					m_editorContext->InputModule()->Update(m_frameTimer.Reset());

					ImGui::DockSpaceOverViewport();
					
					// Main meny bar:
					ImGui::BeginMainMenuBar();
					EditorMainMenuAction::RegistryEntry::GetAll([&](const EditorMainMenuAction* action) {
						if (DrawMenuAction(action->MenuPath(), action))
							action->Execute(m_editorContext);
						});
					ImGui::EndMainMenuBar();

					// ImGui Render jobs:
					m_executeRenderJobs();

					// yield to prevent congestions of various kind :)
					std::this_thread::yield();
				}

				virtual void CollectDependencies(Callback<Job*> addDependency) override {}
			};
		}

		namespace {
			static EventInstance<EditorContext*> onNoFieldActive;

			class EditorFeildModifyAction : public virtual UndoStack::Action {
			private:
				Reference<EditorContext> m_context;

				inline void Invalidate(EditorContext* context) {
					if (context != m_context) return;
					onNoFieldActive.operator Jimara::Event<EditorContext*>& () -= Callback(&EditorFeildModifyAction::Invalidate, this);
					m_context = nullptr;
				}
			public:
				inline EditorFeildModifyAction(EditorContext* context) : m_context(context) { 
					onNoFieldActive.operator Jimara::Event<EditorContext*>& () += Callback(&EditorFeildModifyAction::Invalidate, this);
				}
				inline virtual ~EditorFeildModifyAction() { Invalidate(m_context); }
				virtual bool Invalidated()const { return m_context == nullptr; }
				virtual void Undo() {}
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
			const Reference<FileSystemDatabase> fileSystemDB = FileSystemDatabase::Create(graphicsDevice, physics, audio, "Assets/", [&](size_t processed, size_t total) {
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
			const Reference<EditorContext> editorContext = new EditorContext(logger, graphicsDevice, shaderLoader, physics, audio, inputModule, fileSystemDB);
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
				if (editor == nullptr) return;

				// Perform Undo
				if (HotKey::Undo().Check(context->InputModule()))
					editor->m_undoManager->Undo();

				// Draw all windows...
				editor->m_jobs.Execute(context->Log());

				// Push undo actions:
				static thread_local Stopwatch undoPushTimer;
				static const constexpr float MIN_UNDO_PUSH_INTERVAL = 0.025f;
				if (!ImGui::IsAnyItemActive())
					onNoFieldActive(context);
				else if (ImGuiRenderer::AnyFieldModified() && undoPushTimer.Elapsed() >= MIN_UNDO_PUSH_INTERVAL) {
					editor->m_undoActions.push_back(Object::Instantiate<EditorFeildModifyAction>(context));
					undoPushTimer.Reset();
				}
				if (editor->m_undoActions.size() > 0) {
					editor->m_undoManager->AddAction(UndoStack::Action::Combine(editor->m_undoActions.data(), editor->m_undoActions.size()));
					editor->m_undoActions.clear();
				}
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
				editor->ReleaseRef();
				return editor;
			}
		}

		JimaraEditor::~JimaraEditor() {
			m_window->OnUpdate() -= Callback<OS::Window*>(&JimaraEditor::OnUpdate, this);
			m_renderEngine->RemoveRenderer(m_renderer);
			EditorDataSerializer::Store(m_editorStorage, m_context);
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
			if (m_context != nullptr) {
				std::unique_lock<SpinLock> lock(m_context->m_editorLock);
				m_context->m_editor = this;
			}
			EditorDataSerializer::Load(m_editorStorage, m_context);
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
