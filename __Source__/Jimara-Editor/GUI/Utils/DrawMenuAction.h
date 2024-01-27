#pragma once
#include "../ImGuiRenderer.h"
#include "DrawTooltip.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Adds/Draws an arbitary chain of ImGui::MenuItem heirarchy and returns if the action gets clicked
		/// </summary>
		/// <param name="menuPath"> Menu path (sub-items separated by '/') </param>
		/// <param name="tooltip"> Tooltip of the object </param>
		/// <param name="actionId"> Unique identifier for the corresponding action-on-click </param>
		/// <param name="isSelected"> If true, the item will be displayed as selected selectable </param>
		/// <returns> True, if the action gets clicked </returns>
		inline static bool DrawMenuAction(const std::string_view& menuPath, const std::string_view& tooltip, const void* actionId, bool isSelected = false) {
			static thread_local std::vector<char> nameBuffer;
			size_t menus = 0;
			bool rv = false;
			for (size_t i = 0; i < menuPath.size(); i++) {
				const char symbol = menuPath[i];
				if (i >= (menuPath.size() - 1)) {
					nameBuffer.push_back(symbol);
					{
						static const char* ACTION_ID_PRE = "###MenuAction_";
						const char* ptr = ACTION_ID_PRE;
						while (true) {
							const char c = *ptr;
							if (c == 0) break;
							else {
								nameBuffer.push_back(c);
								ptr++;
							}
						}
					}
					{
						size_t ptrToI = ((size_t)actionId);
						for (size_t p = 0; p < (sizeof(size_t) * 2); p++) {
							nameBuffer.push_back('a' + (char)(ptrToI & 15));
							ptrToI >>= 4;
						}
					}
					nameBuffer.push_back(0);
					rv = ImGui::MenuItem(nameBuffer.data(), nullptr, isSelected);
					DrawTooltip(menuPath, tooltip);
					break;
				}
				else if (symbol == '/' || symbol == '\\') {
					if (nameBuffer.size() > 0) {
						nameBuffer.push_back(0);
						if (!ImGui::BeginMenu(nameBuffer.data())) break;
						else {
							nameBuffer.clear();
							menus++;
						}
					}
				}
				else nameBuffer.push_back(symbol);
			}
			nameBuffer.clear();
			for (size_t i = 0; i < menus; i++)
				ImGui::EndMenu();
			return rv;
		}
	}
}

