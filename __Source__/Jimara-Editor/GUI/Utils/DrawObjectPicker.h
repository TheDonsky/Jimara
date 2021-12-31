#pragma once
#include "DrawTooltip.h"
#include <Components/Component.h>
#include <Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h>

namespace Jimara {
	namespace Editor {
		void DrawObjectPicker(
			const Serialization::SerializedObject& serializedObject, const std::string_view& serializedObjectId,
			OS::Logger* logger, Component* rootObject, const FileSystemDatabase* assetDatabase, std::vector<char>* searchBuffer);
	}
}
