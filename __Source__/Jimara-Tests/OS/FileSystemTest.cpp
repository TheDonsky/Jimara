#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/IO/DirectoryChangeObserver.h"
#include <condition_variable>
#include <unordered_set>
#include <fstream>
#include <thread>
#include <tuple>


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


		// Test for DirectoryChangeWatcher (non-interactive, for basic testing)
		TEST(FileSystemTest, ListenToDirectory_Simple) {
			static const std::string_view TEST_DIRECTORY = "__tmp__/ListenToDirectory_Simple";
			std::filesystem::remove_all(TEST_DIRECTORY);
			std::filesystem::create_directories(TEST_DIRECTORY);
			{
				Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
				EXPECT_NE(DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, true), nullptr);
			}

			Jimara::Test::Memory::MemorySnapshot snapshot;
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();

			Reference<DirectoryChangeObserver> watcherA = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, false);
			EXPECT_NE(watcherA, nullptr);

			Reference<DirectoryChangeObserver> watcherB = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, false);
			EXPECT_NE(watcherB, nullptr);
			EXPECT_NE(watcherA, watcherB);

			Reference<DirectoryChangeObserver> watcherC = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, true);
			EXPECT_NE(watcherC, nullptr);
			EXPECT_NE(watcherA, watcherC);
			EXPECT_NE(watcherB, watcherC);

			Reference<DirectoryChangeObserver> watcherD = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, true);
			EXPECT_NE(watcherD, nullptr);
			EXPECT_NE(watcherA, watcherD);
			EXPECT_NE(watcherB, watcherD);
			EXPECT_EQ(watcherD, watcherC);

			{
				static std::mutex changeLock;
				static std::condition_variable changedCondition;
				typedef std::tuple<Reference<DirectoryChangeObserver>, std::vector<DirectoryChangeObserver::FileChangeInfo>, bool> ChangeLog;
				ChangeLog 
					infoA = ChangeLog(watcherA, {}, false), 
					infoB = ChangeLog(watcherB, {}, false), 
					infoC = ChangeLog(watcherC, {}, false);

				static auto logObserver = [](ChangeLog* log) -> DirectoryChangeObserver* { return std::get<0>(*log); };
				static auto logMessages = [](ChangeLog* log) -> std::vector<DirectoryChangeObserver::FileChangeInfo>& { return std::get<1>(*log); };
				static auto logError = [](ChangeLog* log) -> bool& { return std::get<2>(*log); };

				void(*changeCallback)(ChangeLog*, const DirectoryChangeObserver::FileChangeInfo&) = [](ChangeLog* log, const DirectoryChangeObserver::FileChangeInfo& change) {
					std::lock_guard<std::mutex> lock(changeLock);
					if (logObserver(log) != change.observer) logError(log) = true;
					else logMessages(log).push_back(change);
					changedCondition.notify_all();
				};

				watcherA->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoA);
				watcherB->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoB);
				watcherC->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoC);


				size_t messageIndex = 0;
				auto message = [&](ChangeLog& log) { return logMessages(&log)[messageIndex]; };
				auto waitForMessage = [&](ChangeLog& log) -> bool {
					Stopwatch stopwatch;
					std::unique_lock<std::mutex> lock(changeLock);
					while (stopwatch.Elapsed() < 0.2f) {
						if (logMessages(&log).size() > messageIndex) return true;
						else changedCondition.wait_for(lock, std::chrono::milliseconds(50));
					}
					return false;
				};

				{
					const Path FILE_A = TEST_DIRECTORY / Path("FileA");
					const Path FILE_B = TEST_DIRECTORY / Path("FileB");
					{
						std::ofstream stream(FILE_A);

						ASSERT_TRUE(waitForMessage(infoA));
						ASSERT_TRUE(waitForMessage(infoB));
						ASSERT_TRUE(waitForMessage(infoC));

						{
							std::unique_lock<std::mutex> lock(changeLock);
							EXPECT_FALSE(logError(&infoA));
							EXPECT_EQ(message(infoA).filePath, FILE_A);
							EXPECT_EQ(message(infoA).oldPath.has_value(), false);
							EXPECT_EQ(message(infoA).changeType, DirectoryChangeObserver::FileChangeType::CREATED);
							EXPECT_EQ(message(infoA).observer, watcherA);

							EXPECT_FALSE(logError(&infoB));
							EXPECT_EQ(message(infoB).filePath, FILE_A);
							EXPECT_EQ(message(infoB).oldPath.has_value(), false);
							EXPECT_EQ(message(infoB).changeType, DirectoryChangeObserver::FileChangeType::CREATED);
							EXPECT_EQ(message(infoB).observer, watcherB);

							EXPECT_FALSE(logError(&infoC));
							EXPECT_EQ(message(infoC).filePath, FILE_A);
							EXPECT_EQ(message(infoC).oldPath.has_value(), false);
							EXPECT_EQ(message(infoC).changeType, DirectoryChangeObserver::FileChangeType::CREATED);
							EXPECT_EQ(message(infoC).observer, watcherC);
						}

						std::string text;
						for (size_t i = 0; i < (1 << 20); i++)
							text += "AAABBBCCC";
						stream << text << std::endl;
						stream.close();
					}

					{
						messageIndex++;
						ASSERT_TRUE(waitForMessage(infoA));
						ASSERT_TRUE(waitForMessage(infoB));
						ASSERT_TRUE(waitForMessage(infoC));

						std::unique_lock<std::mutex> lock(changeLock);
						EXPECT_FALSE(logError(&infoA));
						EXPECT_EQ(message(infoA).filePath, FILE_A);
						EXPECT_EQ(message(infoA).oldPath.has_value(), false);
						EXPECT_EQ(message(infoA).changeType, DirectoryChangeObserver::FileChangeType::MODIFIED);
						EXPECT_EQ(message(infoA).observer, watcherA);

						EXPECT_FALSE(logError(&infoB));
						EXPECT_EQ(message(infoB).filePath, FILE_A);
						EXPECT_EQ(message(infoB).oldPath.has_value(), false);
						EXPECT_EQ(message(infoB).changeType, DirectoryChangeObserver::FileChangeType::MODIFIED);
						EXPECT_EQ(message(infoB).observer, watcherB);

						EXPECT_FALSE(logError(&infoC));
						EXPECT_EQ(message(infoC).filePath, FILE_A);
						EXPECT_EQ(message(infoC).oldPath.has_value(), false);
						EXPECT_EQ(message(infoC).changeType, DirectoryChangeObserver::FileChangeType::MODIFIED);
						EXPECT_EQ(message(infoC).observer, watcherC);
					}

					{
						std::filesystem::rename(FILE_A, FILE_B);

						messageIndex++;
						ASSERT_TRUE(waitForMessage(infoA));
						ASSERT_TRUE(waitForMessage(infoB));
						ASSERT_TRUE(waitForMessage(infoC));

						std::unique_lock<std::mutex> lock(changeLock);
						EXPECT_FALSE(logError(&infoA));
						EXPECT_EQ(message(infoA).filePath, FILE_B);
						ASSERT_EQ(message(infoA).oldPath.has_value(), true);
						EXPECT_EQ(message(infoA).oldPath.value(), FILE_A);
						EXPECT_EQ(message(infoA).changeType, DirectoryChangeObserver::FileChangeType::RENAMED);
						EXPECT_EQ(message(infoA).observer, watcherA);

						EXPECT_FALSE(logError(&infoB));
						EXPECT_EQ(message(infoB).filePath, FILE_B);
						ASSERT_EQ(message(infoB).oldPath.has_value(), true);
						EXPECT_EQ(message(infoB).oldPath.value(), FILE_A);
						EXPECT_EQ(message(infoB).changeType, DirectoryChangeObserver::FileChangeType::RENAMED);
						EXPECT_EQ(message(infoB).observer, watcherB);

						EXPECT_FALSE(logError(&infoC));
						EXPECT_EQ(message(infoC).filePath, FILE_B);
						ASSERT_EQ(message(infoC).oldPath.has_value(), true);
						EXPECT_EQ(message(infoC).oldPath.value(), FILE_A);
						EXPECT_EQ(message(infoC).changeType, DirectoryChangeObserver::FileChangeType::RENAMED);
						EXPECT_EQ(message(infoC).observer, watcherC);
					}

					{
						std::filesystem::remove(FILE_B);

						messageIndex++;
						ASSERT_TRUE(waitForMessage(infoA));
						ASSERT_TRUE(waitForMessage(infoB));
						ASSERT_TRUE(waitForMessage(infoC));

						std::unique_lock<std::mutex> lock(changeLock);
						EXPECT_FALSE(logError(&infoA));
						EXPECT_EQ(message(infoA).filePath, FILE_B);
						EXPECT_EQ(message(infoA).oldPath.has_value(), false);
						EXPECT_EQ(message(infoA).changeType, DirectoryChangeObserver::FileChangeType::DELETED);
						EXPECT_EQ(message(infoA).observer, watcherA);

						EXPECT_FALSE(logError(&infoB));
						EXPECT_EQ(message(infoB).filePath, FILE_B);
						EXPECT_EQ(message(infoB).oldPath.has_value(), false);
						EXPECT_EQ(message(infoB).changeType, DirectoryChangeObserver::FileChangeType::DELETED);
						EXPECT_EQ(message(infoB).observer, watcherB);

						EXPECT_FALSE(logError(&infoC));
						EXPECT_EQ(message(infoC).filePath, FILE_B);
						EXPECT_EQ(message(infoC).oldPath.has_value(), false);
						EXPECT_EQ(message(infoC).changeType, DirectoryChangeObserver::FileChangeType::DELETED);
						EXPECT_EQ(message(infoC).observer, watcherC);
					}
				}
			}

			watcherA = watcherB = watcherC = watcherD = nullptr;
			std::filesystem::remove_all(TEST_DIRECTORY);
			EXPECT_EQ(logger->Numfailures(), 0);
			logger = nullptr;
			EXPECT_TRUE(snapshot.Compare());
		}

		// Test for DirectoryChangeWatcher (interactive, for manual testing)
		TEST(FileSystemTest, ListenToDirectory_Manual) {
			static const std::string_view TEST_NAME = "__tmp__/ListenToDirectory_Manual";
			Jimara::Test::Memory::MemorySnapshot snapshot;

			std::filesystem::create_directories(TEST_NAME);
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<DirectoryChangeObserver> watcher = DirectoryChangeObserver::Create(TEST_NAME, logger, false);
			ASSERT_NE(watcher, nullptr);

			logger->Info("#### This is a manual test; modify files in the '", TEST_NAME, "' directory tree and observe detected changes ####");

			static std::atomic<float> timeLeft;
			timeLeft = 60.0f;

			watcher->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>([](const DirectoryChangeObserver::FileChangeInfo& info) {
				info.observer->Log()->Info(info);
				timeLeft = 300.0f;
				});

			Stopwatch stopwatch;
			while (true) {
				timeLeft = timeLeft - stopwatch.Reset();
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
