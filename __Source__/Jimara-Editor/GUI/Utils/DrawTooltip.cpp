#include "DrawTooltip.h"
#include <Core/Stopwatch.h>


namespace Jimara {
	namespace Editor {
		void DrawTooltip(const std::string_view& targetObjectId, const std::string_view& tooltip, bool ignoreHoveredState, float minHoveredTimeToDisplay) {
			static std::string text;
			static std::mutex tooltipLock;
			static Stopwatch stopwatch;
			if (tooltip.size() <= 0) return;
			std::unique_lock<std::mutex> lock(tooltipLock);
			if (ignoreHoveredState || ImGui::IsItemHovered()) {
				if (text != targetObjectId) {
					stopwatch.Reset();
					text = targetObjectId;
				}
				if (stopwatch.Elapsed() > minHoveredTimeToDisplay)
#pragma warning(disable: 4068)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
					ImGui::SetTooltip(tooltip.data());
#pragma GCC diagnostic pop			
#pragma warning(default: 4068)	
			}
			else if (text == targetObjectId)
				text = "";
		}
	}
}
