#pragma once
#include "../../Core/Systems/Event.h"
#include "../Logging/Logger.h"
#include "Path.h"
#include <optional>
#include <iostream>


namespace Jimara {
	namespace OS {
		/// <summary>
		/// A tool for 'observing' changes within a file system subtree of a single directory
		/// </summary>
		class DirectoryChangeObserver : public virtual Object {
		public:
			/// <summary>
			/// Creates a DirectoryChangeObserver instance for given directory
			/// </summary>
			/// <param name="directory"> Directory to observe </param>
			/// <param name="logger"> Logger for error/warning reporting </param>
			/// <param name="cached"> If true, the observer instance will be 'reused' by similar calls to the same directory </param>
			/// <returns> DirectoryChangeObserver if nothing went wrong during creation </returns>
			static Reference<DirectoryChangeObserver> Create(const Path& directory, OS::Logger* logger, bool cached = true);

			/// <summary> 'Type' of the change that occured </summary>
			enum class FileChangeType : uint8_t {
				/// <summary> Nothing happened (never reported; just a default value for 'no operation') </summary>
				NO_OP = 0,

				/// <summary> A file was just created, discovered or moved from an external directory </summary>
				CREATED = 1,

				/// <summary> A file was deleted or moved to an external directoy </summary>
				DELETED = 2,

				/// <summary> A file got renamed (do not expect moving a file from folder to folder to always report this one) </summary>
				RENAMED = 3,

				/// <summary> Content of a file got changed </summary>
				CHANGED = 4,

				/// <summary> Not a valid event type; just number of viable event types </summary>
				FileChangeType_COUNT = 5
			};

			/// <summary> Information about a change </summary>
			struct FileChangeInfo {
				/// <summary> File that has been altered (formatted as Directory()/RelativePath()) </summary>
				Path filePath;

				/// <summary> Old name of a renamed file, formatted the same as filePath (exists if and only if changeType is FileChangeType::RENAMED) </summary>
				std::optional<Path> oldPath;

				/// <summary> Type of the change that occured </summary>
				FileChangeType changeType = FileChangeType::NO_OP;

				/// <summary> Observer, reporting the change </summary>
				DirectoryChangeObserver* observer = nullptr;
			};

			/// <summary> Logger, used by the observer </summary>
			inline Logger* Log()const { return m_logger; }

			/// <summary> Target directory the observer is 'looking at' </summary>
			inline const Path& Directory()const { return m_directory; }

			/// <summary> Event, invoked each time the observer detects change in the file system (reporting may be slightly delayed, but generally will be fast enough) </summary>
			virtual Event<const FileChangeInfo&>& OnFileChanged()const = 0;


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="directory"> Target directory the observer is 'looking at' </param>
			/// <param name="logger"> Logger, used by the observer </param>
			DirectoryChangeObserver(const Path& directory, Logger* logger) : m_directory(directory), m_logger(logger) {}

		private:
			// Target directory the observer is 'looking at'
			const Path m_directory;
			
			// Logger, used by the observer
			const Reference<Logger> m_logger;
		};


		/// <summary>
		/// Outputs DirectoryChangeObserver::FileChangeType to std::ostream
		/// </summary>
		/// <param name="stream"> Stream to output to </param>
		/// <param name="type"> Value to output </param>
		/// <returns> stream </returns>
		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeType& type);

		/// <summary>
		/// Outputs DirectoryChangeObserver::FileChangeInfo to std::ostream
		/// </summary>
		/// <param name="stream"> Stream to output to </param>
		/// <param name="type"> Value to output </param>
		/// <returns> stream </returns>
		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeInfo& info);
	}
}
