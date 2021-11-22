#pragma once
#include "../../Core/Systems/Event.h"
#include "../Logging/Logger.h"
#include "Path.h"
#include <optional>
#include <iostream>


namespace Jimara {
	namespace OS {
		class DirectoryChangeObserver : public virtual Object {
		public:
			static Reference<DirectoryChangeObserver> Create(const Path& directory, OS::Logger* logger, bool cached = true);

			enum class FileChangeType : uint8_t {
				NO_OP = 0,
				CREATED = 1,
				DELETED = 2,
				RENAMED = 3,
				CHANGED = 4,
				FileChangeType_COUNT = 5
			};

			struct FileChangeInfo {
				Path filePath;
				std::optional<Path> oldPath;
				FileChangeType changeType = FileChangeType::NO_OP;
				DirectoryChangeObserver* observer = nullptr;
			};

			inline Logger* Log()const { return m_logger; }

			inline const Path& Directory()const { return m_directory; }

			virtual Event<const FileChangeInfo&>& OnFileChanged()const = 0;

		protected:
			DirectoryChangeObserver(const Path& directory, Logger* logger) : m_directory(directory), m_logger(logger) {}

		private:
			const Path m_directory;
			const Reference<Logger> m_logger;
		};

		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeType& type);
		std::ostream& operator<<(std::ostream& stream, const DirectoryChangeObserver::FileChangeInfo& info);
	}
}
