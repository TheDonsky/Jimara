#pragma once
#include "DrawTooltip.h"
#include "../../Environment/JimaraEditorTypeRegistry.h"
#include <Jimara/Components/Component.h>
#include <Jimara/Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h>

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Draws Object picker for ObjectReference-type serialized object
		/// </summary>
		/// <param name="serializedObject"> Serialized Object </param>
		/// <param name="serializedObjectId"> Unique identifier for the serializedObject </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <param name="rootObject"> Component [sub]tree root for the component references </param>
		/// <param name="assetDatabase"> Asset database for asset/resource references </param>
		/// <param name="searchBuffer"> Search field buffer (nullptr will prevent search bar from appearing) </param>
		/// <returns> True, if the value gets modified </returns>
		JIMARA_EDITOR_API bool DrawObjectPicker(
			const Serialization::SerializedObject& serializedObject, const std::string_view& serializedObjectId,
			OS::Logger* logger, Component* rootObject, FileSystemDatabase* assetDatabase, std::vector<char>* searchBuffer);
	}
}
