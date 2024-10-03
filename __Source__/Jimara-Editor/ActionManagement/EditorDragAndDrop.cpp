#include "EditorDragAndDrop.h"
#include "../GUI/ImGuiIncludes.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static const constexpr std::string_view ASSERT_DRAG_AND_DROP_TYPE = "JM_EDITOR_ASSET_DRAG_AND_DROP_T";
		}

		JIMARA_EDITOR_API void SetDragAndDropAsset(Asset* asset) {
			const GUID id = (asset != nullptr) ? asset->Guid() : GUID::Null();
			ImGui::SetDragDropPayload(ASSERT_DRAG_AND_DROP_TYPE.data(), &id, sizeof(id));
		}

		JIMARA_EDITOR_API Reference<Asset> AcceptDragAndDropAsset(AssetDatabase* database) {
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSERT_DRAG_AND_DROP_TYPE.data());
			if (payload == nullptr)
				return nullptr;
			if (database == nullptr ||
				ASSERT_DRAG_AND_DROP_TYPE != payload->DataType ||
				payload->DataSize != sizeof(GUID))
				return nullptr;
			return database->FindAsset(*reinterpret_cast<const GUID*>(payload->Data));
		}
	}
}
