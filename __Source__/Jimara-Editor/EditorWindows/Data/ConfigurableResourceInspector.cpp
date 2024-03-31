#include "ConfigurableResourceInspector.h"
#include "../../ActionManagement/HotKey.h"
#include "../../Environment/EditorStorage.h"
#include "../../GUI/Utils/DrawMenuAction.h"
#include "../../GUI/Utils/DrawObjectPicker.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/OS/IO/FileDialogues.h>
#include <Jimara/Core/Stopwatch.h>
#include <fstream>




namespace Jimara {
	namespace Editor {
		struct ConfigurableResourceInspector::Helpers {
			class UndoInvalidationEvent : public virtual EventInstance<>, public virtual ObjectCache<Reference<ConfigurableResource>>::StoredObject {
			public:
				class Cache : public virtual ObjectCache<Reference<ConfigurableResource>> {
				public:
					inline static Reference<UndoInvalidationEvent> GetFor(ConfigurableResource* resource) {
						static Cache cache;
						return cache.GetCachedOrCreate(resource, Object::Instantiate<UndoInvalidationEvent>);
					}
				};
			};

			class ChangeUndoAction : public virtual UndoStack::Action {
			private:
				SpinLock m_lock;
				Reference<ConfigurableResource> m_resource;
				Reference<AssetDatabase> m_database;
				const Reference<OS::Logger> m_logger;
				const Reference<Graphics::GraphicsDevice> m_graphicsDevice;
				const Reference<Graphics::ShaderLoader> m_shaderLoader;
				const Reference<Physics::PhysicsInstance> m_phyiscsInstance;
				const Reference<Audio::AudioDevice> m_audioDevice;
				const nlohmann::json m_serializedData;
				Reference<UndoInvalidationEvent> m_invalidateEvent;

				inline void Invalidate() {
					std::unique_lock<SpinLock> lock(m_lock);
					if (m_invalidateEvent == nullptr) return;
					(m_invalidateEvent->operator Jimara::Event<> &()) -= Callback(&ChangeUndoAction::Invalidate, this);
					m_invalidateEvent = nullptr;
					m_resource = nullptr;
					m_database = nullptr;
				}

			public:
				inline ChangeUndoAction(ConfigurableResource* resource, EditorContext* context, const nlohmann::json& data)
					: m_resource(resource)
					, m_database(context->EditorAssetDatabase())
					, m_logger(context->Log())
					, m_graphicsDevice(context->GraphicsDevice())
					, m_shaderLoader(context->ShaderBinaryLoader())
					, m_phyiscsInstance(context->PhysicsInstance())
					, m_audioDevice(context->AudioDevice())
					, m_serializedData(data)
					, m_invalidateEvent(UndoInvalidationEvent::Cache::GetFor(resource)) {
					(m_invalidateEvent->operator Jimara::Event<> &()) += Callback(&ChangeUndoAction::Invalidate, this);
				}

				inline ~ChangeUndoAction() { Invalidate(); }

				inline virtual bool Invalidated()const final override { return m_invalidateEvent == nullptr; }

				inline virtual void Undo() final override {
					std::unique_lock<SpinLock> lock(m_lock);
					ConfigurableResource::SerializableInstance instance;
					{
						instance.instance = m_resource;
						instance.recreateArgs.log = m_logger;
						instance.recreateArgs.graphicsDevice = m_graphicsDevice;
						instance.recreateArgs.shaderLoader = m_shaderLoader;
						instance.recreateArgs.physicsInstance = m_phyiscsInstance;
						instance.recreateArgs.audioDevice = m_audioDevice;
					}
					if (!ConfigurableResourceFileAsset::DeserializeFromJson(&instance, m_database, m_logger, m_serializedData))
						m_logger->Error("ConfigurableResourceInspector::ChangeUndoAction - Failed to restore resource data!");
				}

				inline static void InvalidateFor(ConfigurableResource* resource, std::optional<nlohmann::json>& savedSanpshot) {
					const Reference<UndoInvalidationEvent> evt = UndoInvalidationEvent::Cache::GetFor(resource);
					evt->operator()();
					savedSanpshot = std::optional<nlohmann::json>();
				}
			};
		};

		ConfigurableResourceInspector::ConfigurableResourceInspector(EditorContext* context)
			: EditorWindow(context, "Configurable Resource Editor", ImGuiWindowFlags_MenuBar) { }

		ConfigurableResourceInspector::~ConfigurableResourceInspector() {
			Helpers::ChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
		}

		ConfigurableResource* ConfigurableResourceInspector::Target()const { return m_target; }

		void ConfigurableResourceInspector::SetTarget(ConfigurableResource* resource) { m_target = resource; }

		void ConfigurableResourceInspector::DrawEditorWindow() {
			// If the target comes from an asset, let us make sure we have the latest one:
			{
				Reference<ConfigurableResourceFileAsset> targetAsset = (m_target == nullptr) ?
					((ConfigurableResourceFileAsset*)nullptr) : dynamic_cast<ConfigurableResourceFileAsset*>(m_target->GetAsset());
				if (targetAsset != nullptr) {
					targetAsset = EditorWindowContext()->EditorAssetDatabase()->FindAsset(targetAsset->Guid());
					m_target = targetAsset->LoadResource();
				}
				if (m_target == nullptr) {
					m_target = Object::Instantiate<ConfigurableResource>();
					targetAsset = nullptr;
				}
			}

			// Define recreate args:
			auto getCreateArgs = [&]() {
				ConfigurableResource::CreateArgs createArgs;
				createArgs.log = EditorWindowContext()->Log();
				createArgs.graphicsDevice = EditorWindowContext()->GraphicsDevice();
				createArgs.shaderLoader = EditorWindowContext()->ShaderBinaryLoader();
				createArgs.physicsInstance = EditorWindowContext()->PhysicsInstance();
				createArgs.audioDevice = EditorWindowContext()->AudioDevice();
				return createArgs;
			};

			auto findAsset = [&](const OS::Path& path) {
				Reference<ModifiableAsset::Of<ConfigurableResource>> asset;
				EditorWindowContext()->EditorAssetDatabase()->GetAssetsFromFile<ConfigurableResource>(path, [&](FileSystemDatabase::AssetInformation info) {
					if (asset == nullptr) 
						asset = dynamic_cast<ModifiableAsset::Of<ConfigurableResource>*>(info.AssetRecord());
					});
				return asset;
			};

			auto updateAsset = [&](const OS::Path& path) {
				Helpers::ChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
				
				Stopwatch stopwatch;
				while (true) {
					bool error = false;
					ConfigurableResource::SerializableInstance instance;
					instance.instance = m_target;
					instance.recreateArgs = getCreateArgs();
					const nlohmann::json json = ConfigurableResourceFileAsset::SerializeToJson(&instance, EditorWindowContext()->Log(), error);
					if (error) {
						EditorWindowContext()->Log()->Error("ConfigurableResourceInspector - Failed to serialize resource! Content will be discarded!");
						return false;
					}

					std::ofstream stream(path);
					if (!stream.good()) {
						if (stopwatch.Elapsed() > 10.0f) {
							EditorWindowContext()->Log()->Error(
								"ConfigurableResourceInspector - Failed to open file stream '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						}
						else continue;
					}

					stream << json.dump(1, '\t') << std::endl;
					break;
				}

				stopwatch.Reset();
				const Reference<ConfigurableResource::ResourceFactory::Set> factories = ConfigurableResource::ResourceFactory::All();
				while (true) {
					Reference<ConfigurableResourceFileAsset> asset;
					EditorWindowContext()->EditorAssetDatabase()->GetAssetsFromFile<ConfigurableResource>(path, [&](const FileSystemDatabase::AssetInformation& info) {
						asset = dynamic_cast<ConfigurableResourceFileAsset*>(info.AssetRecord());
						}, false);
					if (asset == nullptr) {
						if (stopwatch.Elapsed() > 10.0f) {
							EditorWindowContext()->Log()->Error(
								"ConfigurableResourceInspector - Resource query timed out '", path, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						}
						else continue;
					}
					Reference<ConfigurableResource> resource = asset->LoadResource();
					if (resource == nullptr)
						continue;
					if (factories->FindFactory(resource.operator->()) != factories->FindFactory(m_target.operator->()))
						continue;
					m_target = resource;
					break;
				}

				return true;
			};

			// Draw load/save/saveAs buttons:
			if (ImGui::BeginMenuBar()) {
				static const std::vector<OS::FileDialogueFilter> FILE_FILTERS = {
					OS::FileDialogueFilter("Configurable Resources", { OS::Path("*" + (std::string)ConfigurableResourceFileAsset::Extension())})
				};

				auto loadAsset = [&]() {
					std::vector<OS::Path> files = OS::OpenDialogue("Load Configurable Resource", "", FILE_FILTERS);
					if (files.size() <= 0) return;
					const Reference<ModifiableAsset::Of<ConfigurableResource>> asset = findAsset(files[0]);
					if (asset != nullptr) {
						Helpers::ChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
						m_target = asset->Load();
					}
					else EditorWindowContext()->Log()->Error("ConfigurableResourceInspector::LoadAsset - No configurable resource found in '", files[0], "'!");
				};

				auto saveResourceAs = [&]() {
					std::optional<OS::Path> path = OS::SaveDialogue("Save as", "", FILE_FILTERS);
					if (!path.has_value()) 
						return;
					path.value().replace_extension(ConfigurableResourceFileAsset::Extension());

					if (!std::filesystem::exists(path.value())) {
						std::ofstream stream(path.value());
						if (!stream.good()) {
							EditorWindowContext()->Log()->Error("ConfigurableResourceInspector::SaveResourceAs - Failed to create '", path.value(), "'!");
							return;
						}
						else stream << "{}" << std::endl;
					}
					updateAsset(path.value());
				};

				auto saveResource = [&]() {
					Reference<ModifiableAsset> target = dynamic_cast<ModifiableAsset*>(m_target->GetAsset());
					if (target != nullptr) 
						target->StoreResource();
					else saveResourceAs();
				};

				static const uint8_t loadBTN = 0u, saveBTN = 1u, saveAsBTN = 2u;
				if (DrawMenuAction(ICON_FA_FOLDER " Load", "Edit existing resource", (const void*)&loadBTN))
					loadAsset();
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save", "Save resource changes", (const void*)&saveBTN) ||
					(ImGui::IsWindowFocused() && HotKey::Save().Check(EditorWindowContext()->InputModule())))
					saveResource();
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save as", "Save to a new file", (const void*)&saveAsBTN))
					saveResourceAs();

				ImGui::EndMenuBar();
			}

			// Let the user select resource from assets:
			{
				static const Reference<const Serialization::ItemSerializer::Of<Reference<ConfigurableResource>>> serializer =
					Serialization::DefaultSerializer<Reference<ConfigurableResource>>::Create("Resource", "Resource to edit");
				static thread_local std::vector<char> searchBuffer;
				Reference<ConfigurableResource> initialTarget = m_target;
				const Serialization::SerializedObject target(serializer->Serialize(m_target));
				DrawObjectPicker(
					target, CustomSerializedObjectDrawer::DefaultGuiItemName(target, (size_t)this),
					EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
				if (m_target != initialTarget)
					Helpers::ChangeUndoAction::InvalidateFor(initialTarget, m_initialSnapshot);
				ImGui::Separator();
			}

			// Actually edit the resource:
			if (m_target != nullptr) {
				bool error = false;

				ConfigurableResource::SerializableInstance instance;
				instance.instance = m_target;
				instance.recreateArgs = getCreateArgs();

				nlohmann::json snapshot = ConfigurableResourceFileAsset::SerializeToJson(&instance, EditorWindowContext()->Log(), error);
				if (error) 
					EditorWindowContext()->Log()->Error(" ConfigurableResourceInspector::DrawEditorWindow - Failed to serialize the resource!");
				else if (!ConfigurableResourceFileAsset::DeserializeFromJson(&instance, EditorWindowContext()->EditorAssetDatabase(), EditorWindowContext()->Log(), snapshot))
					EditorWindowContext()->Log()->Error("ConfigurableResourceInspector::DrawEditorWindow - Failed to refresh the resource!");

				bool changeFinished = DrawSerializedObject(ConfigurableResource::InstanceSerializer::Instance()->Serialize(instance), (size_t)this, EditorWindowContext()->Log(),
					[&](const Serialization::SerializedObject& object) -> bool {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
					});

				if (instance.instance != m_target) {
					Helpers::ChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
					Reference<Asset> asset = m_target->GetAsset();
					if (asset != nullptr) {
						FileSystemDatabase::AssetInformation assetInfo;
						if (EditorWindowContext()->EditorAssetDatabase()->TryGetAssetInfo(asset->Guid(), assetInfo)) {
							std::swap(instance.instance, m_target);
							updateAsset(assetInfo.SourceFilePath());
							std::swap(instance.instance, m_target); // We do this, so that once the asset DB catches the event, we will not loose the asset reference..
						}
					}
					else m_target = instance.instance;
				}
				else if (!error) {
					const nlohmann::json newSnapshot = ConfigurableResourceFileAsset::SerializeToJson(&instance, EditorWindowContext()->Log(), error);
					bool snapshotChanged = (snapshot != newSnapshot);
					if ((!m_initialSnapshot.has_value()) && snapshotChanged)
						m_initialSnapshot = std::move(snapshot);
				}

				if (changeFinished && m_initialSnapshot.has_value()) {
					const Reference<Helpers::ChangeUndoAction> undoAction =
						Object::Instantiate<Helpers::ChangeUndoAction>(
							m_target, EditorWindowContext(), m_initialSnapshot.value());
					EditorWindowContext()->AddUndoAction(undoAction);
					m_initialSnapshot = std::optional<nlohmann::json>();
				}
			}
		}

		namespace {
			class GameViewSerializer : public virtual EditorStorageSerializer::Of<ConfigurableResourceInspector> {
			public:
				inline GameViewSerializer() : Serialization::ItemSerializer("ConfigurableResourceInspector", "Configurable Resource Inspector (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ConfigurableResourceInspector* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
					typedef ConfigurableResource* ResourceRef;
					typedef ConfigurableResource* (*GetFn)(ConfigurableResourceInspector*);
					typedef void(*SetFn)(const ResourceRef&, ConfigurableResourceInspector*);
					static const GetFn get = [](ConfigurableResourceInspector* inspector) -> ConfigurableResource* { return inspector->Target(); };
					static const SetFn set = [](const ResourceRef& value, ConfigurableResourceInspector* inspector) { inspector->SetTarget(value); };
					static const Reference<const Serialization::ItemSerializer::Of<ConfigurableResourceInspector>> serializer =
						Serialization::ValueSerializer<ResourceRef>::Create<ConfigurableResourceInspector>("Target", "Target resource", Function(get), Callback(set));
					recordElement(serializer->Serialize(target));
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::ConfigurableResourceInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::ConfigurableResourceInspector>(const Callback<const Object*>& report) {
		static const Editor::GameViewSerializer serializer;
		report(&serializer);
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Edit/Configurable Resource", "Open Configurable Resource editor window", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::ConfigurableResourceInspector>(context);
				}));
		report(&editorMenuCallback);
	}
}
