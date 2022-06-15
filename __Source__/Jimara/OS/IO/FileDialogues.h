#pragma once
#include "Path.h"
#include <vector>
#include <string_view>
#include <optional>


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Extension filter for OpenDialogue/SaveDialogue
		/// </summary>
		struct JIMARA_API FileDialogueFilter {
			/// <summary> Filter name </summary>
			std::string filterName;

			/// <summary> List of extensions </summary>
			std::vector<std::string> extensions;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Filter name </param>
			/// <param name="ext"> List of extensions </param>
			inline FileDialogueFilter(
				const std::string_view& name = "AllFiles",
				const std::vector<std::string>& ext = { "*" })
				: filterName(name), extensions(ext) {}
		};

		/// <summary>
		/// Creates "Open File" dialogue
		/// </summary>
		/// <param name="windowTitle"> Title of the dialogue window </param>
		/// <param name="initialPath"> Default path </param>
		/// <param name="filters"> Extension filters </param>
		/// <param name="allowMultiple"> If true, selection will be enabled </param>
		/// <returns> List of selected paths </returns>
		JIMARA_API std::vector<Path> OpenDialogue(
			const std::string_view& windowTitle = "Open File",
			const std::optional<Path>& initialPath = std::optional<Path>(),
			const std::vector<FileDialogueFilter>& filters = {},
			bool allowMultiple = false);

		/// <summary>
		/// Creates "Save File [as]" dialogue
		/// </summary>
		/// <param name="windowTitle"> Title of the dialogue window </param>
		/// <param name="initialPath"> Default path </param>
		/// <param name="filters"> Extension filters </param>
		/// <returns> Selected path or empty if nothing gets selected </returns>
		JIMARA_API std::optional<Path> SaveDialogue(
			const std::string_view& windowTitle,
			const Path& initialPath,
			const std::vector<FileDialogueFilter>& filters = {});
	}
}
