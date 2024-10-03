#pragma once
#include "../Environment/JimaraEditor.h"

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Sets asset as a drag and drop target
		/// </summary>
		/// <param name="asset"> Asset reference </param>
		JIMARA_EDITOR_API void SetDragAndDropAsset(Asset* asset);

		/// <summary>
		/// Accepts drag and drop target asset
		/// </summary>
		/// <param name="database"> Asset database </param>
		/// <returns> Asset from drag and drop payload (nullptr if there was no asset payload) </returns>
		JIMARA_EDITOR_API Reference<Asset> AcceptDragAndDropAsset(AssetDatabase* database);
	}
}
