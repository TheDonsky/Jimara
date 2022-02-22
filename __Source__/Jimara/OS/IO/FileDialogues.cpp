#include "FileDialogues.h"
#include <sstream>
#pragma warning(disable: 26495)
#pragma warning(disable: 26812)
#include <portable-file-dialogs.h>


namespace Jimara {
	namespace OS {
		namespace {
			inline static std::vector<std::string> ToPFDFilter(const std::vector<FileDialogueFilter>& filters) {
				std::vector<std::string> result;
				auto addFilter = [&](const FileDialogueFilter& filter) {
					result.push_back(filter.filterName);
					std::stringstream stream;
					if (filter.extensions.size() <= 0)
						stream << '*';
					else for (size_t i = 0; i < filter.extensions.size(); i++) {
						if (i > 0) stream << ' ';
						stream << filter.extensions[i];
					}
					result.push_back(stream.str());
				};
				if (filters.size() <= 0) 
					addFilter(FileDialogueFilter());
				else for (size_t i = 0; i < filters.size(); i++)
					addFilter(filters[i]);
				return result;
			}
		}

		std::vector<Path> OpenDialogue(
			const std::string_view& windowTitle,
			const std::optional<Path>& initialPath,
			const std::vector<FileDialogueFilter>& filters,
			bool allowMultiple) {
			const std::string startPath = initialPath.has_value() ? (std::string)Path(initialPath.value().root_directory()) : "";
			const std::vector<std::string> files = pfd::open_file(std::string(windowTitle), startPath,
				ToPFDFilter(filters), allowMultiple ? pfd::opt::multiselect : pfd::opt::none).result();
			std::vector<Path> result;
			for (size_t i = 0; i < files.size(); i++)
				result.push_back(files[i]);
			return result;
		}

		std::optional<Path> SaveDialogue(
			const std::string_view& windowTitle,
			const Path& initialPath,
			const std::vector<FileDialogueFilter>& filters) {
			std::string path = pfd::save_file(std::string(windowTitle), Path(initialPath.root_directory()), ToPFDFilter(filters)).result();
			return path.length() > 0 ? std::optional<Path>(Path(path)) : std::optional<Path>();
		}
	}
}

#pragma warning(default: 26495)
#pragma warning(default: 26812)
