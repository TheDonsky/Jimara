#include "FileSystemDatabase.h"
#include "../../../OS/IO/MMappedFile.h"
#include <filesystem>
#include <shared_mutex>
#define FileSystemDatabase_SLEEP_INTERVAL_MILLIS 1


namespace Jimara {
	namespace {
		static std::shared_mutex& FileSystemAsset_LoaderRegistry_Lock() {
			static std::shared_mutex lock;
			return lock;
		}
		typedef std::unordered_map<Reference<FileSystemAsset::Loader>, size_t> FileSystemAsset_ExtensionRegistry;
		typedef std::unordered_map<OS::Path, FileSystemAsset_ExtensionRegistry> FileSystemAsset_LoaderRegistry;
		static FileSystemAsset_LoaderRegistry& FileSystemAsset_AssetLoaderRegistry() {
			static FileSystemAsset_LoaderRegistry registry;
			return registry;
		}

		inline static std::vector<Reference<FileSystemAsset::Loader>> FileSystemAssetLoaders(const OS::Path& extension) {
			std::shared_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
			std::vector<Reference<FileSystemAsset::Loader>> loaders;
			FileSystemAsset_LoaderRegistry::const_iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
			if (extIt != FileSystemAsset_AssetLoaderRegistry().end())
				for (FileSystemAsset_ExtensionRegistry::const_iterator it = extIt->second.begin(); it != extIt->second.end(); ++it)
					loaders.push_back(it->first);
			return loaders;
		}
	}

	void FileSystemAsset::Loader::Register(const OS::Path& extension) {
		std::unique_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
		FileSystemAsset_LoaderRegistry::iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
		if (extIt == FileSystemAsset_AssetLoaderRegistry().end()) FileSystemAsset_AssetLoaderRegistry()[extension][this] = 1;
		else {
			FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
			if (cntIt == extIt->second.end()) extIt->second[this] = 1;
			else cntIt->second++;
		}
	}

	void FileSystemAsset::Loader::Unregister(const OS::Path& extension) {
		std::unique_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
		FileSystemAsset_LoaderRegistry::iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
		if (extIt == FileSystemAsset_AssetLoaderRegistry().end()) return;
		FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
		if (cntIt == extIt->second.end()) return;
		else if (cntIt->second > 1) cntIt->second--;
		else {
			extIt->second.erase(cntIt);
			if (extIt->second.empty())
				FileSystemAsset_AssetLoaderRegistry().erase(extIt);
		}
	}





	Reference<FileSystemDatabase> FileSystemDatabase::Create(Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, const OS::Path& assetDirectory) {
		assert(graphicsDevice != nullptr);
		if (audioDevice == nullptr) {
			graphicsDevice->Log()->Error("FileSystemDatabase::Create - null AudioDevice provided! [File:", __FILE__, "; Line:", __LINE__);
			return nullptr;
		}
		Reference<OS::DirectoryChangeObserver> observer = OS::DirectoryChangeObserver::Create(assetDirectory, graphicsDevice->Log(), true);
		if (observer == nullptr) {
			graphicsDevice->Log()->Error("FileSystemDatabase::Create - Failed to create a DirectoryChangeObserver for '", assetDirectory, "'! [File:", __FILE__, "; Line:", __LINE__);
			return nullptr;
		}
		else return Object::Instantiate<FileSystemDatabase>(graphicsDevice, audioDevice, observer);
	}

	FileSystemDatabase::FileSystemDatabase(Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, OS::DirectoryChangeObserver* assetDirectoryObserver)
		: m_graphicsDevice(graphicsDevice), m_audioDevice(audioDevice), m_assetDirectoryObserver(assetDirectoryObserver) {
		assert(m_assetDirectoryObserver != nullptr);
		if (m_graphicsDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null GraphicsDevice provided! [File:", __FILE__, "; Line:", __LINE__);
		if (m_audioDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null AudioDevice provided! [File:", __FILE__, "; Line:", __LINE__);

		m_assetDirectoryObserver->OnFileChanged() += Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		std::unique_lock<std::mutex> lock(m_databaseLock);
		OS::Path::IterateDirectory(m_assetDirectoryObserver->Directory(), [&](const OS::Path& file) ->bool {
			ScanFile(file);
			return true;
			});
		// __TODO__: Maybe, loading should be done asynchronously and here we should synchronise the thing before returning control to the user.
		// __SIDE_NOTE__: Probably, here we would benefit from a progress bar of sorts for the user to understand why this step may be taking this long...
	}

	FileSystemDatabase::~FileSystemDatabase() {
		m_assetDirectoryObserver->OnFileChanged() -= Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		// __TODO__: Figure out what else is going on here...
	}

	Reference<Asset> FileSystemDatabase::FindAsset(const GUID& id) {
		std::unique_lock<std::mutex> lock(m_databaseLock);
		AssetsByGUID::const_iterator it = m_assetsByGUID.find(id);
		if (it == m_assetsByGUID.end()) return nullptr;
		else return it->second;
	}

	void FileSystemDatabase::ScanFile(const OS::Path& file) {
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

	void FileSystemDatabase::OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info) {
		std::unique_lock<std::mutex> lock(m_databaseLock);
		if ((info.changeType == OS::DirectoryChangeObserver::FileChangeType::CREATED) ||
			(info.changeType == OS::DirectoryChangeObserver::FileChangeType::MODIFIED))
			ScanFile(info.filePath);
		else if (info.changeType == OS::DirectoryChangeObserver::FileChangeType::DELETED) {
			// __TODO__: 
			//		If the file was a resource, erase the record from the database, as well as the corresponding meta file;
			//		Otherwise, recreate the meta file if the resource is still available.
		}
		else if (info.changeType == OS::DirectoryChangeObserver::FileChangeType::RENAMED) {
			// __TODO__:
			//		If the resource file got renamed, make sure to rename the meta file as well;
			//		Otherwise, wait for some amount of time for the resource to be moved as well and delete the meta file if that does not happen...
		}
	}
}
