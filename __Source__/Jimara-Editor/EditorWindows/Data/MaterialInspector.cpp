#include "MaterialInspector.h"
#include "../../ActionManagement/HotKey.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include "../../GUI/Utils/DrawObjectPicker.h"
#include "../../GUI/Utils/DrawMenuAction.h"
#include "../../Environment/EditorStorage.h"
#include <OS/IO/FileDialogues.h>
#include <Core/Stopwatch.h>
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <fstream>


namespace Jimara {
	namespace Editor {
		namespace {
			class UndoInvalidationEvent : public virtual EventInstance<>, public virtual ObjectCache<Reference<Material>>::StoredObject {
			public:
				class Cache : public virtual ObjectCache<Reference<Material>> {
				public:
					inline static Reference<UndoInvalidationEvent> GetFor(Material* material) {
						static Cache cache;
						return cache.GetCachedOrCreate(material, false, Object::Instantiate<UndoInvalidationEvent>);
					}
				};
			};

			class MaterialInspectorChangeUndoAction : public virtual UndoStack::Action {
			private:
				SpinLock m_lock;
				Reference<Material> m_material;
				Reference<AssetDatabase> m_database;
				const Reference<OS::Logger> m_logger;
				const nlohmann::json m_serializedData;
				Reference<UndoInvalidationEvent> m_invalidateEvent;

				inline void Invalidate() {
					std::unique_lock<SpinLock> lock(m_lock);
					if (m_invalidateEvent == nullptr) return;
					(m_invalidateEvent->operator Jimara::Event<> &()) -= Callback(&MaterialInspectorChangeUndoAction::Invalidate, this);
					m_invalidateEvent = nullptr; 
					m_material = nullptr;
					m_database = nullptr;
				}

			public:
				inline MaterialInspectorChangeUndoAction(Material* material, AssetDatabase* database, OS::Logger* logger, const nlohmann::json& data)
					: m_material(material), m_database(database), m_logger(logger), m_serializedData(data), m_invalidateEvent(UndoInvalidationEvent::Cache::GetFor(material)) {
					(m_invalidateEvent->operator Jimara::Event<> &()) += Callback(&MaterialInspectorChangeUndoAction::Invalidate, this);
				}

				inline ~MaterialInspectorChangeUndoAction() { Invalidate(); }

				inline virtual bool Invalidated()const final override { return m_invalidateEvent == nullptr; }

				inline virtual void Undo() final override {
					std::unique_lock<SpinLock> lock(m_lock);
					if (!MaterialFileAsset::DeserializeFromJson(m_material, m_database, m_logger, m_serializedData))
						m_logger->Error("MaterialInspector::MaterialInspectorChangeUndoAction - Failed to restore material data!");
				}

				inline static void InvalidateFor(Material* material, std::optional<nlohmann::json>& savedSanpshot) {
					const Reference<UndoInvalidationEvent> evt = UndoInvalidationEvent::Cache::GetFor(material);
					evt->operator()();
					savedSanpshot = std::optional<nlohmann::json>();
				}
			};
		}

		MaterialInspector::MaterialInspector(EditorContext* context) 
			: EditorWindow(context, "Material Editor", ImGuiWindowFlags_MenuBar) { }

		MaterialInspector::~MaterialInspector() {
			MaterialInspectorChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
		}

		Material* MaterialInspector::Target()const { return m_target; }

		void MaterialInspector::SetTarget(Material* material) { m_target = material; }

		void MaterialInspector::DrawEditorWindow() {
			if (m_target == nullptr)
				m_target = Object::Instantiate<Material>(EditorWindowContext()->GraphicsDevice());

			if (ImGui::BeginMenuBar()) {
				static const std::vector<OS::FileDialogueFilter> FILE_FILTERS = {
					OS::FileDialogueFilter("Materials", { OS::Path("*" + (std::string)MaterialFileAsset::Extension())})
				};

				static auto findAsset = [](MaterialInspector* self, const OS::Path& path) {
					Reference<ModifiableAsset::Of<Material>> asset;
					self->EditorWindowContext()->EditorAssetDatabase()->GetAssetsFromFile<Material>(path, [&](FileSystemDatabase::AssetInformation info) {
						if (asset == nullptr) asset = dynamic_cast<ModifiableAsset::Of<Material>*>(info.AssetRecord());
						});
					return asset;
				};

				typedef void(*MenuBarAction)(MaterialInspector*);

				static MenuBarAction loadMaterial = [](MaterialInspector* self) {
					std::vector<OS::Path> files = OS::OpenDialogue("Load Material", "", FILE_FILTERS);
					if (files.size() <= 0) return;
					const Reference<ModifiableAsset::Of<Material>> asset = findAsset(self, files[0]);
					if (asset != nullptr) {
						MaterialInspectorChangeUndoAction::InvalidateFor(self->m_target, self->m_initialSnapshot);
						self->m_target = asset->Load();
					}
					else self->EditorWindowContext()->Log()->Error("MaterialInspector::LoadMaterial - No material found in '", files[0], "'!");
				};
				
				static MenuBarAction saveMaterialAs = [](MaterialInspector* self) {
					std::optional<OS::Path> path = OS::SaveDialogue("Save as", "", FILE_FILTERS);
					if (!path.has_value()) return;
					path.value().replace_extension(MaterialFileAsset::Extension());

					auto updateAsset = [&]() {
						const Reference<ModifiableAsset::Of<Material>> asset = findAsset(self, path.value());
						if (asset == nullptr) return false;
						Reference<Material> material = asset->Load();
						if (material != nullptr && self->m_target != nullptr && material != self->m_target) {
							bool error = false;
							const nlohmann::json json = MaterialFileAsset::SerializeToJson(self->m_target, self->EditorWindowContext()->Log(), error);
							if (error)
								self->EditorWindowContext()->Log()->Error("MaterialInspector::SaveMaterialAs - Failed to serialize material! Content will be discarded!");
							else if(!MaterialFileAsset::DeserializeFromJson(material, self->EditorWindowContext()->EditorAssetDatabase(), self->EditorWindowContext()->Log(), json))
								self->EditorWindowContext()->Log()->Error("MaterialInspector::SaveMaterialAs - Failed to copy material! Contentmay be incomplete!");
						}
						MaterialInspectorChangeUndoAction::InvalidateFor(self->m_target, self->m_initialSnapshot);
						self->m_target = material;
						return self->m_target != nullptr;
					};

					if (!std::filesystem::exists(path.value())) {
						std::ofstream stream(path.value());
						if (!stream.good()) {
							self->EditorWindowContext()->Log()->Error("MaterialInspector::SaveMaterialAs - Failed to create '", path.value(), "'!");
							return;
						}
						else stream << "{}" << std::endl;
					}
					Stopwatch stopwatch;
					while (true) {
						if (updateAsset()) break;
						else if (stopwatch.Elapsed() > 10.0f) {
							self->EditorWindowContext()->Log()->Error(
								"MaterialInspector::SaveMaterialAs - Resource query timed out '", path.value(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							break;
						}
					}
				};

				static MenuBarAction saveMaterial = [](MaterialInspector* self) {
					Reference<ModifiableAsset> target = dynamic_cast<ModifiableAsset*>(self->m_target->GetAsset());
					if (target != nullptr) target->StoreResource();
					else saveMaterialAs(self);
				};

				if (DrawMenuAction(ICON_FA_FOLDER " Load", (void*)loadMaterial)) loadMaterial(this);
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save", (void*)saveMaterial) || HotKey::Save().Check(EditorWindowContext()->InputModule())) saveMaterial(this);
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save as", (void*)saveMaterialAs)) saveMaterialAs(this);

				ImGui::EndMenuBar();
			}

			if (m_target != nullptr) {
				bool error = false;
				nlohmann::json snapshot = MaterialFileAsset::SerializeToJson(m_target, EditorWindowContext()->Log(), error);
				if (error) EditorWindowContext()->Log()->Error("MaterialInspector::DrawEditorWindow - Failed to serialize material!");
				else if (!MaterialFileAsset::DeserializeFromJson(m_target, EditorWindowContext()->EditorAssetDatabase(), EditorWindowContext()->Log(), snapshot))
					EditorWindowContext()->Log()->Error("MaterialInspector::DrawEditorWindow - Failed to refresh material!");

				bool changeFinished = DrawSerializedObject(Material::Serializer::Instance()->Serialize(m_target), (size_t)this, EditorWindowContext()->Log(),
					[&](const Serialization::SerializedObject& object) -> bool {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
					});
				
				if (!error) {
					const nlohmann::json newSnapshot = MaterialFileAsset::SerializeToJson(m_target, EditorWindowContext()->Log(), error);
					if ((!m_initialSnapshot.has_value()) && (snapshot != newSnapshot))
						m_initialSnapshot = std::move(snapshot);
				}

				if (changeFinished && m_initialSnapshot.has_value()) {
					const Reference<MaterialInspectorChangeUndoAction> undoAction = Object::Instantiate<MaterialInspectorChangeUndoAction>(
						m_target, EditorWindowContext()->EditorAssetDatabase(), EditorWindowContext()->Log(), m_initialSnapshot.value());
					EditorWindowContext()->AddUndoAction(undoAction);
					m_initialSnapshot = std::optional<nlohmann::json>();
				}
			}
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Edit/Material", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<MaterialInspector>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;

			class GameViewSerializer : public virtual EditorStorageSerializer::Of<MaterialInspector> {
			public:
				inline GameViewSerializer() : Serialization::ItemSerializer("MaterialInspector", "Material Inspector (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, MaterialInspector* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
					typedef Material* MaterialRef;
					typedef Material* (*GetFn)(MaterialInspector*);
					typedef void(*SetFn)(const MaterialRef&, MaterialInspector*);
					static const GetFn get = [](MaterialInspector* inspector) -> Material* { return inspector->Target(); };
					static const SetFn set = [](const MaterialRef& value, MaterialInspector* inspector) { inspector->SetTarget(value); };
					static const Reference<const Serialization::ItemSerializer::Of<MaterialInspector>> serializer =
						Serialization::ValueSerializer<MaterialRef>::Create<MaterialInspector>("Target", "Target Material", Function(get), Callback(set));
					recordElement(serializer->Serialize(target));
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::MaterialInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MaterialInspector>(const Callback<const Object*>& report) {
		static const Editor::GameViewSerializer serializer;
		report(&serializer);
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::MaterialInspector>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::MaterialInspector>() {
		Editor::action = nullptr;
	}
}
