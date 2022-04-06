#include "EditorScene.h"
#include "../ActionManagement/SceneUndoManager.h"
#include "../ActionManagement/HotKey.h"
#include <Environment/Scene/SceneUpdateLoop.h>
#include <Data/Serialization/Helpers/ComponentHeirarchySerializer.h>
#include <OS/IO/FileDialogues.h>
#include <OS/IO/MMappedFile.h>
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
				Reference<SceneUndoManager> undoManager;
				Reference<SceneUpdateLoop> updateLoop;
				Size2 requestedSize = Size2(1, 1);
				std::optional<Vector3> offsetAndSize;
				nlohmann::json sceneSnapshot;

				std::optional<OS::Path> assetPath;

				SpinLock editorSceneLock;
				EditorScene* editorScene = nullptr;

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
					undoManager = Object::Instantiate<SceneUndoManager>(scene->Context());
				}

				inline void SaveIfNeedBe() {
					if (HotKey::Save().Check(scene->Context()->Input())) {
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
						args.graphics.shaderLoader = context->ShaderBinaryLoader();
						args.graphics.lightSettings.lightTypeIds = context->LightTypes().lightTypeIds;
						args.graphics.lightSettings.perLightDataSize = context->LightTypes().perLightDataSize;
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
					scene->Context()->Graphics()->OnGraphicsSynch() += Callback(&EditorSceneUpdateJob::SaveIfNeedBe, this);
					updateLoop = Object::Instantiate<SceneUpdateLoop>(scene, true);
					CreateUndoManager();
				}

				inline virtual ~EditorSceneUpdateJob() {
					DiscardUndoManager();
				}

				inline virtual void Execute() final override {
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
					const Size2 targetSize = requestedSize;
					requestedSize = Size2(0, 0);
					if ([&]() -> bool {
						const Reference<Graphics::TextureView> texture = scene->Context()->Graphics()->Renderers().TargetTexture();
							if (texture == nullptr) return false;
							const Size3 textureSize = texture->TargetTexture()->Size();
							return (textureSize.x == targetSize.x && textureSize.y == targetSize.y);
						}()) return;
					if (targetSize.x <= 0 || targetSize.y <= 0) {
						scene->Context()->Graphics()->Renderers().SetTargetTexture(nullptr);
						return;
					}
					const Reference<Graphics::Texture> texture = scene->Context()->Graphics()->Device()->CreateMultisampledTexture(
						Graphics::Texture::TextureType::TEXTURE_2D,
						Graphics::Texture::PixelFormat::B8G8R8A8_SRGB,
						Size3(targetSize.x, targetSize.y, 1),
						1,
						Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
					if (texture == nullptr) {
						context->Log()->Error("EditorScene - Failed to create target texture!");
						return;
					}
					const Reference<Graphics::TextureView> textureView = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
					if (textureView == nullptr) {
						context->Log()->Error("EditorScene - Failed to create target texture view!");
						return;
					}
					scene->Context()->Graphics()->Renderers().SetTargetTexture(textureView);
				}
				inline virtual void CollectDependencies(Callback<Job*>) final override {}

				inline nlohmann::json CreateSnapshot()const {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					ComponentHeirarchySerializerInput input;
					input.rootComponent = scene->Context()->RootObject();
					bool error = false;
					nlohmann::json result = Serialization::SerializeToJson(
						ComponentHeirarchySerializer::Instance()->Serialize(input), context->Log(), error,
						[&](const Serialization::SerializedObject&, bool& error) -> nlohmann::json {
							context->Log()->Error("JimaraEditorScene::Job::CreateSnapshot - ComponentHeirarchySerializer is not expected to have any Component references!");
							error = true;
							return {};
						});
					if (error)
						context->Log()->Error("JimaraEditorScene::Job::CreateSnapshot - Failed to create scene snapshot!");
					return result;
				}

				inline void LoadSnapshot(const nlohmann::json& snapshot) {
					std::unique_lock<std::recursive_mutex> lock(scene->Context()->UpdateLock());
					ComponentHeirarchySerializerInput input;
					input.rootComponent = scene->Context()->RootObject();
					if (!Serialization::DeserializeFromJson(
						ComponentHeirarchySerializer::Instance()->Serialize(input), snapshot, context->Log(),
						[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
							context->Log()->Error("JimaraEditorScene::Job::LoadSnapshot - ComponentHeirarchySerializer is not expected to have any Component references!");
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

		EditorScene::~EditorScene() {}

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
			job->updateLoop->Resume();
			job->DiscardUndoManager();
			m_playState = PlayState::PLAYING;
			m_onStateChange(m_playState, this);
		}

		void EditorScene::Pause() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (m_playState == PlayState::PAUSED) return;
			job->updateLoop->Pause();
			m_playState = PlayState::PAUSED;
			m_onStateChange(m_playState, this);
		}

		void EditorScene::Stop() {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (m_playState == PlayState::STOPPED) return;
			job->updateLoop->Pause();
			job->LoadSnapshot(job->sceneSnapshot);
			job->CreateUndoManager();
			m_playState = PlayState::STOPPED;
			// TODO: Reset scene timers...
			m_onStateChange(m_playState, this);
		}

		EditorScene::PlayState EditorScene::State()const { return m_playState; }

		Event<EditorScene::PlayState, const EditorScene*>& EditorScene::OnStateChange()const { return m_onStateChange; }

		std::optional<OS::Path> EditorScene::AssetPath()const {
			return dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->())->assetPath;
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
				std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
				if (m_playState == PlayState::STOPPED) {
					job->LoadSnapshot(json);
					job->CreateUndoManager();
				}
				else job->sceneSnapshot = json;
				job->assetPath = assetPath;
			}
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
			std::ofstream fileStream(assetPath);
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
			job->assetPath = assetPath;
			return true;
		}

		void EditorScene::TrackComponent(Component* component, bool trackChildren) {
			EditorSceneUpdateJob* job = dynamic_cast<EditorSceneUpdateJob*>(m_updateJob.operator->());
			std::unique_lock<std::recursive_mutex> lock(job->scene->Context()->UpdateLock());
			if (job->undoManager != nullptr) job->undoManager->TrackComponent(component, trackChildren);
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
				const ResourceUpdateInfo* info,
				const ComponentSerializer::Set* serializers) {
				if (component == nullptr) return;
				for (size_t i = 0; i < component->ChildCount(); i++)
					UpdateResourceReference(component->GetChild(i), info, serializers);
				const ComponentSerializer* serializer = serializers->FindSerializerOf(component);
				if (serializer != nullptr)
					UpdateResourceField(info, serializer->Serialize(component));
			}
		}

		void EditorScene::OnFileSystemDBChanged(FileSystemDatabase::DatabaseChangeInfo info) {
			const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			ResourceUpdateInfo resourceInfo = {};
			{
				resourceInfo.database = m_editorContext->EditorAssetDatabase();
				resourceInfo.updateInfo = &info;
			}
			std::unique_lock<std::recursive_mutex> lock(RootObject()->Context()->UpdateLock());
			UpdateResourceReference(RootObject(), &resourceInfo, serializers);
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

			inline static void LoadSceneDialogue(EditorScene* scene) {
				std::vector<OS::Path> result = OS::OpenDialogue("Open Scene", scene->AssetPath(), SCENE_EXTENSION_FILTER);
				if (result.size() > 0)
					scene->Load(result[0]);
			}
			inline static void SaveSceneDialogue(EditorScene* scene) {
				std::optional<OS::Path> initialPath = scene->AssetPath();
				std::optional<OS::Path> assetPath = OS::SaveDialogue("Save Scene",
					initialPath.has_value() ? initialPath.value()
					: OS::Path(scene->Context()->EditorAssetDatabase()->AssetDirectory() / OS::Path(std::string("Scene") + Extension())), SCENE_EXTENSION_FILTER);
				if (assetPath.has_value())
					scene->SaveAs(assetPath.value());
			}
			inline static void SaveScene(EditorScene* scene) {
				if (scene->AssetPath().has_value()) scene->Save();
				else SaveSceneDialogue(scene);
			}

			static const EditorMainMenuCallback loadSceneCallback(
				"Scene/" ICON_FA_FOLDER " Load", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					LoadSceneDialogue(scene);
					}));
			static const EditorMainMenuCallback saveSceneAsCallback(
				"Scene/" ICON_FA_FLOPPY_O " Save As", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					SaveSceneDialogue(scene);
					}));
			static const EditorMainMenuCallback saveSceneCallback(
				"Scene/" ICON_FA_FLOPPY_O " Save", Callback<EditorContext*>([](EditorContext* context) {
					Reference<EditorScene> scene = GetOrCreateMainScene(context);
					SaveScene(scene);
					}));
			static EditorMainMenuAction::RegistryEntry loadAction;
			static EditorMainMenuAction::RegistryEntry saveAction;
			static EditorMainMenuAction::RegistryEntry saveAsAction;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::EditorScene>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
	template<> void TypeIdDetails::OnRegisterType<Editor::EditorScene>() {
		Editor::loadAction = &Editor::loadSceneCallback;
		Editor::saveAction = &Editor::saveSceneCallback;
		Editor::saveAsAction = &Editor::saveSceneAsCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::EditorScene>() {
		Editor::loadAction = nullptr;
		Editor::saveAction = nullptr;
		Editor::saveAsAction = nullptr;
	}
}
