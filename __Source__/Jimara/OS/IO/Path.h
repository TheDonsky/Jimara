#pragma once
#include "../../Core/Helpers.h"
#include <iostream>
#include <filesystem>


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Cross-platform file system path representation
		/// (plain std::filesystem::path had a few issues with wide strings when it came to cross-platform stuff)
		/// </summary>
		class Path : public std::filesystem::path {
		public:
			/// <summary> Default constructor </summary>
			inline Path() {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const std::filesystem::path& path) : std::filesystem::path(path) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(std::filesystem::path&& path) : std::filesystem::path(std::move(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const std::string_view& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const std::wstring_view& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const char* path) : Path(std::string_view(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const wchar_t* path) : Path(std::wstring_view(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const std::string& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(const std::wstring& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(path)) {}

			/// <summary>
			/// Move-Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(std::string&& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(std::move(path))) {}

			/// <summary>
			/// Move-Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			inline Path(std::wstring&& path) : std::filesystem::path(Convert<std::filesystem::path::string_type>(std::move(path))) {}

			/// <summary>
			/// Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			inline Path& operator=(const std::string_view& path) { return (*this) = Path(path); }

			/// <summary>
			/// Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			inline Path& operator=(const std::wstring_view& path) { return (*this) = Path(path); }

			/// <summary>
			/// Move-Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			inline Path& operator=(std::string_view&& path) { return (*this) = Path(std::move(path)); }

			/// <summary>
			/// Move-Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			inline Path& operator=(std::wstring_view&& path) { return (*this) = Path(std::move(path)); }

			/// <summary> Type cast to std::string </summary>
			inline operator std::string()const { return Convert<std::string>(native()); }

			/// <summary> Type cast to std::wstring </summary>
			inline operator std::wstring()const { return Convert<std::wstring>(native()); }
		};
	}
}

namespace std {
	/// <summary>
	/// Output to an std::ostream
	/// </summary>
	/// <param name="stream"> Stream to write to </param>
	/// <param name="path"> Path to print </param>
	/// <returns> stream </returns>
	inline std::ostream& operator<<(std::ostream& stream, const Jimara::OS::Path& path) {
		const std::string text(path);
		stream << text;
		return stream;
	}

	/// <summary>
	/// Output to an std::wostream
	/// </summary>
	/// <param name="stream"> Stream to write to </param>
	/// <param name="path"> Path to print </param>
	/// <returns> stream </returns>
	inline std::wostream& operator<<(std::wostream& stream, const Jimara::OS::Path& path) {
		const std::wstring text(path);
		stream << text;
		return stream;
	}

	/// <summary>
	/// Standard std::hash for Path type
	/// </summary>
	template<>
	struct hash<Jimara::OS::Path> {
		/// <summary>
		/// Calculates the hash
		/// </summary>
		/// <param name="path"> Path </param>
		/// <returns> Hash of the path </returns>
		inline size_t operator()(const Jimara::OS::Path& path)const {
			return std::hash<filesystem::path::string_type>()(path.operator std::filesystem::path::string_type());
		}
	};
}
