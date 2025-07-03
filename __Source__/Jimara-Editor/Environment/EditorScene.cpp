#include "EditorScene.h"
#include "../ActionManagement/SceneUndoManager.h"
#include "../ActionManagement/HotKey.h"
#include <Jimara/Core/Stopwatch.h>
#include <Jimara/Data/Serialization/Helpers/ComponentHierarchySerializer.h>
#include <Jimara/OS/IO/FileDialogues.h>
#include <Jimara/OS/IO/MMappedFile.h>
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <fstream>


namespace Jimara {
	namespace Editor {
		namespace {
			inline static void LoadSceneDialogue(EditorScene* scene);
			inline static void SaveSceneDialogue(EditorScene* scene);
			inline static void SaveScene(EditorScene* scene);

			class EditorSceneUpdateJob : public virtual JobSystem::Job {
			public:
				const Reference<EditorContext> context;
				const Reference<EditorInput> input;
				const Reference<Scene> scene;
				Reference<RenderStack> renderStack;
				Reference<SceneUndoManager> undoManager;
				Reference<SceneSelection> selection;
				Reference<SceneClipboard> clipboard;

				struct EditorSceneUpdateThread {
					std::thread updateThread;
					struct State {
						std::atomic<bool> stopped = true;
						std::atomic<bool> interrupted = false;
						std::atomic<bool> paused = true;

						Stopwatch timer;
					};
					const std::shared_ptr<State> state = std::make_shared<State>();
					Reference<EditorContext> editorContext;
					Reference<Scene> targetScene;


					inline static void Update(Scene* scene, State* state) {
						if (state->interrupted) std::this_thread::sleep_for(std::chrono::nanoseconds(1));
						else {
							scene->Context()->UpdateLock().lock();
							auto unlock = [&](Object*) { scene->Context()->UpdateLock().unlock(); };
							scene->Context()->ExecuteAfterUpdate(Callback<Object*>::FromCall(&unlock), nullptr);
							if (state->paused) scene->SynchAndRender(state->timer.Reset());
							else scene->Update(state->timer.Reset());
						}
					}

					inline void UpdateTargetScene() { Update(targetScene, state.operator->()); }

					inline void Stop() {
						if (state->stopped) return;
						state->stopped = true;
						if (updateThread.joinable())
							updateThread.join();
						if (editorContext != nullptr) {
							editorContext->OnMainLoop() -= Callback<>(&EditorSceneUpdateThread::UpdateTargetScene, this);
							editorContext = nullptr;
						}
						targetScene = nullptr;
					}

					// Note: context is optional; if provided, main update loop will be used instead of a custom thread (will make life easier on lower-end hardware)
					inline void Start(Scene* scene, EditorContext* context) {
						Stop();
						state->stopped = false;
						targetScene = scene;
						editorContext = context;
						if (editorContext != nullptr) {
							state->timer.Reset();
							editorContext->OnMainLoop() += Callback<>(&EditorSceneUpdateThread::UpdateTargetScene, this);
						}
						else {
							const std::shared_ptr<Semaphore> lock = std::make_shared<Semaphore>();
							updateThread = std::thread([](Scene* scene, std::shared_ptr<State> state, std::shared_ptr<Semaphore> semaphore) {
								Reference<Scene> sceneRef = scene;
								semaphore->post();
								semaphore = nullptr;
								state->timer.Reset();
								while (!state->stopped) {
									if (state->interrupted || state->timer.Elapsed() <= 0.00001f)
										std::this_thread::sleep_for(std::chrono::nanoseconds(1));
									else Update(scene, state.operator->());
									std::this_thread::yield();
								}
								}, scene, state, lock);
							lock->wait();
						}
					}
				} updateThread;
				
				Size2 lastRequestedSize = Size2(0, 0);
				size_t sameRequestedSizeCount = 0;
				Size2 requestedSize = Size2(1, 1);
				std::optional<Vector3> offsetAndSize;
				
				nlohmann::json sceneSnapshot;

			private:
				std::optional<OS::Path> m_assetPath;

			public:
				const inline std::optional<OS::Path>& AssetPath()const { return m_assetPath; }
				void SetAssetPath(const std::optional<OS::Path>& path) {
					if (path.has_value()) {
						std::error_code error;
						const OS::Path& assetDir = context->EditorAssetDatabase()->AssetDirectory();
						m_assetPath = assetDir / std::filesystem::relative(path.value(), assetDir, error);
						if (error) 
							m_assetPath = path.value();
					}
					else m_assetPath = path;
				}

				SpinLock editorSceneLock;
				EditorScene* editorScene = nullptr;

				struct UpdateJobs : public virtual Object {
					const Reference<EditorSceneUpdateJob> owner;
					SynchronousActionQueue<EditorScene*> jobQueue;

					inline UpdateJobs(EditorSceneUpdateJob* ownerPtr) : owner(ownerPtr) {}
					inline virtual ~UpdateJobs() {}
					inline virtual void OnOutOfScope()const override {
						{
							std::unique_lock<SpinLock> lock(owner->updateJobs.lock);
							if (RefCount() > 0) return;
							else owner->updateJobs.ref = 0;
						}
						Object::OnOutOfScope();
					}
				};
				struct UpdateJobsRef {
					SpinLock lock;
					UpdateJobs* ref = nullptr;

					inline operator Reference<UpdateJobs>() {
						std::unique_lock<SpinLock> refLock(lock);
						Reference<UpdateJobs> reference = ref;
						return reference;
					}
				} updateJobs;

				Reference<EditorScene> GetEditorScene() {
					std::unique_lock<SpinLock> lock(editorSceneLock);
					Reference<EditorScene> rv = editorScene;
					return rv;
				}

				void DiscardUndoManager() {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					if (undoManager == nullptr) return;
					undoManager->Discard();
					undoManager = nullptr;
				}

				void CreateUndoManager() {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					DiscardUndoManager();
					assert(selection != nullptr);
					undoManager = Object::Instantiate<SceneUndoManager>(selection);
				}

				inline void SaveIfNeedBe() {
					if (HotKey::Save().Check(context->InputModule())) {
						Reference<EditorScene> editorScene = GetEditorScene();
						if (editorScene != nullptr)
							SaveScene(editorScene);
					}
				}

				inline EditorSceneUpdateJob(EditorContext* context, EditorInput* inputModule)
					: context(context)
					, input(inputModule)
					, scene([&]() -> Reference<Scene> {
					Scene::CreateArgs args;
					{
						args.logic.logger = context->Log();
						args.logic.input = inputModule;
						args.logic.assetDatabase = context->EditorAssetDatabase();
					}
					{
						args.graphics.graphicsDevice = context->GraphicsDevice();
						args.graphics.bindlessResources.bindlessArrays = context->BindlessBuffers();
						args.graphics.bindlessResources.bindlessSamplers = context->BindlessSamplers();
						args.graphics.shaderLibrary = context->ShaderLibrary();
					}
					{
						args.physics.physicsInstance = context->PhysicsInstance();
					}
					{
						args.audio.audioDevice = context->AudioDevice();
					}
					args.createMode = Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS;
					return Scene::Create(args);
						}()) {
					renderStack = RenderStack::Main(scene->Context());
					selection = Object::Instantiate<SceneSelection>(scene->Context());
					clipboard = Object::Instantiate<SceneClipboard>(scene->Context());
					updateThread.Start(scene, context);
					CreateUndoManager();

					Reference<UpdateJobs> updates = Object::Instantiate<UpdateJobs>(this);
					updateJobs.ref = updates;
					context->AddStorageObject(updates);
				}

				inline virtual ~EditorSceneUpdateJob() {
					updateThread.Stop();
					Reference<UpdateJobs> updates = updateJobs;
					context->RemoveStorageObject(updates);
					DiscardUndoManager();
					clipboard = nullptr;
					selection = nullptr;
				}

				inline virtual void Execute() final override {
					// We need this lock for most everything...
					const bool wasInterrupted = updateThread.state->interrupted.exchange(true);
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					updateThread.state->interrupted = wasInterrupted;

					// Execute scheduled jobs:
					{
						Reference<UpdateJobs> jobs = updateJobs;
						Reference<EditorScene> scene = GetEditorScene();
						if (jobs != nullptr) jobs->jobQueue.Flush(scene);
					}

					// Save is requested:
					SaveIfNeedBe();

					// Record undo actions:
					if (undoManager != nullptr) {
						Reference<UndoStack::Action> action = undoManager->Flush();
						context->AddUndoAction(action);
					}

					// Set input settings:
					if (offsetAndSize.has_value()) {
						input->SetEnabled(true);
						input->SetMouseOffset(offsetAndSize.value());
						input->SetMouseScale(offsetAndSize.value().z);
					}
					else input->SetEnabled(false);

					// Resize target texture:
					if (lastRequestedSize == requestedSize) {
						if (sameRequestedSizeCount > scene->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount())
							renderStack->SetResolution(requestedSize);
						else sameRequestedSizeCount++;
					}
					else {
						renderStack->SetResolution(Size2(0u));
						sameRequestedSizeCount = 0;
					}
					lastRequestedSize = requestedSize;
					requestedSize = Size2(0, 0);
				}
				inline virtual void CollectDependencies(Callback<Job*>) final override {}

				inline nlohmann::json CreateSnapshot()const {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					ComponentHierarchySerializerInput input;
					input.rootComponent = scene->Context()->RootObject();
					bool error = false;
					nlohmann::json result = Serialization::SerializeToJson(
						ComponentHierarchySerializer::Instance()->Serialize(input), context->Log(), error,
						[&](const Serialization::SerializedObject&, bool& error) -> nlohmann::json {
							context->Log()->Error("JimaraEditorScene::Job::CreateSnapshot - ComponentHierarchySerializer is not expected to have any Component references!");
							error = true;
							return {};
						});
					if (error)
						context->Log()->Error("JimaraEditorScene::Job::CreateSnapshot - Failed to create scene snapshot!");
					return result;
				}

				inline void LoadSnapshot(const nlohmann::json& snapshot) {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					ComponentHierarchySerializerInput input;
					input.rootComponent = scene->Context()->RootObject();
					Stopwatch totalTime;
					Stopwatch clock;
					auto reportProgress = [&](const ComponentHierarchySerializer::ProgressInfo& info) {
						if (totalTime.Elapsed() < 0.25f || (info.stepsTaken != info.totalSteps && clock.Elapsed() < 0.25f))
							return;
						clock.Reset();
						context->Log()->Info("JimaraEditorScene::Job::LoadSnapshot - ",
							(float(info.stepsTaken) / info.totalSteps * 100.0f), "% (",
							info.stepsTaken, "/", info.totalSteps, "; ",
							totalTime.Elapsed(), " seconds elapsed)");
					};
					input.reportProgress = Callback<ComponentHierarchySerializer::ProgressInfo>::FromCall(&reportProgress);
					if (!Serialization::DeserializeFromJson(
						ComponentHierarchySerializer::Instance()->Serialize(input), snapshot, context->Log(),
						[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
							context->Log()->Error("JimaraEditorScene::Job::LoadSnapshot - ComponentHierarchySerializer is not expected to have any Component references!");
							return false;
						}))
						context->Log()->Error("JimaraEditorScene::Job::LoadSnapshot - Failed to load scene snapshot!");
				}
			};
		}

		EditorScene::EditorScene(EditorContext* editorContext)
			: m_editorContext(editorContext)
			, m_updateJob([&]() {
			const Reference<EditorInput> input = editorContext->CreateInputModule();
			return Object::Instantiate<EditorSceneUpdateJob>(editorContext, input);
				}()) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			{
				std::unique_lock<SpinLock> lock(job->editorSceneLock);
				job->editorScene = this;
			}
			m_editorContext->AddRenderJob(m_updateJob);
			m_editorContext->EditorAssetDatabase()->OnDatabaseChanged() += Callback(&EditorScene::OnFileSystemDBChanged, this);
		}

		EditorScene::~EditorScene() {
			m_editorContext->RemoveRenderJob(m_updateJob);
			m_editorContext->EditorAssetDatabase()->OnDatabaseChanged() -= Callback(&EditorScene::OnFileSystemDBChanged, this);
		}

		void EditorScene::OnOutOfScope()const {
			AddRef();
			{
				EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
				{
					Reference<EditorScene> self = job->GetEditorScene();
					if (self == this)
						m_editorContext->EditorAssetDatabase()->OnDatabaseChanged() -= Callback(&EditorScene::OnFileSystemDBChanged, self.operator->());
				}
				m_editorContext->RemoveRenderJob(m_updateJob);
				{
					Reference<EditorSceneUpdateJob::UpdateJobs> updateJobs;
					{
						std::unique_lock<SpinLock> lock(job->updateJobs.lock);
						updateJobs = job->updateJobs.ref;
						job->updateJobs.ref = nullptr;
					}
					m_editorContext->RemoveStorageObject(updateJobs);
				}
				{
					std::unique_lock<SpinLock> lock(job->editorSceneLock);
					job->editorScene = nullptr;
				}
			}
			if (RefCount() <= 1) 
				Object::OnOutOfScope();
			else ReleaseRef();
		}

		Component* EditorScene::RootObject()const { return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->scene->RootObject(); }

		EditorContext* EditorScene::Context()const { return m_editorContext; }

		void EditorScene::RequestResolution(Size2 size) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			if (job->requestedSize.x < size.x) job->requestedSize.x = size.x;
			if (job->requestedSize.y < size.y) job->requestedSize.y = size.y;
		}

		void EditorScene::RequestInputOffsetAndScale(Vector2 offset, float scale) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			job->offsetAndSize = Vector3(offset, scale);
		}

		std::recursive_mutex& EditorScene::UpdateLock()const {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->scene->Context()->UpdateLock();
		}

		void EditorScene::Play() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (m_playState == PlayState::PLAYING) return;
			else if (m_playState == PlayState::STOPPED)
				job->sceneSnapshot = job->CreateSnapshot();
			{
				typedef void(*EnableFn)(Object*);
				static const EnableFn enableFn = [](Object* selfPtr) {
					static const EnableFn enableInOneMoreFrame = [](Object* selfPtr) {
						EditorScene* self = dynamic_cast<EditorScene*>(selfPtr);
						EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(self->m_updateJob.operator->());
						job->updateThread.state->paused = (self->m_playState != PlayState::PLAYING);
					};
					dynamic_cast<EditorSceneUpdateJob*>(dynamic_cast<EditorScene*>(selfPtr)->m_updateJob.operator->())
						->scene->Context()->ExecuteAfterUpdate(Callback(enableInOneMoreFrame), selfPtr);
				};
				job->scene->Context()->ExecuteAfterUpdate(Callback(enableFn), this);
			}
			job->DiscardUndoManager();
			m_playState = PlayState::PLAYING;
			m_onStateChange(m_playState, this);
		}

		void EditorScene::Pause() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (m_playState == PlayState::PAUSED) return;
			job->updateThread.state->paused = true;
			m_playState = PlayState::PAUSED;
			m_onStateChange(m_playState, this);
		}

		void EditorScene::Stop() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (m_playState == PlayState::STOPPED) return;
			job->updateThread.state->paused = true;
			job->LoadSnapshot(job->sceneSnapshot);
			job->CreateUndoManager();
			m_playState = PlayState::STOPPED;
			// TODO: Reset scene timers...
			m_onStateChange(m_playState, this);
		}

		EditorScene::PlayState EditorScene::State()const { return m_playState; }

		Event<EditorScene::PlayState, const EditorScene*>& EditorScene::OnStateChange()const { return m_onStateChange; }

		std::optional<OS::Path> EditorScene::AssetPath()const {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->AssetPath();
		}
		
		bool EditorScene::Load(const OS::Path& assetPath) {
			const Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(assetPath, m_editorContext->Log());
			if (mapping == nullptr) {
				m_editorContext->Log()->Error("EditorScene::Load - Could not open file: \"", assetPath, "\"!");
				return false;
			}
			nlohmann::json json;
			try {
				MemoryBlock block(*mapping);
				json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
			}
			catch (nlohmann::json::parse_error& err) {
				m_editorContext->Log()->Error("EditorScene::Load - Could not parse file: \"", assetPath, "\"! [Error: <", err.what(), ">]");
				return false;
			}
			{
				EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
				const bool wasInterrupted = job->updateThread.state->interrupted.exchange(true);
				std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
				job->updateThread.state->interrupted = wasInterrupted;
				if (m_playState == PlayState::STOPPED) {
					job->LoadSnapshot(json);
					job->CreateUndoManager();
				}
				else job->sceneSnapshot = json;
				job->SetAssetPath(assetPath);
			}
			return true;
		}

		bool EditorScene::Clear() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			Stop();
			Reference<Component> rootComponent = job->scene->RootObject();
			for (size_t i = rootComponent->ChildCount(); i > 0u; i--)
				rootComponent->GetChild(i - 1)->Destroy();
			job->CreateUndoManager();
			job->SetAssetPath(std::optional<OS::Path>());
			return true;
		}

		bool EditorScene::Reload() {
			std::optional<OS::Path> path = AssetPath();
			if (path.has_value()) return Load(path.value());
			else {
				m_editorContext->Log()->Error("EditorScene::Reload - Scene not loaded from a file!");
				return false;
			}
		}

		bool EditorScene::Save() {
			std::optional<OS::Path> path = AssetPath();
			if (path.has_value()) return SaveAs(path.value());
			else {
				m_editorContext->Log()->Error("EditorScene::Save - Scene not tied to a file!");
				return false;
			}
		}

		bool EditorScene::SaveAs(const OS::Path& assetPath) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::ofstream fileStream((const std::filesystem::path&)assetPath);
			if ((!fileStream.is_open()) || (fileStream.bad())) {
				m_editorContext->Log()->Error("EditorScene::SaveAs - Could not open \"", assetPath, "\" for writing!");
				return false;
			}
			nlohmann::json snapshot;
			{
				std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
				if (m_playState == PlayState::STOPPED)
					snapshot = job->CreateSnapshot();
				else snapshot = job->sceneSnapshot;
			}
			fileStream << snapshot.dump(1, '\t') << std::endl;
			fileStream.close();
			job->SetAssetPath(assetPath);
			return true;
		}

		void EditorScene::TrackComponent(Component* component, bool trackChildren) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (job->undoManager != nullptr) job->undoManager->TrackComponent(component, trackChildren);
		}

		SceneSelection* EditorScene::Selection() {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->selection;
		}

		SceneClipboard* EditorScene::Clipboard() {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->clipboard;
		}

		void EditorScene::ExecuteOnImGuiThread(const Callback<Object*, EditorScene*>& callback, Object* object) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			Reference<EditorSceneUpdateJob::UpdateJobs> updateJobs = job->updateJobs;
			if (updateJobs != nullptr) updateJobs->jobQueue.Schedule(callback, object);
		}

		namespace {
			struct ResourceUpdateInfo {
				FileSystemDatabase* database = nullptr;
				const FileSystemDatabase::DatabaseChangeInfo* updateInfo = nullptr;
			};

			inline static void UpdateResourceField(const ResourceUpdateInfo* info, Serialization::SerializedObject field) {
				const Serialization::ObjectReferenceSerializer* referenceSerializer = field.As<Serialization::ObjectReferenceSerializer>();
				if (referenceSerializer != nullptr) {
					Object* value = referenceSerializer->GetObjectValue(field.TargetAddr());
					Resource* resource = dynamic_cast<Resource*>(value);
					Asset* asset;
					if (resource != nullptr)
						asset = resource->GetAsset();
					else asset = nullptr;
					
					if (asset == nullptr) {
						resource = nullptr;
						asset = dynamic_cast<Asset*>(value);
					}
					if (asset != nullptr) {
						asset->RefreshExternalDependencies();
						if (asset->Guid() == info->updateInfo->assetGUID) {
							Reference<Asset> newAsset = info->database->FindAsset(info->updateInfo->assetGUID);
							if (resource == nullptr)
								field.SetObjectValue(newAsset);
							else if (newAsset != nullptr) {
								Reference<Resource> newResource = newAsset->LoadResource();
								field.SetObjectValue(newResource);
							}
							else field.SetObjectValue(nullptr);
						}
					}
				}
				else if (field.As<Serialization::SerializerList>() != nullptr)
					field.GetFields(Callback<Serialization::SerializedObject>(UpdateResourceField, info));
			}

			inline static void UpdateResourceReference(
				Component* component, 
				const ResourceUpdateInfo* info) {
				if (component == nullptr) return;
				for (size_t i = 0; i < component->ChildCount(); i++)
					UpdateResourceReference(component->GetChild(i), info);
				component->GetFields(Callback<Serialization::SerializedObject>(UpdateResourceField, info));
			}
		}

		void EditorScene::OnFileSystemDBChanged(FileSystemDatabase::DatabaseChangeInfo info) {
			ResourceUpdateInfo resourceInfo = {};
			{
				resourceInfo.database = m_editorContext->EditorAssetDatabase();
				resourceInfo.updateInfo = &info;
			}
			std::unique_lock<std::recursive_mutex> lock(RootObject()->Context()->UpdateLock());
			UpdateResourceReference(RootObject(), &resourceInfo);
		}



		namespace {
			inline static const constexpr char* Extension() {
				return ".jimara";
			}
			static const std::vector<OS::FileDialogueFilter> SCENE_EXTENSION_FILTER = {
				OS::FileDialogueFilter(std::string("Jimara scene (") + Extension() + ")", { std::string("*") + Extension() }) };

			inline static Reference<EditorScene> GetOrCreateMainScene(EditorContext* context) {
				Reference<EditorScene> scene = context->GetScene();
				if (scene == nullptr) {
					scene = Object::Instantiate<EditorScene>(context);
					context->SetScene(scene);
				}
				return scene;
			}

			inline static std::optional<OS::Path> GetLoadedAssetAbsPath(EditorScene * scene) {
				const std::optional<OS::Path> loadedPath = scene->AssetPath();
				if (loadedPath.has_value()) {
					std::error_code error;
					std::filesystem::path path = std::filesystem::absolute(loadedPath.value(), error);
					return (error ? loadedPath : path);
				}
				else return std::filesystem::current_path();
			}

			inline static void LoadSceneDialogue(EditorScene* scene) {
				std::vector<OS::Path> result = OS::OpenDialogue("Open Scene", GetLoadedAssetAbsPath(scene), SCENE_EXTENSION_FILTER);
				if (result.size() > 0)
					scene->Load(result[0]);
			}
			inline static void SaveSceneDialogue(EditorScene* scene) {
				std::optional<OS::Path> initialPath = GetLoadedAssetAbsPath(scene);
				std::optional<OS::Path> assetPath = OS::SaveDialogue("Save Scene",
					initialPath.has_value() ? initialPath.value()
					: OS::Path(scene->Context()->EditorAssetDatabase()->AssetDirectory() / OS::Path(std::string("Scene") + Extension())), SCENE_EXTENSION_FILTER);
				if (assetPath.has_value()) {
					if (!assetPath.value().has_extension())
						assetPath.value().replace_extension(Extension());
					scene->SaveAs(assetPath.value());
				}
			}
			inline static void SaveScene(EditorScene* scene) {
				if (scene->AssetPath().has_value()) scene->Save();
				else SaveSceneDialogue(scene);
			}

			static const EditorMainMenuCallback loadSceneCallback(
				"Scene/" ICON_FA_FOLDER " Load", "Load scene from file", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					LoadSceneDialogue(scene);
					}));
			static const EditorMainMenuCallback clearSceneAsCallback(
				"Scene/" ICON_FA_FILES_O " New Scene", "Create blank scene", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					scene->Clear();
					}));
			static const EditorMainMenuCallback saveSceneAsCallback(
				"Scene/" ICON_FA_FLOPPY_O " Save As", "Save scene to a custom file", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					SaveSceneDialogue(scene);
					}));
			static const EditorMainMenuCallback saveSceneCallback(
				"Scene/" ICON_FA_FLOPPY_O " Save", "Save scene changes", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					SaveScene(scene);
					}));
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::EditorScene>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::EditorScene>(const Callback<const Object*>& report) {
		report(&Editor::loadSceneCallback);
		report(&Editor::clearSceneAsCallback);
		report(&Editor::saveSceneCallback);
		report(&Editor::saveSceneAsCallback);
	}
}
