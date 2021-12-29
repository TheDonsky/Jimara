#pragma once
#include "../../Environment/JimaraEditor.h"
#include "../../GUI/ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		void DrawSerializedObject(const Serialization::SerializedObject& object, size_t viewId, EditorContext* context);
	}
}
