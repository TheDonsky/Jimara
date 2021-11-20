#include "../GtestHeaders.h"
#include "OS/IO/DirectoryChangeWatcher.h"
#include <unordered_set>


namespace Jimara {
	namespace OS {
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
	}
}
