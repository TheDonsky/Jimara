#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/IO/DirectoryChangeObserver.h"
#include <unordered_set>
#include <thread>


namespace Jimara {
	namespace OS {
		// Basic test for Path::IterateDirectory() function
		TEST(FileSystemTest, IterateDirectory) {
			const Path WORKING_DIRECTORY = "./";
			const Path TEST_EXECUTABLE_RELATIVE =
#ifdef _WIN32
				"Jimara-Test.exe";
#else
				"Jimara-Test";
#endif
			const Path TEST_EXECUTABLE = WORKING_DIRECTORY / TEST_EXECUTABLE_RELATIVE;
			const Path SHADERS = "./Shaders";
			const Path SHADERS_RELATIVE = "Shaders";
			const Path ASSETS = "./Assets";
			const Path ASSETS_RELATIVE =
#ifdef _WIN32
				"../../../../../Jimara-BuiltInAssets";
#else
				"../../../Jimara-BuiltInAssets";
#endif
			const Path MESHES = ASSETS / "Meshes";
			const Path MESHES_RELATIVE = ASSETS_RELATIVE / "Meshes";
			const Path ASSETS_LICENSE_FILE = ASSETS / "LICENSE";
			const Path ASSETS_LICENSE_FILE_RELATIVE = ASSETS_RELATIVE / "LICENSE";
			const Path NON_ASCII_SUB_FL = L"Meshes/OBJ/ხო... კუბი.obj";
			const Path NON_ASCII_FILE = ASSETS / NON_ASCII_SUB_FL;
			const Path NON_ASCII_FILE_RELATIVE = ASSETS_RELATIVE / NON_ASCII_SUB_FL;

			std::unordered_set<Path> paths;
			std::unordered_set<Path> relpaths;
			auto iterate = [&](Path::IterateDirectoryFlags flags) {
				paths.clear();
				relpaths.clear();
				Path::IterateDirectory(WORKING_DIRECTORY, [&](const Path& path) -> bool {
					const Path relPath = std::filesystem::relative(path, WORKING_DIRECTORY);
					paths.insert(path);
					relpaths.insert(relPath);
					return true;
					}, flags);
			};
			auto pathsContain = [&](const Path& path) { return paths.find(path) != paths.end(); };
			auto relpathsContain = [&](const Path& path) { return relpaths.find(path) != relpaths.end(); };

			{
				iterate(Path::IterateDirectoryFlags::REPORT_NOTHING);
				EXPECT_TRUE(paths.empty());
				EXPECT_TRUE(relpaths.empty());
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_FILES);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(!relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(!relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(!relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_DIRECTORIES);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(!relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_RECURSIVE);

				EXPECT_TRUE(paths.empty());
				EXPECT_TRUE(relpaths.empty());
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_FILES_RECURSIVE);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(!relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(!relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(!relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_DIRECTORIES_RECURSIVE);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_ALL);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(!relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
			{
				iterate(Path::IterateDirectoryFlags::REPORT_ALL_RECURSIVE);

				EXPECT_TRUE(!pathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(!pathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(pathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(pathsContain(SHADERS));
				EXPECT_TRUE(!pathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS));
				EXPECT_TRUE(!pathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(pathsContain(MESHES));
				EXPECT_TRUE(!pathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(pathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(!pathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(pathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(!pathsContain(NON_ASCII_FILE_RELATIVE));

				EXPECT_TRUE(!relpathsContain(WORKING_DIRECTORY));
				EXPECT_TRUE(relpathsContain(TEST_EXECUTABLE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(TEST_EXECUTABLE));
				EXPECT_TRUE(!relpathsContain(SHADERS));
				EXPECT_TRUE(relpathsContain(SHADERS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS));
				EXPECT_TRUE(relpathsContain(ASSETS_RELATIVE));
				EXPECT_TRUE(!relpathsContain(MESHES));
				EXPECT_TRUE(relpathsContain(MESHES_RELATIVE));
				EXPECT_TRUE(!relpathsContain(ASSETS_LICENSE_FILE));
				EXPECT_TRUE(relpathsContain(ASSETS_LICENSE_FILE_RELATIVE));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_SUB_FL));
				EXPECT_TRUE(!relpathsContain(NON_ASCII_FILE));
				EXPECT_TRUE(relpathsContain(NON_ASCII_FILE_RELATIVE));
			}
		}


		// Basic test for DirectoryChangeWatcher (interactive, for manual testing)
		TEST(FileSystemTest, ListenToDirectory_Manual) {
			static const std::string_view TEST_NAME = "ListenToDirectory_Manual";
			Jimara::Test::Memory::MemorySnapshot snapshot;

			std::filesystem::create_directories(TEST_NAME);
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<DirectoryChangeObserver> watcher = DirectoryChangeObserver::Create(TEST_NAME, logger, false);
			ASSERT_NE(watcher, nullptr);

			logger->Info("#### This is a manual test; modify files in the '", TEST_NAME, "' directory tree and observe detected changes ####");

			static Stopwatch stopwatch;
			stopwatch.Reset();

			watcher->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>([](const DirectoryChangeObserver::FileChangeInfo& info) {
				info.observer->Log()->Info(info);
				stopwatch.Reset();
				});

			while (true) {
				const float timeLeft = (60.0f - stopwatch.Elapsed());
				if (timeLeft <= 0.0f) break;
				logger->Info("Test terminating in ", static_cast<int>(timeLeft), " seconds... (modify any file from '", TEST_NAME, "' to reset the timer)");
				std::this_thread::sleep_for(std::chrono::milliseconds(std::max(static_cast<int>(1000.0f * timeLeft / 3.0f), 1000)));
			}

			watcher = nullptr;
			logger = nullptr;
			std::filesystem::remove_all(TEST_NAME);

			EXPECT_TRUE(snapshot.Compare());
		}
	}
}
