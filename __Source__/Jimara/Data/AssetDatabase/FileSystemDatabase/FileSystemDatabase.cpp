#include "FileSystemDatabase.h"
#include "../../../OS/IO/MMappedFile.h"
#include <filesystem>
#include <shared_mutex>
#include <chrono>


namespace Jimara {
	namespace {
		static std::shared_mutex& FileSystemAsset_LoaderRegistry_Lock() {
			static std::shared_mutex lock;
			return lock;
		}
		typedef std::unordered_map<Reference<FileSystemDatabase::AssetReader::Serializer>, size_t> FileSystemAsset_ExtensionRegistry;
		typedef std::unordered_map<OS::Path, FileSystemAsset_ExtensionRegistry> FileSystemAsset_LoaderRegistry;
		static FileSystemAsset_LoaderRegistry& FileSystemAsset_AssetLoaderRegistry() {
			static FileSystemAsset_LoaderRegistry registry;
			return registry;
		}

		inline static std::vector<Reference<FileSystemDatabase::AssetReader::Serializer>> FileSystemAssetLoaders(const OS::Path& extension) {
			std::shared_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
			std::vector<Reference<FileSystemDatabase::AssetReader::Serializer>> loaders;
			FileSystemAsset_LoaderRegistry::const_iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
			if (extIt != FileSystemAsset_AssetLoaderRegistry().end())
				for (FileSystemAsset_ExtensionRegistry::const_iterator it = extIt->second.begin(); it != extIt->second.end(); ++it)
					loaders.push_back(it->first);
			return loaders;
		}
	}

	void FileSystemDatabase::AssetReader::Serializer::Register(const OS::Path& extension) {
		std::unique_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
		FileSystemAsset_LoaderRegistry::iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
		if (extIt == FileSystemAsset_AssetLoaderRegistry().end()) FileSystemAsset_AssetLoaderRegistry()[extension][this] = 1;
		else {
			FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
			if (cntIt == extIt->second.end()) extIt->second[this] = 1;
			else cntIt->second++;
		}
	}

	void FileSystemDatabase::AssetReader::Serializer::Unregister(const OS::Path& extension) {
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





	Reference<FileSystemDatabase> FileSystemDatabase::Create(
		Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, const OS::Path& assetDirectory, size_t importThreadCount) {
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
		else return Object::Instantiate<FileSystemDatabase>(graphicsDevice, audioDevice, observer, importThreadCount);
	}

	FileSystemDatabase::FileSystemDatabase(
		Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, OS::DirectoryChangeObserver* assetDirectoryObserver, size_t importThreadCount)
		: m_graphicsDevice(graphicsDevice), m_audioDevice(audioDevice), m_assetDirectoryObserver(assetDirectoryObserver) {
		// Let's make sure the configuration is valid:
		assert(m_assetDirectoryObserver != nullptr);
		if (m_graphicsDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null GraphicsDevice provided! [File:", __FILE__, "; Line:", __LINE__);
		if (m_audioDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null AudioDevice provided! [File:", __FILE__, "; Line:", __LINE__);

		m_assetDirectoryObserver->OnFileChanged() += Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		
		// We lock observers to make sure no signals come in while initializing the state:
		std::unique_lock<std::mutex> lock(m_observerLock);
		
		// Schedule all pre-existing files for scan:
		OS::Path::IterateDirectory(m_assetDirectoryObserver->Directory(), [&](const OS::Path& file) ->bool {
			QueueFile(AssetFileInfo{ file, {} });
			return true;
			});

		// Create import theads:
		if (importThreadCount <= 0) importThreadCount = 1;
		for (size_t i = 0; i < importThreadCount; i++)
			m_importThreads.push_back(std::thread([](FileSystemDatabase* self) { self->ImportThread(); }, this));
		
		// To make sure all pre-existing files are loaded when the application starts, we wait for the import queue to get empty:
		while (true) {
			{
				std::unique_lock<std::mutex> lock(m_importQueueLock);
				if (m_importQueue.empty()) break;
			}
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	FileSystemDatabase::~FileSystemDatabase() {
		m_assetDirectoryObserver->OnFileChanged() -= Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		m_dead = true;
		for (size_t i = 0; i < m_importThreads.size(); i++)
			QueueFile(AssetFileInfo{ "", {} });
		m_importAvaliable.notify_all();
		for (size_t i = 0; i < m_importThreads.size(); i++)
			m_importThreads[i].join();
	}

	Reference<Asset> FileSystemDatabase::FindAsset(const GUID& id) {
		std::unique_lock<std::mutex> lock(m_databaseLock);
		AssetsByGUID::const_iterator it = m_assetsByGUID.find(id);
		if (it == m_assetsByGUID.end()) return nullptr;
		else return it->second;
	}

	void FileSystemDatabase::ImportThread() {
		while (true) {
			AssetFileInfo fileInfo;
			{
				std::unique_lock<std::mutex> lock(m_importQueueLock);
				while (true) {
					if (m_dead) return;
					else if (!m_importQueue.empty()) break;
					else m_importAvaliable.wait(lock);
				}
				fileInfo = std::move(m_importQueue.front());
				m_importQueue.pop();
				m_queuedPaths.erase(fileInfo.filePath);
			}

			Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(fileInfo.filePath); // No logger needed; File may not be readable and it's perfectly valid..
			if (memoryMapping == nullptr) {
				std::error_code error;
				bool exists = std::filesystem::exists(fileInfo.filePath, error);
				if ((!error) && exists) {
					bool isFile = std::filesystem::is_regular_file(fileInfo.filePath, error);
					if (isFile && (!error))
						QueueFile(std::move(fileInfo));
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			// __TODO__: Check if we already have a record reffering to the given file and try to update; Otherwise, just attempt a fresh import
		}
	}

	void FileSystemDatabase::QueueFile(AssetFileInfo&& fileInfo) {
		if (fileInfo.serializers.empty()) {
			fileInfo.serializers = FileSystemAssetLoaders(fileInfo.filePath.extension());
			if (fileInfo.serializers.empty()) return;
		}
		std::lock_guard<std::mutex> lock(m_importQueueLock);
		if (m_queuedPaths.find(fileInfo.filePath) == m_queuedPaths.end()) {
			m_queuedPaths.insert(fileInfo.filePath);
			m_importQueue.push(std::move(fileInfo));
		}
		m_importAvaliable.notify_all();
	}

	void FileSystemDatabase::OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info) {
		std::unique_lock<std::mutex> lock(m_observerLock);
		if ((info.changeType == OS::DirectoryChangeObserver::FileChangeType::CREATED) ||
			(info.changeType == OS::DirectoryChangeObserver::FileChangeType::MODIFIED))
			QueueFile(AssetFileInfo{ info.filePath, {} });
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
