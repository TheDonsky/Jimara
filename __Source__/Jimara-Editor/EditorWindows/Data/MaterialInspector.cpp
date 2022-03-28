#include "MaterialInspector.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include "../../GUI/Utils/DrawObjectPicker.h"
#include "../../GUI/Utils/DrawMenuAction.h"
#include <Data/Formats/MaterialFileAsset.h>
#include <OS/IO/FileDialogues.h>
#include <Core/Stopwatch.h>
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <fstream>


namespace Jimara {
	namespace Editor {
		MaterialInspector::MaterialInspector(EditorContext* context) 
			: EditorWindow(context, "Material Editor", ImGuiWindowFlags_MenuBar) { }

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
					if (asset != nullptr) self->m_target = asset->Load();
					else self->EditorWindowContext()->Log()->Error("MaterialInspector::LoadMaterial - No material found in '", files[0], "'!");
				};
				
				static MenuBarAction saveMaterialAs = [](MaterialInspector* self) {
					std::optional<OS::Path> path = OS::SaveDialogue("Save as", "", FILE_FILTERS);
					if (!path.has_value()) return;
					path.value().replace_extension(MaterialFileAsset::Extension());

					auto updateAsset = [&]() {
						const Reference<ModifiableAsset::Of<Material>> asset = findAsset(self, path.value());
						if (asset == nullptr) return false;
						self->EditorWindowContext()->Log()->Error("MaterialInspector::SaveMaterialAs - Material copy not yet implemented!");
						self->m_target = asset->Load();
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
				else if (DrawMenuAction(ICON_FA_FLOPPY_O " Save", (void*)saveMaterial)) saveMaterial(this);
				else if (DrawMenuAction(ICON_FA_FLOPPY_O " Save as", (void*)saveMaterialAs)) saveMaterialAs(this);

				ImGui::EndMenuBar();
			}

			if (m_target != nullptr)
				DrawSerializedObject(Material::Serializer::Instance()->Serialize(m_target), (size_t)this, EditorWindowContext()->Log(),
					[&](const Serialization::SerializedObject& object) -> bool {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, EditorWindowContext()->Log(), nullptr, EditorWindowContext()->EditorAssetDatabase(), &searchBuffer);
					});
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Edit/Material", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<MaterialInspector>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::MaterialInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::MaterialInspector>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::MaterialInspector>() {
		Editor::action = nullptr;
	}
}
