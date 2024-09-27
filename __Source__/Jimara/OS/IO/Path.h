#pragma once
#include "../../Core/JimaraApi.h"
#include "../../Core/Helpers.h"
#include <iostream>
#include <filesystem>
#include <unordered_set>

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
			inline operator std::string()const { return Convert<std::string>(std::move(operator std::wstring())); }

			/// <summary> Type cast to std::wstring </summary>
			inline operator std::wstring()const { 
				std::wstring result = Convert<std::wstring>(native());
				for (size_t i = 0; i < result.length(); i++)
					if (result[i] == '\\') result[i] = '/';
				return result;
			}

			/// <summary>
			/// Options for IterateDirectory
			/// </summary>
			enum class IterateDirectoryFlags : uint8_t {
				/// <summary> Does not request any reporting </summary>
				REPORT_NOTHING = 0,

				/// <summary> Requests file reporting </summary>
				REPORT_FILES = 1 << 0,

				/// <summary> Requests directory reporting </summary>
				REPORT_DIRECTORIES = 1 << 1,

				/// <summary> Requests iterating directory recursively </summary>
				REPORT_RECURSIVE = 1 << 2,

				/// <summary> Requests file reporting recursively </summary>
				REPORT_FILES_RECURSIVE = (REPORT_FILES | REPORT_RECURSIVE),

				/// <summary> Requests file directory recursively </summary>
				REPORT_DIRECTORIES_RECURSIVE = (REPORT_DIRECTORIES | REPORT_RECURSIVE),

				/// <summary> Requests reporting both files and directories </summary>
				REPORT_ALL = (REPORT_FILES | REPORT_DIRECTORIES),

				/// <summary> Requests reporting both files and directories recursively </summary>
				REPORT_ALL_RECURSIVE = (REPORT_ALL | REPORT_RECURSIVE)
			};

			/// <summary>
			/// Iterates over a directory
			/// </summary>
			/// <typeparam name="InspectFileCallback"> Function to invoke per file (has to be compatible compatible with Function<bool, const Path&>) </typeparam>
			/// <param name="path"> Directory path </param>
			/// <param name="inspectFile"> For each file in the directory subtree, invokes this function (should return true to continue iteration) </param>
			/// <param name="options"> Filtering options </param>
			template<typename InspectFileCallback>
			inline static void IterateDirectory(const Path& path, const InspectFileCallback& inspectFile, IterateDirectoryFlags options = IterateDirectoryFlags::REPORT_FILES_RECURSIVE);
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
			static thread_local filesystem::path::string_type buffer;
			buffer.clear();
			const filesystem::path::string_type& data = path.native();
			for (size_t i = 0u; i < data.length(); i++) {
				const auto sym = data[i];
				buffer += (sym == '\\') ? decltype(sym)('/') : sym;
			}
			const size_t rv = std::hash<filesystem::path::string_type>()(buffer);
			buffer.clear();
			return rv;
		}
	};
}


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Iterates over a directory
		/// </summary>
		/// <typeparam name="InspectFileCallback"> Function to invoke per file (has to be compatible compatible with Function<bool, const Path&>) </typeparam>
		/// <param name="path"> Directory path </param>
		/// <param name="inspectFile"> For each file in the directory subtree, invokes this function (should return true to continue iteration) </param>
		/// <param name="options"> Filtering options </param>
		template<typename InspectFileCallback>
		inline void Path::IterateDirectory(const Path& path, const InspectFileCallback& inspectFile, IterateDirectoryFlags options) {
			try { if (!std::filesystem::is_directory(path)) return; }
			catch (const std::exception&) { return; }
			std::unordered_set<Path> absPaths;
			bool shouldContinueIteration = true;
			bool inspectFileExceptionRaised = false;
			typedef void(*ScanDirectoryFn)(
				const std::filesystem::path&, const InspectFileCallback&, IterateDirectoryFlags, bool&, bool&, void*, 
				std::unordered_set<Path>&, const Path&);
			ScanDirectoryFn scanDirectory = [](
				const std::filesystem::path& directory, const InspectFileCallback& inspectFileFn, IterateDirectoryFlags flags,
				bool& continueIteration, bool& inspectFileException, void* recurse, 
				std::unordered_set<Path>& absolutePathCache, const Path& rootPath) {
					const bool recursiveScan = ((static_cast<uint8_t>(flags) & static_cast<uint8_t>(IterateDirectoryFlags::REPORT_RECURSIVE)) != 0);
					if (recursiveScan) {
						std::error_code error;
						Path absolutePath = std::filesystem::canonical(directory, error);
						if (error || absolutePathCache.find(absolutePath) != absolutePathCache.end()) return;
						else absolutePathCache.insert(std::move(absolutePath));
					}
					try {
						const std::filesystem::directory_iterator iterator(directory);
						const std::filesystem::directory_iterator end = std::filesystem::end(iterator);
						for (std::filesystem::directory_iterator it = std::filesystem::begin(iterator); continueIteration && it != end; ++it) {
							const std::filesystem::path& file = *it;
							const bool isDirectory = std::filesystem::is_directory(file);
							if ((static_cast<uint8_t>(flags) & static_cast<uint8_t>(isDirectory ? IterateDirectoryFlags::REPORT_DIRECTORIES : IterateDirectoryFlags::REPORT_FILES)) != 0) {
								try { continueIteration = inspectFileFn(file); }
								catch (const std::exception& e) {
									inspectFileException = true;
									throw e;
								}
							}
							if (continueIteration && recursiveScan && isDirectory)
								((ScanDirectoryFn)recurse)(file, inspectFileFn, flags, continueIteration, inspectFileException, recurse, absolutePathCache, rootPath);
						}
					}
					catch (const std::exception& e) {
						if (inspectFileException) throw e;
					}
			};
			scanDirectory(path, inspectFile, options, shouldContinueIteration, inspectFileExceptionRaised, (void*)scanDirectory, absPaths, path);
		}
	}
}
