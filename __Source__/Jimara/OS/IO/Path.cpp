#include "Path.h"
#include "../../Core/Helpers.h"
#include <locale>


namespace Jimara {
	namespace OS {
		Path::Path() {}

		Path::Path(const char* path) : Path(std::string_view(path)) {}

		Path::Path(const wchar_t* path) : Path(std::wstring_view(path)) {}

		Path::Path(const std::string_view& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

		Path::Path(const std::wstring_view& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

		Path::Path(const std::string& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

		Path::Path(const std::wstring& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

		Path& Path::operator=(const std::string_view& path) { return (*this) = Path(path); }

		Path& Path::operator=(const std::wstring_view& path) { return (*this) = Path(path);  }

		Path::operator std::string()const { return Convert<std::string>(native()); }

		Path::operator std::wstring()const { return Convert<std::wstring>(native()); }
	}
}

namespace std {
	std::ostream& operator<<(std::ostream& stream, const Jimara::OS::Path& path) {
		const std::string text(path);
		stream << text;
		return stream;
	}

	std::wostream& operator<<(std::wostream& stream, const Jimara::OS::Path& path) {
		const std::wstring text(path);
		stream << text;
		return stream;
	}

	size_t hash<Jimara::OS::Path>::operator()(const Jimara::OS::Path& path)const {
		return std::hash<filesystem::path::string_type>()(path.operator std::filesystem::path::string_type());
	}
}
