#include "FullscreenToggle.h"

namespace Jimara {
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::FullscreenToggleAction>(const Callback<const Object*>& report) {
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Window/ToggleFullscreen", "Toggles fullscreen mode", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				context->Window()->SetFullscreen(!context->Window()->IsFullscreen());
				}));
		report(&editorMenuCallback);
	}
}
