#include "PhysicsMaterialInspector.h"
#include "../../ActionManagement/HotKey.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include "../../GUI/Utils/DrawObjectPicker.h"
#include "../../GUI/Utils/DrawMenuAction.h"
#include "../../Environment/EditorStorage.h"
#include <Jimara/Data/Serialization/DefaultSerializer.h>
#include <Jimara/OS/IO/FileDialogues.h>
#include <Jimara/OS/Input/NoInput.h>
#include <Jimara/Core/Stopwatch.h>
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <fstream>


namespace Jimara {
	namespace Editor {
		struct PhysicsMaterialInspector::Helpers {
			class UndoInvalidationEvent : public virtual EventInstance<>, public virtual ObjectCache<Reference<Physics::PhysicsMaterial>>::StoredObject {
			public:
				class Cache : public virtual ObjectCache<Reference<Physics::PhysicsMaterial>> {
				public:
					inline static Reference<UndoInvalidationEvent> GetFor(Physics::PhysicsMaterial* material) {
						static Cache cache;
						return cache.GetCachedOrCreate(material, Object::Instantiate<UndoInvalidationEvent>);
					}
				};
			};

			class ChangeUndoAction : public virtual UndoStack::Action {
			private:
				SpinLock m_lock;
				Reference<Physics::PhysicsMaterial> m_material;
				Reference<AssetDatabase> m_database;
				const Reference<OS::Logger> m_logger;
				const nlohmann::json m_serializedData;
				Reference<UndoInvalidationEvent> m_invalidateEvent;

				inline void Invalidate() {
					std::unique_lock<SpinLock> lock(m_lock);
					if (m_invalidateEvent == nullptr) return;
					(m_invalidateEvent->operator Jimara::Event<> &()) -= Callback(&ChangeUndoAction::Invalidate, this);
					m_invalidateEvent = nullptr;
					m_material = nullptr;
					m_database = nullptr;
				}

			public:
				inline ChangeUndoAction(Physics::PhysicsMaterial* material, AssetDatabase* database, OS::Logger* logger, const nlohmann::json& data)
					: m_material(material), m_database(database), m_logger(logger), m_serializedData(data), m_invalidateEvent(UndoInvalidationEvent::Cache::GetFor(material)) {
					(m_invalidateEvent->operator Jimara::Event<> &()) += Callback(&ChangeUndoAction::Invalidate, this);
				}

				inline ~ChangeUndoAction() { Invalidate(); }

				inline virtual bool Invalidated()const final override { return m_invalidateEvent == nullptr; }

				inline virtual void Undo() final override {
					std::unique_lock<SpinLock> lock(m_lock);
					if (!PhysicsMaterialFileAsset::DeserializeFromJson(m_material, m_database, m_logger, m_serializedData))
						m_logger->Error("PhysicsMaterialInspector::ChangeUndoAction - Failed to restore physics material data!");
				}

				inline static void InvalidateFor(Physics::PhysicsMaterial* material, std::optional<nlohmann::json>& savedSanpshot) {
					const Reference<UndoInvalidationEvent> evt = UndoInvalidationEvent::Cache::GetFor(material);
					evt->operator()();
					savedSanpshot = std::optional<nlohmann::json>();
				}
			};
		};

		PhysicsMaterialInspector::PhysicsMaterialInspector(EditorContext* context)
			: EditorWindow(context, "Physics Material Editor", ImGuiWindowFlags_MenuBar) { }

		PhysicsMaterialInspector::~PhysicsMaterialInspector() {
			Helpers::ChangeUndoAction::InvalidateFor(m_target, m_initialSnapshot);
		}

		Physics::PhysicsMaterial* PhysicsMaterialInspector::Target()const { return m_target; }

		void PhysicsMaterialInspector::SetTarget(Physics::PhysicsMaterial* material) { m_target = material; }

		void PhysicsMaterialInspector::DrawEditorWindow() {
			if (m_target == nullptr)
				m_target = EditorWindowContext()->PhysicsInstance()->CreateMaterial();

			// Draw load/save/saveAs buttons:
			if (ImGui::BeginMenuBar()) {
				static const std::vector<OS::FileDialogueFilter> FILE_FILTERS = {
					OS::FileDialogueFilter("Physics Materials", { OS::Path("*" + (std::string)PhysicsMaterialFileAsset::Extension())})
				};

				static auto findAsset = [](PhysicsMaterialInspector* self, const OS::Path& path) {
					Reference<ModifiableAsset::Of<Physics::PhysicsMaterial>> asset;
					self->EditorWindowContext()->EditorAssetDatabase()->GetAssetsFromFile<Physics::PhysicsMaterial>(path, [&](FileSystemDatabase::AssetInformation info) {
						if (asset == nullptr) asset = dynamic_cast<ModifiableAsset::Of<Physics::PhysicsMaterial>*>(info.AssetRecord());
						});
					return asset;
				};

				typedef void(*MenuBarAction)(PhysicsMaterialInspector*);

				static MenuBarAction loadMaterial = [](PhysicsMaterialInspector* self) {
					std::vector<OS::Path> files = OS::OpenDialogue("Load Physics Material", "", FILE_FILTERS);
					if (files.size() <= 0) return;
					const Reference<ModifiableAsset::Of<Physics::PhysicsMaterial>> asset = findAsset(self, files[0]);
					if (asset != nullptr) {
						Helpers::ChangeUndoAction::InvalidateFor(self->m_target, self->m_initialSnapshot);
						self->m_target = asset->Load();
					}
					else self->EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::LoadMaterial - No physics material found in '", files[0], "'!");
				};

				static MenuBarAction saveMaterialAs = [](PhysicsMaterialInspector* self) {
					std::optional<OS::Path> path = OS::SaveDialogue("Save as", "", FILE_FILTERS);
					if (!path.has_value()) return;
					path.value().replace_extension(PhysicsMaterialFileAsset::Extension());

					auto updateAsset = [&]() {
						const Reference<ModifiableAsset::Of<Physics::PhysicsMaterial>> asset = findAsset(self, path.value());
						if (asset == nullptr) return false;
						Reference<Physics::PhysicsMaterial> material = asset->Load();
						if (material != nullptr && self->m_target != nullptr && material != self->m_target) {
							bool error = false;
							const nlohmann::json json = PhysicsMaterialFileAsset::SerializeToJson(self->m_target, self->EditorWindowContext()->Log(), error);
							if (error)
								self->EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::SaveMaterialAs - Failed to serialize physics material! Content will be discarded!");
							else if (!PhysicsMaterialFileAsset::DeserializeFromJson(material, self->EditorWindowContext()->EditorAssetDatabase(), self->EditorWindowContext()->Log(), json))
								self->EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::SaveMaterialAs - Failed to copy physics material! Contentmay be incomplete!");
						}
						Helpers::ChangeUndoAction::InvalidateFor(self->m_target, self->m_initialSnapshot);
						self->m_target = material;
						return self->m_target != nullptr;
					};

					if (!std::filesystem::exists(path.value())) {
						std::ofstream stream(path.value());
						if (!stream.good()) {
							self->EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::SaveMaterialAs - Failed to create '", path.value(), "'!");
							return;
						}
						else stream << "{}" << std::endl;
					}
					Stopwatch stopwatch;
					while (true) {
						if (updateAsset()) break;
						else if (stopwatch.Elapsed() > 10.0f) {
							self->EditorWindowContext()->Log()->Error(
								"PhysicsMaterialInspector::SaveMaterialAs - Resource query timed out '", path.value(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							break;
						}
					}
				};

				static MenuBarAction saveMaterial = [](PhysicsMaterialInspector* self) {
					Reference<ModifiableAsset> target = dynamic_cast<ModifiableAsset*>(self->m_target->GetAsset());
					if (target != nullptr) target->StoreResource();
					else saveMaterialAs(self);
				};

				if (DrawMenuAction(ICON_FA_FOLDER " Load",
					"Edit existing physics material",
					(void*)loadMaterial)) loadMaterial(this);
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save", "Save physics material changes", (void*)saveMaterial) ||
					(ImGui::IsWindowFocused() && HotKey::Save().Check(EditorWindowContext()->InputModule())))
					saveMaterial(this);
				if (DrawMenuAction(ICON_FA_FLOPPY_O " Save as",
					"Save to a new file",
					(void*)saveMaterialAs)) saveMaterialAs(this);

				ImGui::EndMenuBar();
			}

			// Let the user select physics material from assets:
			{
				static const Reference<const Serialization::ItemSerializer::Of<Reference<Physics::PhysicsMaterial>>> serializer =
					Serialization::DefaultSerializer<Reference<Physics::PhysicsMaterial>>::Create("Physics Material", "Physics Material to edit");
				static thread_local std::vector<char> searchBuffer;
				const Serialization::SerializedObject target(serializer->Serialize(m_target));
				DrawObjectPicker(
					target, CustomSerializedObjectDrawer::DefaultGuiItemName(target, (size_t)this),
					EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
				ImGui::Separator();
			}

			// Actually edit the physics material:
			if (m_target != nullptr) {
				bool error = false;
				nlohmann::json snapshot = PhysicsMaterialFileAsset::SerializeToJson(m_target, EditorWindowContext()->Log(), error);
				if (error) EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::DrawEditorWindow - Failed to serialize physics material!");
				else if (!PhysicsMaterialFileAsset::DeserializeFromJson(m_target, EditorWindowContext()->EditorAssetDatabase(), EditorWindowContext()->Log(), snapshot))
					EditorWindowContext()->Log()->Error("PhysicsMaterialInspector::DrawEditorWindow - Failed to refresh physics material!");

				static const Physics::PhysicsMaterial::Serializer serializer("Physics Material", "Physics Material");
				bool changeFinished = DrawSerializedObject(serializer.Serialize(m_target), (size_t)this, EditorWindowContext()->Log(),
					[&](const Serialization::SerializedObject& object) -> bool {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
					});

				if (!error) {
					const nlohmann::json newSnapshot = PhysicsMaterialFileAsset::SerializeToJson(m_target, EditorWindowContext()->Log(), error);
					bool snapshotChanged = (snapshot != newSnapshot);
					if ((!m_initialSnapshot.has_value()) && snapshotChanged)
						m_initialSnapshot = std::move(snapshot);
				}

				if (changeFinished && m_initialSnapshot.has_value()) {
					const Reference<Helpers::ChangeUndoAction> undoAction =
						Object::Instantiate<Helpers::ChangeUndoAction>(
							m_target, EditorWindowContext()->EditorAssetDatabase(), EditorWindowContext()->Log(), m_initialSnapshot.value());
					EditorWindowContext()->AddUndoAction(undoAction);
					m_initialSnapshot = std::optional<nlohmann::json>();
				}
			}
		}

		namespace {
			class GameViewSerializer : public virtual EditorStorageSerializer::Of<PhysicsMaterialInspector> {
			public:
				inline GameViewSerializer() : Serialization::ItemSerializer("PhysicsMaterialInspector", "Physics Material Inspector (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, PhysicsMaterialInspector* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
					typedef Physics::PhysicsMaterial* MaterialRef;
					typedef Physics::PhysicsMaterial* (*GetFn)(PhysicsMaterialInspector*);
					typedef void(*SetFn)(const MaterialRef&, PhysicsMaterialInspector*);
					static const GetFn get = [](PhysicsMaterialInspector* inspector) -> Physics::PhysicsMaterial* { return inspector->Target(); };
					static const SetFn set = [](const MaterialRef& value, PhysicsMaterialInspector* inspector) { inspector->SetTarget(value); };
					static const Reference<const Serialization::ItemSerializer::Of<PhysicsMaterialInspector>> serializer =
						Serialization::ValueSerializer<MaterialRef>::Create<PhysicsMaterialInspector>("Target", "Target Material", Function(get), Callback(set));
					recordElement(serializer->Serialize(target));
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::PhysicsMaterialInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::PhysicsMaterialInspector>(const Callback<const Object*>& report) {
		static const Editor::GameViewSerializer serializer;
		report(&serializer);
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Edit/Physics Material", "Open Physics Material editor window", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::PhysicsMaterialInspector>(context);
				}));
		report(&editorMenuCallback);
	}
}
