#include "../GtestHeaders.h"
#include "OS/IO/DirectoryChangeWatcher.h"
#include <unordered_set>


namespace Jimara {
	namespace OS {
		TEST(FileSystemTest, IterateDirectory) {
			{
				std::unordered_set<Path> paths;
				Path::IterateDirectory("./", [&](const Path& path) -> bool {
					paths.insert(path);
					std::cout << "Path found: " << path << std::endl;
					return true;
					});
				ASSERT_FALSE(true);
			}
		}
	}
}
