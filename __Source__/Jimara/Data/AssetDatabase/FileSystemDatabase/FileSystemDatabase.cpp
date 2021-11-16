#include "FileSystemDatabase.h"
#include <filesystem>
#define FileSystemDatabase_SLEEP_INTERVAL_MILLIS 1


namespace Jimara {
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
							m_graphicsDevice->Log()->Info("File discovered: ", file);
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
}
