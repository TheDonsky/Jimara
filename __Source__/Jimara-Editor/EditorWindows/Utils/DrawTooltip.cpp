#include "DrawTooltip.h"
#include <Core/Stopwatch.h>


namespace Jimara {
	namespace Editor {
		void DrawTooltip(const std::string_view& targetObjectId, const std::string_view& tooltip, float minHoveredTimeToDisplay) {
			static std::string text;
			static std::mutex tooltipLock;
			static Stopwatch stopwatch;
			std::unique_lock<std::mutex> lock(tooltipLock);
			if (ImGui::IsItemHovered()) {
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
