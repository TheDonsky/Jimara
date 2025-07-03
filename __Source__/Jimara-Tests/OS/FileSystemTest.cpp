#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/IO/DirectoryChangeObserver.h"
#include <condition_variable>
#include <unordered_set>
#include <sstream>
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
			const Path NON_ASCII_SUB_FL = std::string("Meshes/OBJ/ხო... კუბი.obj");
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


		namespace {
			inline static std::string ChangeToString(const DirectoryChangeObserver::FileChangeInfo& change) {
				std::stringstream stream;
				stream << change;
				return stream.str();
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
			ASSERT_NE(watcherA, nullptr);

			Reference<DirectoryChangeObserver> watcherB = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, false);
			ASSERT_NE(watcherB, nullptr);
			EXPECT_NE(watcherA, watcherB);

			Reference<DirectoryChangeObserver> watcherC = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, true);
			ASSERT_NE(watcherC, nullptr);
			EXPECT_NE(watcherA, watcherC);
			EXPECT_NE(watcherB, watcherC);

			Reference<DirectoryChangeObserver> watcherD = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, true);
			ASSERT_NE(watcherD, nullptr);
			EXPECT_NE(watcherA, watcherD);
			EXPECT_NE(watcherB, watcherD);
			EXPECT_EQ(watcherD, watcherC);

			{
				static std::mutex changeLock;
				typedef std::vector<DirectoryChangeObserver::FileChangeInfo> ChangeLog;
				ChangeLog infoA, infoB, infoC;

				static bool pushingChange = false;
				void(*changeCallback)(ChangeLog*, const DirectoryChangeObserver::FileChangeInfo&) = [](ChangeLog* log, const DirectoryChangeObserver::FileChangeInfo& change) {
					std::unique_lock<std::mutex> lock(changeLock);
					pushingChange = true;
					log->push_back(change);
					pushingChange = false;
				};

				watcherA->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoA);
				watcherB->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoB);
				watcherC->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &infoC);


				size_t messageIndex = 0;
				auto waitForMessage = [&](ChangeLog& log) -> bool {
					Stopwatch stopwatch;
					while (stopwatch.Elapsed() < 0.2f) {
						{
							std::unique_lock<std::mutex> lock(changeLock);
							if (pushingChange) logger->Fatal("Internal error! Test race condition");
							if (log.size() > messageIndex) return true;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					return false;
				};
				auto waitForMessages = [&]() -> bool {
					if ((!waitForMessage(infoA)) || (!waitForMessage(infoB)) || (!waitForMessage(infoC))) {
						watcherA = watcherB = watcherC = watcherD = nullptr;
						return false;
					}
					else return true;
				};
				auto changeString = [&](ChangeLog& log) -> std::string {
					std::unique_lock<std::mutex> lock(changeLock);
					if (pushingChange) return "... Pushing change! internal error! ...";
					else return ChangeToString(log[messageIndex]);
				};

				{
					const Path FILE_A = Path(TEST_DIRECTORY / Path("FileA"));
					const Path FILE_B = Path(TEST_DIRECTORY / Path("FileB"));
					{
						logger->Info("Creating file: '", FILE_A, "'...");
						std::ofstream stream((const std::filesystem::path&)FILE_A);

						ASSERT_TRUE(waitForMessages());

						{
							DirectoryChangeObserver::FileChangeInfo expectedChange;
							expectedChange.filePath = FILE_A;
							expectedChange.changeType = DirectoryChangeObserver::FileChangeType::CREATED;

							expectedChange.observer = watcherA;
							EXPECT_EQ(changeString(infoA), ChangeToString(expectedChange));

							expectedChange.observer = watcherB;
							EXPECT_EQ(changeString(infoB), ChangeToString(expectedChange));

							expectedChange.observer = watcherC;
							EXPECT_EQ(changeString(infoC), ChangeToString(expectedChange));
						}

						logger->Info("Writeing to file: '", FILE_A, "'...");
						for (size_t i = 0; i < (1 << 17); i++)
							stream << "AAABBBCCC" << std::endl;
						logger->Info("Done writing to : '", FILE_A, "'...");
					}

					{
						messageIndex++;
						ASSERT_TRUE(waitForMessages());

						DirectoryChangeObserver::FileChangeInfo expectedChange;
						expectedChange.filePath = FILE_A;
						expectedChange.changeType = DirectoryChangeObserver::FileChangeType::MODIFIED;

						expectedChange.observer = watcherA;
						EXPECT_EQ(changeString(infoA), ChangeToString(expectedChange));

						expectedChange.observer = watcherB;
						EXPECT_EQ(changeString(infoB), ChangeToString(expectedChange));

						expectedChange.observer = watcherC;
						EXPECT_EQ(changeString(infoC), ChangeToString(expectedChange));
					}

					{
						logger->Info("Giving the filesystem and the listener some time to flush the changes...");
						std::this_thread::sleep_for(std::chrono::seconds(5)); // Let's give it a bit of time to completely flush write calls...
						std::unique_lock<std::mutex> lock(changeLock);
						messageIndex = infoA.size();
					}

					{

						logger->Info("Renaming '", FILE_A, "' to '", FILE_B, "'...");
						std::filesystem::rename(FILE_A, FILE_B);

						ASSERT_TRUE(waitForMessages());

						DirectoryChangeObserver::FileChangeInfo expectedChange;
						expectedChange.filePath = FILE_B;
						expectedChange.oldPath = FILE_A;
						expectedChange.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;

						expectedChange.observer = watcherA;
						EXPECT_EQ(changeString(infoA), ChangeToString(expectedChange));

						expectedChange.observer = watcherB;
						EXPECT_EQ(changeString(infoB), ChangeToString(expectedChange));

						expectedChange.observer = watcherC;
						EXPECT_EQ(changeString(infoC), ChangeToString(expectedChange));
					}

					{
						logger->Info("Deleting '", FILE_B, "'...");
						std::filesystem::remove(FILE_B);

						messageIndex++;
						ASSERT_TRUE(waitForMessages());

						DirectoryChangeObserver::FileChangeInfo expectedChange;
						expectedChange.filePath = FILE_B;
						expectedChange.changeType = DirectoryChangeObserver::FileChangeType::DELETED;

						expectedChange.observer = watcherA;
						EXPECT_EQ(changeString(infoA), ChangeToString(expectedChange));

						expectedChange.observer = watcherB;
						EXPECT_EQ(changeString(infoB), ChangeToString(expectedChange));

						expectedChange.observer = watcherC;
						EXPECT_EQ(changeString(infoC), ChangeToString(expectedChange));
					}
				}

				watcherA = watcherB = watcherC = watcherD = nullptr;
			}

			std::filesystem::remove_all(TEST_DIRECTORY);
			EXPECT_EQ(logger->Numfailures(), 0);
			logger = nullptr;
			EXPECT_TRUE(snapshot.Compare());
		}

		// Test for DirectoryChangeWatcher (non-interactive, for subdirectories)
		TEST(FileSystemTest, ListenToDirectory_Subdirectories) {
			static const std::string_view TEST_DIRECTORY = "__tmp__/ListenToDirectory_Subdirectories";
			Jimara::Test::Memory::MemorySnapshot snapshot;

			std::filesystem::remove_all(TEST_DIRECTORY);
			std::filesystem::create_directories(TEST_DIRECTORY);
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			
			{
				Reference<DirectoryChangeObserver> watcher = DirectoryChangeObserver::Create(TEST_DIRECTORY, logger, false);
				ASSERT_NE(watcher, nullptr);

				static std::mutex changeLock;
				typedef std::vector<DirectoryChangeObserver::FileChangeInfo> ChangeLog;
				ChangeLog changeLog;
				void(*changeCallback)(ChangeLog*, const DirectoryChangeObserver::FileChangeInfo&) = [](ChangeLog* log, const DirectoryChangeObserver::FileChangeInfo& change) {
					change.observer->Log()->Info("Got: ", change);
					std::unique_lock<std::mutex> lock(changeLock);
					log->push_back(change);
				};
				watcher->OnFileChanged() += Callback<const DirectoryChangeObserver::FileChangeInfo&>(changeCallback, &changeLog);

				size_t messageIndex = 0;
				auto waitForMessage = [&]() -> bool {
					std::this_thread::sleep_for(std::chrono::milliseconds(128));
					Stopwatch stopwatch;
					while (stopwatch.Elapsed() < 1.0f) {
						{
							std::unique_lock<std::mutex> lock(changeLock);
							if (changeLog.size() > messageIndex) return true;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					return false;
				};
				auto waitForNextMessage = [&]() -> bool { 
					messageIndex++;
					return waitForMessage();
				};
				auto changeString = [&](size_t changeIndex) -> std::string {
					std::unique_lock<std::mutex> lock(changeLock);
					return ChangeToString(changeLog[changeIndex]);
				};
				auto messageString = [&]() -> std::string { return changeString(messageIndex); };
				auto hasMessage = [&](const DirectoryChangeObserver::FileChangeInfo& expectedMessage) -> bool {
					std::unique_lock<std::mutex> lock(changeLock);
					const std::string expected = ChangeToString(expectedMessage);
					for (size_t i = changeLog.size(); i > 0; i--)
						if (ChangeToString(changeLog[i - 1]) == expected)
							return true;
					return false;
				};
				auto clearMessages = [&]() {
					std::unique_lock<std::mutex> lock(changeLock);
					changeLog.clear();
					messageIndex = 0;
				};

				const Path SUBDIR_A = TEST_DIRECTORY / Path("dirA");
				{
					logger->Info("Creating directory: '", SUBDIR_A, "'...");
					ASSERT_TRUE(std::filesystem::create_directories(SUBDIR_A));

					ASSERT_TRUE(waitForMessage());
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = SUBDIR_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_EQ(messageString(), ChangeToString(expectedMessage));
					clearMessages();
				}

				const Path FILE_A = SUBDIR_A / Path("FileA");
				{
					logger->Info("Creating file: '", FILE_A, "'...");
					{
						std::ofstream stream((const std::filesystem::path&)FILE_A);
						stream << 'A' << std::endl;
					}

					messageIndex += 2;
					waitForMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = FILE_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::MODIFIED;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				const Path SUBDIR_A_B = SUBDIR_A / Path("dirB");
				{
					logger->Info("Creating directory: '", SUBDIR_A_B, "'...");
					std::filesystem::create_directories(SUBDIR_A_B);

					waitForNextMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = SUBDIR_A_B;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				const Path FILE_B = SUBDIR_A_B / Path("FileB");
				{
					logger->Info("Creating file: '", FILE_B, "'...");
					{
						std::ofstream stream((const std::filesystem::path&)FILE_B);
						stream << 'A' << std::endl;
					}

					messageIndex += 2;
					waitForMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = FILE_B;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::MODIFIED;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				const Path R_DIR_A = TEST_DIRECTORY / Path("r_dirA");
				const Path R_FILE_A = R_DIR_A / FILE_A.filename();
				const Path R_DIR_A_B = R_DIR_A / SUBDIR_A_B.filename();
				const Path R_FILE_B = R_DIR_A_B / FILE_B.filename();
				{
					logger->Info("Renaming '", SUBDIR_A, "' to '", R_DIR_A, "'...");
					std::filesystem::rename(SUBDIR_A, R_DIR_A);

					messageIndex += 3;
					ASSERT_TRUE(waitForMessage());

					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = R_DIR_A;
					expectedMessage.oldPath = SUBDIR_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::RENAMED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_DIR_A_B;
					expectedMessage.oldPath = SUBDIR_A_B;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_FILE_A;
					expectedMessage.oldPath = FILE_A;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_FILE_B;
					expectedMessage.oldPath = FILE_B;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Deleting '", R_DIR_A, "'...");
					std::filesystem::remove_all(R_DIR_A);

					messageIndex += 6;
					waitForMessage();

					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = R_DIR_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_DIR_A_B;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_FILE_A;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = R_FILE_B;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Creating directory: '", SUBDIR_A, "'...");
					ASSERT_TRUE(std::filesystem::create_directories(SUBDIR_A));

					ASSERT_TRUE(waitForMessage());
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = SUBDIR_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_EQ(messageString(), ChangeToString(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Creating directory: '", SUBDIR_A_B, "'...");
					std::filesystem::create_directories(SUBDIR_A_B);

					waitForNextMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = SUBDIR_A_B;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Creating file: '", FILE_A, "'...");
					{
						std::ofstream stream((const std::filesystem::path&)FILE_A);
						stream << 'A' << std::endl;
					}

					messageIndex += 2;
					waitForMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = FILE_A;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::MODIFIED;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Creating file: '", FILE_B, "'...");
					{
						std::ofstream stream((const std::filesystem::path&)FILE_B);
						stream << 'A' << std::endl;
					}

					messageIndex += 2;
					waitForMessage();
					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = FILE_B;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::CREATED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::MODIFIED;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}

				{
					logger->Info("Deleting '", SUBDIR_A_B, "'...");
					std::filesystem::remove_all(SUBDIR_A_B);

					EXPECT_TRUE(waitForNextMessage());
					waitForNextMessage();

					DirectoryChangeObserver::FileChangeInfo expectedMessage;
					expectedMessage.filePath = SUBDIR_A_B;
					expectedMessage.changeType = DirectoryChangeObserver::FileChangeType::DELETED;
					expectedMessage.observer = watcher;
					EXPECT_TRUE(hasMessage(expectedMessage));

					expectedMessage.filePath = FILE_B;
					EXPECT_TRUE(hasMessage(expectedMessage));
					clearMessages();
				}
			}

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
