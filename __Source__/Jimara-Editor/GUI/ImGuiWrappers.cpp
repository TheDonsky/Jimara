#include "ImGuiWrappers.h"
#include "ImGuiIncludes.h"

namespace Jimara {
	namespace Editor {
		bool Button(const std::string_view& label, const Vector2& size) {
			return ImGui::Button(label.data(), ImVec2(size.x, size.y));
		}

		void Label(const std::string_view& text) {
			ImGui::TextUnformatted(text.data(), text.data() + text.size());
		}
	}
}
