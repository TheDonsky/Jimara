#include "FileSystemDatabase.h"
#include "../../../OS/IO/MMappedFile.h"
#include <filesystem>
#include <shared_mutex>
#define FileSystemDatabase_SLEEP_INTERVAL_MILLIS 1


namespace Jimara {
	namespace {
		static std::shared_mutex fileSystemAsset_LoaderRegistry_Lock;
		typedef std::unordered_map<Reference<FileSystemAsset::Loader>, size_t> FileSystemAsset_ExtensionRegistry;
		typedef std::unordered_map<std::string, FileSystemAsset_ExtensionRegistry> FileSystemAsset_LoaderRegistry;
		static FileSystemAsset_LoaderRegistry fileSystemAsset_LoaderRegistry;

		inline static std::vector<Reference<FileSystemAsset::Loader>> FileSystemAssetLoaders(const std::string& extension) {
			std::shared_lock<std::shared_mutex> lock(fileSystemAsset_LoaderRegistry_Lock);
			std::vector<Reference<FileSystemAsset::Loader>> loaders;
			FileSystemAsset_LoaderRegistry::const_iterator extIt = fileSystemAsset_LoaderRegistry.find(extension);
			if (extIt != fileSystemAsset_LoaderRegistry.end())
				for (FileSystemAsset_ExtensionRegistry::const_iterator it = extIt->second.begin(); it != extIt->second.end(); ++it)
					loaders.push_back(it->first);
			return loaders;
		}
	}

	void FileSystemAsset::Loader::Register(const std::string& extension) {
		std::unique_lock<std::shared_mutex> lock(fileSystemAsset_LoaderRegistry_Lock);
		FileSystemAsset_LoaderRegistry::iterator extIt = fileSystemAsset_LoaderRegistry.find(extension);
		if (extIt == fileSystemAsset_LoaderRegistry.end()) fileSystemAsset_LoaderRegistry[extension][this] = 1;
		else {
			FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
			if (cntIt == extIt->second.end()) extIt->second[this] = 1;
			else cntIt->second++;
		}
	}

	void FileSystemAsset::Loader::Unregister(const std::string& extension) {
		std::unique_lock<std::shared_mutex> lock(fileSystemAsset_LoaderRegistry_Lock);
		FileSystemAsset_LoaderRegistry::iterator extIt = fileSystemAsset_LoaderRegistry.find(extension);
		if (extIt == fileSystemAsset_LoaderRegistry.end()) return;
		FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
		if (cntIt == extIt->second.end()) return;
		else if (cntIt->second > 1) cntIt->second--;
		else {
			extIt->second.erase(cntIt);
			if (extIt->second.empty())
				fileSystemAsset_LoaderRegistry.erase(extIt);
		}
	}


	FileSystemDatabase::FileSystemDatabase(Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, const std::string_view& assetDirectory) 
		: m_graphicsDevice(graphicsDevice), m_audioDevice(audioDevice), m_assetDirectory(assetDirectory) {
		static void (*rescan)(FileSystemDatabase*) = [](FileSystemDatabase* database) { database->Rescan(true, "", true); };
		static auto scanningThread = [](std::atomic<bool>* dead, Callback<> rescanAll) { while (!dead) rescanAll(); };
		m_scanningThread = std::thread(scanningThread, &m_dead, Callback<>(rescan, this));
	}

	FileSystemDatabase::~FileSystemDatabase() {
		m_dead = true;
		m_scanningThread.join();
	}

	void FileSystemDatabase::RescanAll() { Rescan(false, "", true); }

	void FileSystemDatabase::ScanDirectory(const std::string_view& directory, bool recurse) { Rescan(false, directory, recurse); }

	Reference<Asset> FileSystemDatabase::FindAsset(const GUID& id) {
		std::unique_lock<std::mutex> lock(m_databaseLock);
		AssetsByGUID::const_iterator it = m_assetsByGUID.find(id);
		if (it == m_assetsByGUID.end()) return nullptr;
		else return it->second;
	}

	void FileSystemDatabase::Rescan(bool slow, const std::string_view& subdirectory, bool recursive) {
		auto scanDirectory = [&](const std::filesystem::path& directory, auto recurse) {
			try {
				if (!std::filesystem::is_directory(directory)) return;
				{
					const std::filesystem::directory_iterator iterator(directory);
					const std::filesystem::directory_iterator end = std::filesystem::end(iterator);
					for (std::filesystem::directory_iterator it = std::filesystem::begin(iterator); it != end; ++it) {
						const std::filesystem::path& file = *it;
						if (!std::filesystem::is_directory(file)) {
							const std::string& filePath = file.u8string();
							ScanFile(filePath);
						}
						if (slow)
							std::this_thread::sleep_for(std::chrono::milliseconds(FileSystemDatabase_SLEEP_INTERVAL_MILLIS));
						if (m_dead) return;
					}
				}
				if (recursive) {
					const std::filesystem::directory_iterator iterator(directory);
					const std::filesystem::directory_iterator end = std::filesystem::end(iterator);
					for (std::filesystem::directory_iterator it = std::filesystem::begin(iterator); it != end; ++it) {
						const std::filesystem::path& file = *it;
						if (std::filesystem::is_directory(file))
							recurse(file, recurse);
						if (m_dead) return;
					}
				}
			}
			catch (const std::exception& e) {
				m_graphicsDevice->Log()->Error("FileSystemDatabase::RescanAll - Error: <", e.what(), ">!");
			}
		};
		const std::filesystem::path rootDirectory = std::filesystem::path(m_assetDirectory) / subdirectory;
		scanDirectory(rootDirectory, scanDirectory);
	}

	void FileSystemDatabase::ScanFile(const std::string& file) {
		Reference<OS::MMappedFile> mappedFile = OS::MMappedFile::Create(file, m_graphicsDevice->Log());
		const std::vector<Reference<FileSystemAsset::Loader>> loaders = [&]() {
			const std::filesystem::path filePath(file);
			const std::string extension = filePath.extension().u8string();
			return FileSystemAssetLoaders(extension);
		}();
		if (mappedFile == nullptr || loaders.empty()) {
			// __TODO__: If we have an existing asset in the database or there's a meta file somewhere, we should erase it from the existance
		}
		else {
			// __TODO__: If the asset is inside a database, we just check the size and the checksum and in case of a mismatch, update the asset; 
			// Otherwise, just go ahead and create a brand new one from scratch; Either way, store the meta file on disc...
		}
	}
}
