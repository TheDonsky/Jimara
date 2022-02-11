#pragma once
#include "../ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Draws tooltip with some delay
		/// </summary>
		/// <param name="targetObjectId"> String that uniquely identifies last drawn ImGui object </param>
		/// <param name="tooltip"> Tooltip to display </param>
		/// <param name="ignoreHoveredState"> If true, the tooltip will be displayed regardless if the element is hovered on or not </param>
		/// <param name="minHoveredTimeToDisplay"> Minimal cursor hover time to display the tooltip </param>
		void DrawTooltip(const std::string_view& targetObjectId, const std::string_view& tooltip, bool ignoreHoveredState = false, float minHoveredTimeToDisplay = 0.75f);
	}
}
