#pragma once
#include "../../Core/Systems/Event.h"
#include "../Logging/Logger.h"
#include "Path.h"
#include <optional>


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

			virtual const Path& Directory()const = 0;

			virtual Event<const FileChangeInfo&>& OnFileChanged()const = 0;
		};
	}
}
