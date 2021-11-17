#pragma once
#include <string>
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
			Path();

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const std::filesystem::path& path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(std::filesystem::path&& path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const char* path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const wchar_t* path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const std::string_view& path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const std::wstring_view& path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const std::string& path);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="path"> Path </param>
			Path(const std::wstring& path);

			/// <summary>
			/// Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			Path& operator=(const std::string_view& path);

			/// <summary>
			/// Assignmnet
			/// </summary>
			/// <param name="path"> Path </param>
			/// <returns> Self </returns>
			Path& operator=(const std::wstring_view& path);

			/// <summary> Type cast to std::string </summary>
			operator std::string()const;

			/// <summary> Type cast to std::wstring </summary>
			operator std::wstring()const;
		};
	}
}

namespace std {
	std::ostream& operator<<(std::ostream& stream, const Jimara::OS::Path& path);

	std::wostream& operator<<(std::wostream& stream, const Jimara::OS::Path& path);

	template<>
	struct hash<Jimara::OS::Path> {
		size_t operator()(const Jimara::OS::Path& path)const;
	};
}
