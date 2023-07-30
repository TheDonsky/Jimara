#include "FullscreenToggle.h"

namespace Jimara {
	namespace {
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Window/ToggleFullscreen", "Toggles fullscreen mode", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				context->Window()->SetFullscreen(!context->Window()->IsFullscreen());
				}));
		static Editor::EditorMainMenuAction::RegistryEntry action;
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::FullscreenToggleAction>() {
		action = &editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::FullscreenToggleAction>() {
		action = nullptr;
	}
}
