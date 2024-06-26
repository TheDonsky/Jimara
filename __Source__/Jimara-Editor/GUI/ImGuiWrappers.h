#pragma once
#include "../Environment/JimaraEditorTypeRegistry.h"
#include <string_view>
#include <sstream>
#include <Jimara/Math/Math.h>


namespace Jimara {
	namespace Editor {
		JIMARA_EDITOR_API bool Button(const std::string_view& label, const Vector2& size = Vector2(0.0f));

		JIMARA_EDITOR_API void Label(const std::string_view& text);
	}
}
