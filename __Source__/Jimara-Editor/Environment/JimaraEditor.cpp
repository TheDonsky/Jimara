#include "JimaraEditor.h"
#include "EditorStorage.h"
#include "../GUI/ImGuiRenderer.h"
#include "../GUI/Utils/DrawMenuAction.h"
#include "../ActionManagement/HotKey.h"
#include <Jimara/OS/Logging/StreamLogger.h>
#include <Jimara/OS/System/DynamicLibrary.h>
#include <Jimara/Core/Stopwatch.h>
#include <Jimara/Data/Serialization/Helpers/SerializeToJson.h>
#include <Jimara/Data/Serialization/Helpers/ComponentHierarchySerializer.h>
#include <Jimara/Environment/Rendering/LightingModels/ForwardRendering/ForwardPlusLightingModel.h>
#include <fstream>
#include <map>

namespace Jimara {
	namespace Editor {
		Reference<EditorInput> EditorContext::CreateInputModule()const {
			Reference<OS::Input> baseInput = m_window->CreateInputModule();
			return Object::Instantiate<EditorInput>(baseInput);
		}

		LightingModel* EditorContext::DefaultLightingModel()const { return ForwardPlusLightingModel::Instance(); }

		FileSystemDatabase* EditorContext::EditorAssetDatabase()const { return m_fileSystemDB; }

		void EditorContext::AddRenderJob(JobSystem::Job* job) { 
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Add(job);
		}

		void EditorContext::RemoveRenderJob(JobSystem::Job* job) {
			std::unique_lock<SpinLock> lock(m_editorLock);
			if (m_editor != nullptr) m_editor->m_jobs.Remove(job);
		}

		Event<>& EditorContext::OnMainLoop() { return m_onMainLoopUpdate; }

		EditorContext::EditorContext(
			OS::Logger* logger,
			Graphics::GraphicsDevice* graphicsDevice,
			Physics::PhysicsInstance* physicsInstance,
			Audio::AudioDevice* audioDevice,
			OS::Input* inputModule,
			FileSystemDatabase* database,
			Jimara::ShaderLibrary* shaderLibrary,
			OS::Window* window)
			: m_logger(logger)
			, m_graphicsDevice(graphicsDevice)
			, m_physicsInstance(physicsInstance)
			, m_audioDevice(audioDevice)
			, m_inputModule(inputModule)
			, m_fileSystemDB(database)
			, m_shaderLibrary(shaderLibrary)
			, m_window(window) { }
		
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

				typedef std::pair<const Serialization::SerializedObject*, EditorContext*> SerializeAsGUIDInput;
				class SerializeAsGUID : public virtual Serialization::SerializerList::From<SerializeAsGUIDInput> {
				public:
					inline SerializeAsGUID(const Serialization::ItemSerializer* base) 
						: Serialization::ItemSerializer(base->TargetName(), base->TargetHint()) {}
					virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializeAsGUIDInput* target)const {
						const Serialization::ObjectReferenceSerializer* referenceSerializer = target->first->As<Serialization::ObjectReferenceSerializer>();
						if (referenceSerializer == nullptr) {
							target->second->Log()->Error("EditorDataSerializer::SerializeAsGUID::GetFields - Unsupported serializer type!");
							return;
						}
						const Reference<Object> objValue = referenceSerializer->GetObjectValue(target->first->TargetAddr());
						const Reference<Asset> assetValue = objValue;
						const Reference<Resource> resourceValue = objValue;
						GUID guid =
							(assetValue != nullptr) ? assetValue->Guid() :
							(resourceValue != nullptr && resourceValue->HasAsset()) ? resourceValue->GetAsset()->Guid() : GUID{};
						{
							static const GUID::Serializer serializer("GUID", "GUID");
							serializer.GetFields(recordElement, &guid);
						}
						const Reference<Asset> updatedAsset = target->second->EditorAssetDatabase()->FindAsset(guid);
						if (objValue != updatedAsset && referenceSerializer->ReferencedValueType().CheckType(updatedAsset))
							referenceSerializer->SetObjectValue(updatedAsset, target->first->TargetAddr());
						else if (updatedAsset != nullptr) {
							const Reference<Resource> updatedResource = updatedAsset->LoadResource();
							if (objValue != updatedResource && referenceSerializer->ReferencedValueType().CheckType(updatedResource))
								referenceSerializer->SetObjectValue(updatedResource, target->first->TargetAddr());
						}
					}
				};

			public:
				inline EditorDataSerializer() : Serialization::ItemSerializer("EditorStorage") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EditorPersistentData* target)const final override {
					// Serialize currently loaded scene
					{
						static const Reference<const Serialization::ItemSerializer::Of<std::string>> serializer =
							Serialization::StringViewSerializer::For<std::string>(
								"Scene", "Currently open scene",
								[](std::string* path) -> std::string_view { return *path; },
								[](const std::string_view& value, std::string* path) { (*path) = value; });
						Reference<EditorScene> scene = target->context->GetScene();
						const std::optional<OS::Path> path = (scene != nullptr) ? scene->AssetPath() : std::optional<OS::Path>();
						const std::string originalPathString = path.has_value() ? ((std::string)path.value()) : std::string("");
						std::string pathString = originalPathString;
						recordElement(serializer->Serialize(pathString));
						if (pathString != originalPathString) {
							if (scene == nullptr) {
								scene = Object::Instantiate<EditorScene>(target->context);
								target->context->SetScene(scene);
							}
							std::error_code err;
							if (pathString.length() > 0 && std::filesystem::exists(pathString, err) && (!err))
								scene->Load(pathString);
							else scene->Clear();
						}
					}
					
					const Reference<const EditorStorageSerializer::Set> serializers = EditorStorageSerializer::Set::All();
					std::vector<Entry> data;
					
					// 'Assemble' data
					{
						for (const auto& item : *target->objects) {
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
						[&](const Serialization::SerializedObject& obj, const nlohmann::json& data) -> bool {
							SerializeAsGUIDInput input(&obj, context);
							const SerializeAsGUID serializer(obj.Serializer());
							return Serialization::DeserializeFromJson(serializer.Serialize(input), data, context->Log(),
								[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
									context->Log()->Error(
										"EditorDataSerializer::Load - SerializeAsGUID should not have any object pointer references! [File: ", __FILE__, "; Line: ", __LINE__, "]");
									return false;
								});
						})) context->Log()->Error("EditorDataSerializer::Load - Serialization error occured! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				inline static void Store(std::unordered_set<Reference<Object>>& objects, EditorContext* context) {
					if (context == nullptr) return;
					EditorPersistentData target{ &objects, context };
					bool error = false;
					const nlohmann::json json = Serialization::SerializeToJson(Instance()->Serialize(target), context->Log(), error,
						[&](const Serialization::SerializedObject& obj, bool& err) -> nlohmann::json {
							SerializeAsGUIDInput input(&obj, context);
							const SerializeAsGUID serializer(obj.Serializer());
							return Serialization::SerializeToJson(serializer.Serialize(input), context->Log(), err,
								[&](const Serialization::SerializedObject& obj, bool& e)->nlohmann::json {
									context->Log()->Error(
										"EditorDataSerializer::Store - SerializeAsGUID should not have any Object pointers! [File: ", __FILE__, "; Line: ", __LINE__, "]");
									e = true;
									return {};
								});
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
				float m_averageFrameTime = -1.0f;

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

				inline virtual void Render(Object* engineData, Graphics::InFlightBufferInfo bufferInfo) override {
					ImGuiRenderer* renderer = dynamic_cast<ImGuiRenderer*>(engineData);
					if (renderer != nullptr) renderer->Render(bufferInfo);
					else m_deviceContext->GraphicsDevice()->Log()->Error("JimaraEditorRenderer::Render - Invalid engine data!");
				}


				// JobSystem::Job:
				virtual void Execute() override {
					// Update last known framerate:
					const float frameTime = m_frameTimer.Reset();
					m_averageFrameTime =
						(m_averageFrameTime <= 0.0f) ? frameTime :
						Math::Lerp(m_averageFrameTime, frameTime, Math::Min(Math::Max(0.01f, frameTime * 4.0f), 1.0f));
					m_editorContext->Window()->SetName([&]() -> std::string {
						std::stringstream stream;
						stream << "Jimara Editor " << std::fixed << std::setprecision(2) <<
							"[" << (m_averageFrameTime * 1000.0f) << " ms; "
							<< (1.0f / Math::Max(m_averageFrameTime, std::numeric_limits<float>::epsilon())) << " fps]";
						return stream.str();
						} ());

					// Update Input:
					m_editorContext->InputModule()->Update(frameTime);

					ImGui::DockSpaceOverViewport();
					
					// Main meny bar:
					ImGui::BeginMainMenuBar();
					EditorMainMenuAction::GetAll([&](const EditorMainMenuAction* action) {
						if (DrawMenuAction(action->MenuPath(), action->Tooltip(), action))
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

			class EditorShaderLibrary : public virtual Jimara::ShaderLibrary {
			private:
				mutable SpinLock m_loaderLock;
				Reference<Jimara::ShaderLibrary> m_loader;

			public:
				Reference<Jimara::ShaderLibrary> Loader()const {
					std::unique_lock<SpinLock> lock(m_loaderLock);
					Reference<Jimara::ShaderLibrary> loader(m_loader);
					return m_loader;
				}

				void SetLoader(Jimara::ShaderLibrary* loader) {
					std::unique_lock<SpinLock> lock(m_loaderLock);
					m_loader = loader;
				}

				inline virtual const Material::LitShaderSet* LitShaders()const final override {
					Reference<Jimara::ShaderLibrary> loader = Loader();
					if (loader != nullptr)
						return loader->LitShaders();
					else return nullptr;
				}

				inline virtual Reference<Graphics::SPIRV_Binary> LoadLitShader(
					const std::string_view& lightingModelPath, const std::string_view& lightingModelStage,
					const Material::LitShader* litShader, Graphics::PipelineStage graphicsStage) final override {
					Reference<Jimara::ShaderLibrary> loader = Loader();
					if (loader != nullptr)
						return loader->LoadLitShader(lightingModelPath, lightingModelStage, litShader, graphicsStage);
					else return nullptr;
				}

				inline virtual Reference<Graphics::SPIRV_Binary> LoadShader(const std::string_view& path) final override {
					Reference<Jimara::ShaderLibrary> loader = Loader();
					if (loader != nullptr)
						return loader->LoadShader(path);
					else return nullptr;
				}

				inline virtual bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const final override {
					Reference<Jimara::ShaderLibrary> loader = Loader();
					if (loader != nullptr)
						return loader->GetLightTypeId(lightTypeName, lightTypeId);
					else return false;
				}

				inline virtual size_t PerLightDataSize()const final override {
					Reference<Jimara::ShaderLibrary> loader = Loader();
					return (loader == nullptr) ? size_t(0u) : loader->PerLightDataSize();
				}
			};

			static const std::string GAME_LIBRARY_DIRECTORY = "Game";
			static const std::string LOADED_LIBRARY_DIRECTORY = ".jimara";
		}

		Reference<JimaraEditor> JimaraEditor::Create(const CreateArgs& args) {
			// Logger:
			const Reference<OS::Logger> logger = (
				args.graphicsInstance != nullptr ? Reference<OS::Logger>(args.graphicsInstance->Log()) :
				args.physicsInstance != nullptr ? Reference<OS::Logger>(args.physicsInstance->Log()) :
				args.audioDevice != nullptr ? Reference<OS::Logger>(args.audioDevice->APIInstance()->Log()) :
				args.targetWindow != nullptr ? Reference<OS::Logger>(args.targetWindow->Log()) :
				Reference<OS::Logger>(Object::Instantiate<OS::StreamLogger>()));
			if (logger == nullptr) return nullptr;

			auto error = [&](const auto&... message) {
				logger->Error(message...);
				return nullptr;
			};

			Stopwatch totalTime;
			Stopwatch stopwatch;

			// Application info:
			const Reference<const Application::AppInformation> appInfo = (
				args.graphicsInstance != nullptr ? Reference<const Application::AppInformation>(args.graphicsInstance->AppInfo()) :
				Reference<const Application::AppInformation>(Object::Instantiate<Application::AppInformation>("Jimara Editor", Application::AppVersion(0, 0, 1))));
			if (appInfo == nullptr) 
				return error("JimaraEditor::Create - AppInfo could not be created!");

			// Graphics instance:
			const Reference<Graphics::GraphicsInstance> graphics = (
				args.graphicsInstance != nullptr ? Reference<Graphics::GraphicsInstance>(args.graphicsInstance) :
				Graphics::GraphicsInstance::Create(logger, appInfo, Graphics::GraphicsInstance::Backend::VULKAN));
			if (graphics == nullptr)
				return error("JimaraEditor::Create - Graphics instance could not be created!");
			logger->Info("JimaraEditor::Create - GraphicsInstance created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor window:
			const Reference<OS::Window> window = (
				args.targetWindow != nullptr ? Reference<OS::Window>(args.targetWindow) :
				OS::Window::Create(logger, "Jimara Editor", Size2(1280, 720), true, OS::Window::Backend::GLFW));
			if (window == nullptr)
				return error("JimaraEditor::Create - Editor window instance could not be created!");
			logger->Info("JimaraEditor::Create - Window created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Render surface:
			const Reference<Graphics::RenderSurface> surface = graphics->CreateRenderSurface(window);
			if (surface == nullptr)
				return error("JimaraEditor::Create - Render surface could not be created!");
			logger->Info("JimaraEditor::Create - RenderSurface created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Graphics Device:
			const Reference<Graphics::GraphicsDevice> graphicsDevice = [&]() -> Reference<Graphics::GraphicsDevice> {
				Graphics::PhysicalDevice* physicalDevice;
				if (args.graphicsDeviceIndex.has_value()) {
					const size_t deviceId = args.graphicsDeviceIndex.value();
					if (deviceId >= graphics->PhysicalDeviceCount())
						return error("JimaraEditor::Create - Graphics device ", deviceId, " does not exist!");
					physicalDevice = graphics->GetPhysicalDevice(deviceId);
					if (!surface->DeviceCompatible(physicalDevice))
						logger->Warning("JimaraEditor::Create - Graphics device ", deviceId, " not compatible with render surface!");
				}
				else physicalDevice = surface->PrefferedDevice();
				if (physicalDevice == nullptr)
					return error("JimaraEditor::Create - Render surface has no compatible physical device!");
				else {
					logger->Info("JimaraEditor::Create - Graphics device: ", physicalDevice->Name(), " (Features: ", physicalDevice->Features(), ")");
					Reference<Graphics::GraphicsDevice> logicalDevice = physicalDevice->CreateLogicalDevice();
					if (logicalDevice == nullptr) logger->Error("JimaraEditor::Create - Failed to create logical graphics device!");
					return logicalDevice;
				}
			}();
			if (graphicsDevice == nullptr) return nullptr;
			logger->Info("JimaraEditor::Create - GraphicsDevice created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Physics instance:
			const Reference<Physics::PhysicsInstance> physics = (
				args.physicsInstance != nullptr ? Reference<Physics::PhysicsInstance>(args.physicsInstance) :
				Physics::PhysicsInstance::Create(logger, Physics::PhysicsInstance::Backend::NVIDIA_PHYSX));
			if (physics == nullptr) return error("JimaraEditor::Create - Failed to create physics instance!");
			logger->Info("JimaraEditor::Create - PhysicsInstance created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Audio device:
			const Reference<Audio::AudioDevice> audio = (
				args.audioDevice != nullptr ? Reference<Audio::AudioDevice>(args.audioDevice) : [&]()->Reference<Audio::AudioDevice> {
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
			logger->Info("JimaraEditor::Create - AudioDevice created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");


			// Render Engine:
			const Reference<Graphics::RenderEngine> renderEngine = graphicsDevice->CreateRenderEngine(surface);
			if (renderEngine == nullptr)
				return error("JimaraEditor::Create - Failed to create render engine!");
			logger->Info("JimaraEditor::Create - RenderEngine created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");


			// ImGui API context:
			const Reference<ImGuiAPIContext> imGuiContext = Object::Instantiate<ImGuiAPIContext>(logger);
			if (imGuiContext == nullptr)
				return error("JimaraEditor::Create - Failed to get ImGuiAPIContext!");

			// ImGui device context:
			const Reference<ImGuiDeviceContext> imGuiDeviceContext = imGuiContext->CreateDeviceContext(graphicsDevice, window);
			if (imGuiDeviceContext == nullptr)
				return error("JimaraEditor::Create - Failed to create ImGui device context!");
			logger->Info("JimaraEditor::Create - ImGuiDeviceContext created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Registries and dynamic libraries:
			std::vector<Reference<Object>> registries;

			// Engine type registry:
			const Reference<BuiltInTypeRegistrator> builtInTypeRegistry = BuiltInTypeRegistrator::Instance();
			if (builtInTypeRegistry == nullptr)
				return error("JimaraEditor::Create - Failed to retrieve built in type registry!");
			else registries.push_back(builtInTypeRegistry);

			// Editor type registry:
			const Reference<JimaraEditorTypeRegistry> editorTypeRegistry = JimaraEditorTypeRegistry::Instance();
			if (editorTypeRegistry == nullptr)
				return error("JimaraEditor::Create - Failed to retrieve editor type registry!");
			else registries.push_back(editorTypeRegistry);
			logger->Info("JimaraEditor::Create - Type registries created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Input module:
			const Reference<OS::Input> inputModule = window->CreateInputModule();
			if (inputModule == nullptr)
				return error("JimaraEditor::Create - Failed to create an input module!");
			logger->Info("JimaraEditor::Create - Input module created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Game library directory:
			const OS::Path gameLibraryDir = "Game";
			{
				std::error_code fsError;
				std::filesystem::create_directories(gameLibraryDir, fsError);
				if (fsError) return error("JimaraEditor::Create - Failed to create game library directories!");
				if (std::filesystem::exists(LOADED_LIBRARY_DIRECTORY)) {
					std::filesystem::remove_all(LOADED_LIBRARY_DIRECTORY, fsError);
					if (fsError) return error("JimaraEditor::Create - Failed to clean the directory of old loaded libraries");
				}
				std::filesystem::create_directories(LOADED_LIBRARY_DIRECTORY, fsError);
				if (fsError) return error("JimaraEditor::Create - Failed to create directories for loaded libraries!");
			}
			const Reference<OS::DirectoryChangeObserver> gameLibraryObserver = OS::DirectoryChangeObserver::Create(gameLibraryDir, logger);
			if (gameLibraryObserver == nullptr) return error("JimaraEditor::Create - Failed to create game library observer!");
			logger->Info("JimaraEditor::Create - Game library observer created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Copy game library content to loaded library:
			OS::Path::IterateDirectory(GAME_LIBRARY_DIRECTORY, [&](const OS::Path& path) {
				std::string ext = OS::Path(path.extension());
				for (size_t i = 0; i < ext.length(); i++) ext[i] = std::tolower(ext[i]);
				if ((ext != OS::DynamicLibrary::FileExtension()) && (ext != ".spv") && (ext != ".json")) return true;
				const std::string pathStr = path;
				const OS::Path copiedFile = LOADED_LIBRARY_DIRECTORY + (pathStr.c_str() + GAME_LIBRARY_DIRECTORY.length());
				std::error_code fsError;
				std::filesystem::create_directories(copiedFile.parent_path(), fsError);
				if (fsError) logger->Warning("JimaraEditor - Create directories for '", copiedFile, "'! Ignoring the file...");
				else {
					std::filesystem::copy(path, copiedFile, std::filesystem::copy_options::overwrite_existing, fsError);
					if (fsError)
						logger->Warning("JimaraEditor - Failed to copy '", path, "' (", fsError.message(), ")! Ignoring the file...");
				}
				return true;
				});

			// Shader loader: 
			// __TODO__: This is not completely safe for reloading... We need to do something about this...
			const Reference<EditorShaderLibrary> shaderLibrary = Object::Instantiate<EditorShaderLibrary>(); 
			{
				const auto loader = FileSystemShaderLibrary::Create(LOADED_LIBRARY_DIRECTORY + "/Shaders/", logger);
				if (loader == nullptr)
					return error("JimaraEditor::Create - Shader loader could not be created!");
				else shaderLibrary->SetLoader(loader);
			}

			// File system database:
			const Reference<FileSystemDatabase> fileSystemDB = [&]()-> Reference<FileSystemDatabase> {
				std::vector<Reference<Object>> libs;
				OS::Path::IterateDirectory(GAME_LIBRARY_DIRECTORY, [&](const OS::Path& path) {
					const OS::Path extension = [&]() {
						const std::string rawExtension = OS::Path(path.extension());
						std::string rv;
						for (size_t i = 0; i < rawExtension.length(); i++)
							rv += std::tolower(rawExtension[i]);
						return rv;
						}();
						if (path.extension() == OS::DynamicLibrary::FileExtension())
							libs.push_back(OS::DynamicLibrary::Load(path, logger));
						return true;
					});
				return FileSystemDatabase::Create(
					graphicsDevice, shaderLibrary, physics, audio, "Assets/", [&](size_t processed, size_t total) {
						static thread_local Stopwatch stopwatch;
						if (stopwatch.Elapsed() > 0.5f) {
							stopwatch.Reset();
							logger->Info("FileSystemDatabase - Files processed: ", processed, '/', total,
								" (", (static_cast<float>(processed) / static_cast<float>(total) * 100.0f), "%)", processed == total ? "" : "...");
						}
					}, "JimaraDatabaseCache.json");
			}();
			if (fileSystemDB == nullptr)
				return error("JimaraEditor::Create - Failed to create FileSystemDatabase!");
			logger->Info("JimaraEditor::Create - FileSystemDatabase created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor context:
			const Reference<EditorContext> editorContext = new EditorContext(logger, graphicsDevice, physics, audio, inputModule, fileSystemDB, shaderLibrary, window);
			if (editorContext == nullptr)
				return error("JimaraEditor::Create - Failed to create editor context!");
			else editorContext->ReleaseRef();
			logger->Info("JimaraEditor::Create - Editor context created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

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
			logger->Info("JimaraEditor::Create - Editor renderer created! [Time: ", stopwatch.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Editor instance:
			const Reference<JimaraEditor> editor = new JimaraEditor(
				std::move(registries), editorContext, renderEngine, editorRenderer, gameLibraryObserver);
			if (editor == nullptr)
				return error("JimaraEditor::Create - Failed to create editor instance!");
			else {
				editor->ReleaseRef();
				return editor;
			}
		}

		JimaraEditor::~JimaraEditor() {
			std::unique_lock<std::mutex> lock(m_updateLock);
			m_gameLibraryObserver->OnFileChanged() -= Callback(&JimaraEditor::OnGameLibraryUpdated, this);
			m_context->m_window->OnUpdate() -= Callback<OS::Window*>(&JimaraEditor::OnUpdate, this);
			m_renderEngine->RemoveRenderer(m_renderer);
			EditorDataSerializer::Store(m_editorStorage, m_context);
			{
				std::unique_lock<SpinLock> lock(m_context->m_editorLock);
				m_context->m_editor = nullptr;
			}
			{
				m_jobs.~JobSystem();
				new(&m_jobs)JobSystem(1);
			}
			m_scene = nullptr;
			m_undoManager = nullptr;
			m_undoActions.clear();
			m_editorStorage.clear();
			m_gameLibraries.clear();
		}

		void JimaraEditor::WaitTillClosed()const {
			m_context->m_window->WaitTillClosed();
		}

		JimaraEditor::JimaraEditor(
			std::vector<Reference<Object>>&& typeRegistries, EditorContext* context,
			Graphics::RenderEngine* renderEngine, Graphics::ImageRenderer* renderer, 
			OS::DirectoryChangeObserver* gameLibraryObserver)
			: m_typeRegistries(std::move(typeRegistries)), m_context(context)
			, m_renderEngine(renderEngine), m_renderer(renderer)
			, m_gameLibraryObserver(gameLibraryObserver) {
			if (m_context != nullptr) {
				std::unique_lock<SpinLock> lock(m_context->m_editorLock);
				m_context->m_editor = this;
			}
			{
				m_gameLibraryObserver->OnFileChanged() += Callback(&JimaraEditor::OnGameLibraryUpdated, this);
				OnGameLibraryUpdated({});
			}
			m_renderEngine->AddRenderer(m_renderer);
			m_context->m_window->OnUpdate() += Callback<OS::Window*>(&JimaraEditor::OnUpdate, this);
		}

		void JimaraEditor::OnUpdate(OS::Window*) {
			std::unique_lock<std::mutex> lock(m_updateLock);
			m_renderEngine->Update();
			m_context->m_onMainLoopUpdate();
		}

		void JimaraEditor::OnGameLibraryUpdated(const OS::DirectoryChangeObserver::FileChangeInfo& info) {
			auto getCopiedPath = [&](const OS::Path& path) {
				const std::string pathStr = path;
#ifndef NDEBUG
				if (pathStr.find(GAME_LIBRARY_DIRECTORY) != 0)
					m_context->Log()->Error("JimaraEditor - '", path, "' expected to start with '", GAME_LIBRARY_DIRECTORY, "'!");
#endif // NDEBUG
				return LOADED_LIBRARY_DIRECTORY + (pathStr.c_str() + GAME_LIBRARY_DIRECTORY.length());
			};
			
			Stopwatch timer;
			Stopwatch totalTime;

			// Make sure we need to respond to this update:
			if (info.changeType != OS::DirectoryChangeObserver::FileChangeType::NO_OP) {
				if (info.filePath.extension() != OS::DynamicLibrary::FileExtension()) 
					return;
				else {
					Stopwatch timer;
					const constexpr float TIMEOUT = 4.0f;
					while (std::filesystem::exists(info.filePath)) {
						const Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(info.filePath);
						const Reference<OS::MMappedFile> copiedMapping = OS::MMappedFile::Create(getCopiedPath(info.filePath));
						if (mapping != nullptr) {
							if (copiedMapping != nullptr) {
								const MemoryBlock srcBlock(*mapping);
								const MemoryBlock dstBlock(*copiedMapping);
								if (srcBlock.Size() == dstBlock.Size() && std::memcmp(srcBlock.Data(), dstBlock.Data(), srcBlock.Size()) == 0) return;
							}
							break;
						}
						else if (timer.Elapsed() > TIMEOUT) {
							m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - Failing to read '", info.filePath, "'! (Ignoring changes)");
							return;
						}
						else std::this_thread::sleep_for(std::chrono::milliseconds(5));
					}
				}
				m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - File checked [Time: ", timer.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");
			}

			std::unique_lock<std::mutex> lock(m_updateLock);
			// Store state:
			if (info.changeType != OS::DirectoryChangeObserver::FileChangeType::NO_OP) {
				m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - Reloading game library!");
				EditorDataSerializer::Store(m_editorStorage, m_context);
				m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - State stored [", info, "] ",
					"[Time: ", timer.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");
			}
			
			// Clear state
			std::unordered_map<GUID, Reference<Resource>> resources;
			{
				if (m_scene != nullptr) {
					std::unique_lock<std::recursive_mutex> lock(m_scene->UpdateLock());
					ComponentHierarchySerializerInput input = {};
					input.rootComponent = m_scene->RootObject();
					auto clearContext = [&]() { input.context = nullptr; };
					input.onResourcesLoaded = Callback<>::FromCall(&clearContext);
					bool error = false;
					Serialization::SerializeToJson(ComponentHierarchySerializer::Instance()->Serialize(input), m_context->Log(), error,
						[&](const Serialization::SerializedObject&, bool&) { return nlohmann::json(); });
					for (size_t i = 0; i < input.resources.size(); i++) {
						const auto& resource = input.resources[i];
						if (resource->HasAsset()) resources[resource->GetAsset()->Guid()] = resource;
					}
				}
				auto onResourceCollectionChanged = [&](FileSystemDatabase::DatabaseChangeInfo info) { resources.erase(info.assetGUID); };
				m_context->EditorAssetDatabase()->OnDatabaseChanged() += Callback<FileSystemDatabase::DatabaseChangeInfo>::FromCall(&onResourceCollectionChanged);
				m_scene = nullptr;
				{
					m_jobs.~JobSystem();
					new(&m_jobs)JobSystem(1);
				}
				m_undoManager = Object::Instantiate<UndoStack>();
				m_undoActions.clear();
				m_editorStorage.clear();
				dynamic_cast<EditorShaderLibrary*>(m_context->m_shaderLibrary.operator->())->SetLoader(nullptr);
				m_gameLibraries.clear();
				m_context->EditorAssetDatabase()->OnDatabaseChanged() -= Callback<FileSystemDatabase::DatabaseChangeInfo>::FromCall(&onResourceCollectionChanged);
				m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - State cleared [Time: ", timer.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");
			};

			// Reload libs:
			OS::Path::IterateDirectory(GAME_LIBRARY_DIRECTORY, [&](const OS::Path& path) {
				const OS::Path extension = [&]() {
					const std::string rawExtension = OS::Path(path.extension());
					std::string rv;
					for (size_t i = 0; i < rawExtension.length(); i++)
						rv += std::tolower(rawExtension[i]);
					return rv;
				}();
				const bool isLibrary = (path.extension() == OS::DynamicLibrary::FileExtension());
				if ((!isLibrary) && (extension != ".spv") && (extension != ".json")) return true;
				const OS::Path copiedFile = getCopiedPath(path);
				{
					std::error_code fsError;
					std::filesystem::create_directories(copiedFile.parent_path(), fsError);
					if (fsError) {
						m_context->Log()->Warning("JimaraEditor - Create directories for '", copiedFile, "'! Ignoring the file...");
						return true;
					}
					std::filesystem::copy(path, copiedFile, std::filesystem::copy_options::overwrite_existing, fsError);
					if (fsError) {
						m_context->Log()->Warning("JimaraEditor - Failed to copy '", path, "' (", fsError.message(), ")! Ignoring the file...");
						return true;
					}
				}
				if (isLibrary) {
					Reference<OS::DynamicLibrary> library = OS::DynamicLibrary::Load(copiedFile, m_context->Log());
					if (library == nullptr) m_context->Log()->Warning("JimaraEditor - Failed to load '", copiedFile, "'! Ignoring the file...");
					else m_gameLibraries.push_back(library);
				}
				return true;
				});
			m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - Libraries reloaded [Time: ", timer.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");

			// Recreate shader loader:
			{
				auto loader = FileSystemShaderLibrary::Create(LOADED_LIBRARY_DIRECTORY + "/Shaders/", m_context->Log());
				if (loader == nullptr)
					return m_context->Log()->Fatal("JimaraEditor::OnGameLibraryUpdated - Failed to create shader binary loader!");
				else dynamic_cast<EditorShaderLibrary*>(m_context->m_shaderLibrary.operator->())->SetLoader(loader);
			}

			// Reload stuff:
			EditorDataSerializer::Load(m_editorStorage, m_context);
			m_context->Log()->Info("JimaraEditor::OnGameLibraryUpdated - State restored [Time: ", timer.Reset(), "; Elapsed: ", totalTime.Elapsed(), "]");
		}


		EditorMainMenuAction::EditorMainMenuAction(const std::string_view& menuPath, const std::string_view& tooltip) 
			: m_path(menuPath), m_tooltip(tooltip) {}

		const std::string& EditorMainMenuAction::MenuPath()const { return m_path; }

		const std::string& EditorMainMenuAction::Tooltip()const { return m_tooltip; }


		void EditorMainMenuAction::GetAll(Callback<const EditorMainMenuAction*> recordEntry) {
			using ActionList = std::vector<Reference<const EditorMainMenuAction>>;
			static std::shared_ptr<ActionList> registeredActions;
			static std::recursive_mutex actionRegistryLock;

			struct RegistrySubscription : public virtual Object {
				inline static void OnRegisteredTypeSetChanged() {
					std::unique_lock<std::recursive_mutex> creationLock(actionRegistryLock);
					registeredActions = nullptr;
				}
				inline RegistrySubscription() {
					TypeId::OnRegisteredTypeSetChanged() += Callback(RegistrySubscription::OnRegisteredTypeSetChanged);
				}
				inline virtual ~RegistrySubscription() {
					TypeId::OnRegisteredTypeSetChanged() -= Callback(RegistrySubscription::OnRegisteredTypeSetChanged);
				}
			};
			static Reference<RegistrySubscription> registrySubscription;

			std::shared_ptr<ActionList> actions;
			{
				std::unique_lock<decltype(actionRegistryLock)> lock(actionRegistryLock);
				if (registeredActions == nullptr) {
					if (registrySubscription == nullptr)
						registrySubscription = Object::Instantiate<RegistrySubscription>();

					std::unordered_set<Reference<const EditorMainMenuAction>> actionsFromRegistry;
					const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
					for (size_t i = 0; i < currentTypes->Size(); i++) {
						auto checkAttribute = [&](const Object* attribute) {
							const EditorMainMenuAction* action = dynamic_cast<const EditorMainMenuAction*>(attribute);
							if (action != nullptr)
								actionsFromRegistry.insert(action);
						};
						currentTypes->At(i).GetAttributes(Callback<const Object*>::FromCall(&checkAttribute));
					}

					registeredActions = std::make_shared<ActionList>();
					for (auto it = actionsFromRegistry.begin(); it != actionsFromRegistry.end(); ++it)
						registeredActions->push_back(*it);

					std::sort(registeredActions->begin(), registeredActions->end(), [](
						const Reference<const EditorMainMenuAction>& a, const Reference<const EditorMainMenuAction>& b) {
							if (a->MenuPath() < b->MenuPath())
								return true;
							else if (a->MenuPath() > b->MenuPath())
								return false;
							else return a < b;
						});
				}
				actions = registeredActions;
			}

			for (size_t i = 0u; i < actions->size(); i++)
				recordEntry(actions->operator[](i).operator->());
		}


		EditorMainMenuCallback::EditorMainMenuCallback(const std::string_view& menuPath, const std::string_view& tooltip, const Callback<EditorContext*>& action)
			: EditorMainMenuAction(menuPath, tooltip), m_action(action) {}

		void EditorMainMenuCallback::Execute(EditorContext* context)const { m_action(context); }
	}
}
