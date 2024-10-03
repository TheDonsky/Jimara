#include "AssetBrowser.h"
#include "../../Environment/EditorStorage.h"
#include "../../ActionManagement/EditorDragAndDrop.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <IconFontCppHeaders/IconsMaterialDesign.h>


namespace Jimara {
	namespace Editor {
		struct AssetBrowser::Helpers {
			// __TODO__: Implement this crap!
		};

		AssetBrowser::AssetBrowser(EditorContext* context) 
			: EditorWindow(context, "Asset Browser") {}

		AssetBrowser::~AssetBrowser() {}

		void AssetBrowser::DrawEditorWindow() {
			const OS::Path& assetDBPath = EditorWindowContext()->EditorAssetDatabase()->AssetDirectory();
			OS::Path path = assetDBPath / ActiveDirectory();
			if (!std::filesystem::is_directory(path)) {
				ActiveDirectory() = OS::Path("");
				path = assetDBPath;
			}

			// Collect subfolder paths:
			std::vector<OS::Path> subfolders = {};
			OS::Path::IterateDirectory(path,
				[&](const OS::Path& p) { subfolders.push_back(p); return true; },
				OS::Path::IterateDirectoryFlags::REPORT_DIRECTORIES);

			// Collect all file paths:
			std::vector<OS::Path> filePaths = {};
			OS::Path::IterateDirectory(path,
				[&](const OS::Path& p) { filePaths.push_back(p); return true; },
				OS::Path::IterateDirectoryFlags::REPORT_FILES);

			// Control path:
			if (!ActiveDirectory().empty()) {
				if (ImGui::Button(ICON_FA_ARROW_LEFT)) {
					try {
						ActiveDirectory() = std::filesystem::relative(path / "../", assetDBPath);
						m_currentSelection = "";
					}
					catch (const std::exception&) { ActiveDirectory() = OS::Path(""); }
				}
			}

			// Display subfolders:
			for (size_t i = 0u; i < subfolders.size(); i++) {
				try {
					const std::string folderStr = OS::Path(subfolders[i].filename());
					bool selected = (m_currentSelection == folderStr);
					if (ImGui::Selectable(folderStr.c_str(), &selected))
						m_currentSelection = folderStr;
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						ActiveDirectory() /= folderStr;
						m_currentSelection = "";
					}
				}
				catch (const std::exception&) {}
			}
			
			// Display all files/assets:
			Stacktor<FileSystemDatabase::AssetInformation> assetsFromFile;
			for (size_t i = 0u; i < filePaths.size(); i++) {
				const OS::Path& path = filePaths[i];
				if (path.extension() == EditorWindowContext()->EditorAssetDatabase()->DefaultMetadataExtension())
					continue;
				// Get assets:
				assetsFromFile.Clear();
				EditorWindowContext()->EditorAssetDatabase()->GetAssetsFromFile(path,
					[&](const FileSystemDatabase::AssetInformation& info) {
						assetsFromFile.Push(info);
					});

				// Draw path dropdown:
				const std::string fileStr = OS::Path(path.filename());
				const std::string treeNodeStrId = [&]()->std::string {
					std::stringstream stream;
					stream << "%s###editor_AssetBrowser_" << ((size_t)this) << "_assetNodeId_" << i;
					return stream.str();
				}();
				const bool nodeExpanded = ImGui::TreeNodeEx(treeNodeStrId.c_str(), 
					ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding |
					((m_currentSelection == fileStr) ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None),
					fileStr.c_str());
				if (ImGui::IsItemClicked())
					m_currentSelection = fileStr;

				// Drag and drop first asset:
				if (assetsFromFile.Size() > 0u)
					if (ImGui::BeginDragDropSource()) {
						// Maybe prioritize by type somehow? (hierarchy asset should come first, for example...)
						SetDragAndDropAsset(assetsFromFile[0u].AssetRecord());
						ImGui::EndDragDropSource();
					}

				// Draw all nodes
				if (nodeExpanded) {
					for (size_t j = 0u; j < assetsFromFile.Size(); j++) {
						const FileSystemDatabase::AssetInformation& data = assetsFromFile[j];
						const std::string resourceTypeName = data.ResourceName() + " <" + std::string(data.AssetRecord()->ResourceType().Name()) + ">";
						const std::string resourceId = [](const GUID id) -> std::string {
							std::stringstream stream;
							stream << '<';
							for (size_t b = 0u; b < id.NUM_BYTES; b++) {
								if (b > 0u)
									stream << '.';
								stream << id.bytes[b];
							}
							stream << '>';
							return stream.str();
						}(data.AssetRecord()->Guid());
						if (ImGui::Selectable(resourceTypeName.c_str(), m_currentSelection == resourceId))
							m_currentSelection = resourceId;
						if (ImGui::BeginDragDropSource()) {
							SetDragAndDropAsset(data.AssetRecord());
							ImGui::EndDragDropSource();
						}
					}
					ImGui::TreePop();
				}

				assetsFromFile.Clear();
			}
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::AssetBrowser>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::AssetBrowser>(const Callback<const Object*>& report) {
		struct AssetBrowserSerializer : public virtual Editor::EditorStorageSerializer::Of<Editor::AssetBrowser> {
			inline AssetBrowserSerializer() : Serialization::ItemSerializer("AssetBrowser", "Asset browser window") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Editor::AssetBrowser* target)const final override {
				Editor::EditorWindow::Serializer()->GetFields(recordElement, target);
				std::string curDir = target->ActiveDirectory();
				JIMARA_SERIALIZE_FIELDS(target, recordElement) {
					JIMARA_SERIALIZE_FIELD(curDir, "Active Directory", "Active [sub]directory");
				};
				target->ActiveDirectory() = OS::Path(curDir);
			}
		};
		static const AssetBrowserSerializer serializer;
		report(&serializer);
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Assets/Browser", "Open asset browser window", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::AssetBrowser>(context);
				}));
		report(&editorMenuCallback);
	}
}
