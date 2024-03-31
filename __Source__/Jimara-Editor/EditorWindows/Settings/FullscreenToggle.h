#pragma once
#include "../../Environment/JimaraEditor.h"

namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about FullscreenToggle action </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::FullscreenToggleAction);

		/// <summary>
		/// Adds fullscreen toggle to the main menu
		/// </summary>
		class FullscreenToggleAction {
		private:
			// Not constructable
			inline FullscreenToggleAction() {}
		};
	}

	// TypeIdDetails for FullscreenToggleAction
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::FullscreenToggleAction>(const Callback<const Object*>& report);
}
