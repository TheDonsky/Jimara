#include "FileSystemDatabase.h"
#include "../../../OS/IO/MMappedFile.h"
#include "../../Serialization/Helpers/SerializeToJson.h"
#include <filesystem>
#include <fstream>
#include <shared_mutex>
#include <chrono>


namespace Jimara {
	namespace {
		static std::shared_mutex& FileSystemAsset_LoaderRegistry_Lock() {
			static std::shared_mutex lock;
			return lock;
		}
		typedef std::unordered_map<Reference<FileSystemDatabase::AssetImporter::Serializer>, size_t> FileSystemAsset_ExtensionRegistry;
		typedef std::unordered_map<OS::Path, FileSystemAsset_ExtensionRegistry> FileSystemAsset_LoaderRegistry;
		static FileSystemAsset_LoaderRegistry& FileSystemAsset_AssetLoaderRegistry() {
			static FileSystemAsset_LoaderRegistry registry;
			return registry;
		}

		inline static std::vector<Reference<FileSystemDatabase::AssetImporter::Serializer>> FileSystemAssetLoaders(const OS::Path& extension) {
			std::shared_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
			std::vector<Reference<FileSystemDatabase::AssetImporter::Serializer>> loaders;
			FileSystemAsset_LoaderRegistry::const_iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
			if (extIt != FileSystemAsset_AssetLoaderRegistry().end())
				for (FileSystemAsset_ExtensionRegistry::const_iterator it = extIt->second.begin(); it != extIt->second.end(); ++it)
					loaders.push_back(it->first);
			return loaders;
		}
	}

	Graphics::GraphicsDevice* FileSystemDatabase::AssetImporter::GraphicsDevice()const { return m_context->graphicsDevice; }

	Audio::AudioDevice* FileSystemDatabase::AssetImporter::AudioDevice()const { return m_context->audioDevice; }

	OS::Path FileSystemDatabase::AssetImporter::AssetFilePath()const {
		OS::Path path;
		{
			std::unique_lock<std::mutex> lock(m_pathLock);
			path = m_path;
		}
		return path;
	}

	OS::Logger* FileSystemDatabase::AssetImporter::Log()const { return GraphicsDevice()->Log(); }

	Reference<Asset> FileSystemDatabase::AssetImporter::FindAsset(const GUID& id)const {
		Reference<AssetDatabase> db;
		if (m_context != nullptr) {
			std::unique_lock<SpinLock>(m_context->ownerLock);
			db = m_context->owner;
		}
		if (db == nullptr) return nullptr;
		else return db->FindAsset(id);
	}

	void FileSystemDatabase::AssetImporter::Serializer::Register(const OS::Path& extension) {
		if (this == nullptr) return;
		std::unique_lock<std::shared_mutex> lock(FileSystemAsset_LoaderRegistry_Lock());
		FileSystemAsset_LoaderRegistry::iterator extIt = FileSystemAsset_AssetLoaderRegistry().find(extension);
		if (extIt == FileSystemAsset_AssetLoaderRegistry().end()) FileSystemAsset_AssetLoaderRegistry()[extension][this] = 1;
		else {
			FileSystemAsset_ExtensionRegistry::iterator cntIt = extIt->second.find(this);
			if (cntIt == extIt->second.end()) extIt->second[this] = 1;
			else cntIt->second++;
		}
	}

	void FileSystemDatabase::AssetImporter::Serializer::Unregister(const OS::Path& extension) {
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
		Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, const OS::Path& assetDirectory, 
		Callback<size_t, size_t> reportImportProgress, size_t importThreadCount, const OS::Path& metadataExtension) {
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
		else return Object::Instantiate<FileSystemDatabase>(graphicsDevice, audioDevice, observer, importThreadCount, metadataExtension, reportImportProgress);
	}

	FileSystemDatabase::FileSystemDatabase(
		Graphics::GraphicsDevice* graphicsDevice, Audio::AudioDevice* audioDevice, OS::DirectoryChangeObserver* assetDirectoryObserver, 
		size_t importThreadCount, const OS::Path& metadataExtension, Callback<size_t, size_t> reportImportProgress)
		: m_context([&]() -> Reference<Context> {
		Reference<Context> ctx = Object::Instantiate<Context>();
		ctx->graphicsDevice = graphicsDevice;
		ctx->audioDevice = audioDevice;
		return ctx;
			}())
		, m_assetDirectoryObserver(assetDirectoryObserver)
		, m_metadataExtension([&]()->OS::Path {
				std::wstring ext = metadataExtension;
				if (ext.length() <= 0) ext = OS::Path(DefaultMetadataExtension());
				if (ext[0] != L'.') ext = L'.' + ext;
				return ext;
					}()) {
		// Let's make sure the configuration is valid:
		assert(m_assetDirectoryObserver != nullptr);
		if (m_context->graphicsDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null GraphicsDevice provided! [File:", __FILE__, "; Line:", __LINE__);
		if (m_context->audioDevice == nullptr)
			m_assetDirectoryObserver->Log()->Fatal("FileSystemDatabase::FileSystemDatabase - null AudioDevice provided! [File:", __FILE__, "; Line:", __LINE__);
		m_context->owner = this;

		m_assetDirectoryObserver->OnFileChanged() += Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		
		// We lock observers to make sure no signals come in while initializing the state:
		std::unique_lock<std::mutex> lock(m_observerLock);
		
		// Create initial import theads:
		if (importThreadCount <= 0) importThreadCount = 1;
		auto createImportThreads = [&]() {
			for (size_t i = 0; i < importThreadCount; i++)
				m_importThreads.push_back(std::thread([](FileSystemDatabase* self) { self->ImportThread(); }, this));
		};
		createImportThreads();

		// Schedule all pre-existing files for scan:
		size_t totalFileCount = 0;
		OS::Path::IterateDirectory(m_assetDirectoryObserver->Directory(), [&](const OS::Path& file) ->bool {
			QueueFile(AssetFileInfo{ file, {} });
			totalFileCount++;
			return true;
			});
		
		// To make sure all pre-existing files are loaded when the application starts, we wait for the import queue to get empty:
		while (true) {
			while (true) {
				size_t numProcessed = 0;
				{
					std::unique_lock<std::mutex> lock(m_importQueueLock);
					if (m_importQueue.empty()) break;
					else numProcessed = (m_importQueue.size() < totalFileCount) ? (totalFileCount - m_importQueue.size()) : 0;
				}
				reportImportProgress(numProcessed, totalFileCount);
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}

			// Temporarily kill the import threads 
			{
				{
					std::unique_lock<std::mutex> lock(m_importQueueLock);
					m_dead = true;
					m_importAvaliable.notify_all();
				}
				for (size_t i = 0; i < m_importThreads.size(); i++)
					m_importThreads[i].join();
				m_importThreads.clear();
				m_dead = false;
			}

			if (m_importQueue.empty()) {
				reportImportProgress(totalFileCount, totalFileCount);
				break;
			}
			else createImportThreads();
		}

		// Recreate import theads:
		createImportThreads();
	}

	FileSystemDatabase::~FileSystemDatabase() {
		m_assetDirectoryObserver->OnFileChanged() -= Callback(&FileSystemDatabase::OnFileSystemChanged, this);
		{
			std::unique_lock<std::mutex> lock(m_importQueueLock);
			m_dead = true;
			m_importAvaliable.notify_all();
		}
		for (size_t i = 0; i < m_importThreads.size(); i++)
			m_importThreads[i].join();
		{
			std::unique_lock<SpinLock> lock(m_context->ownerLock);
			m_context->owner = nullptr;
		}
	}

	const OS::Path& FileSystemDatabase::AssetDirectory()const { return m_assetDirectoryObserver->Directory(); }

	Reference<Asset> FileSystemDatabase::FindAsset(const GUID& id) {
		std::unique_lock<std::recursive_mutex> lock(m_databaseLock);
		AssetCollection::InfoByGUID::const_iterator it = m_assetCollection.infoByGUID.find(id);
		if (it == m_assetCollection.infoByGUID.end()) return nullptr;
		else {
			Reference<Asset> asset = it->second->m_asset;
			return asset;
		}
	}

	bool FileSystemDatabase::TryGetAssetInfo(const Asset* asset, AssetInformation& info)const {
		if (asset == nullptr) return false;
		else return TryGetAssetInfo(asset->Guid(), info);
	}

	bool FileSystemDatabase::TryGetAssetInfo(const GUID& id, AssetInformation& info)const {
		std::unique_lock<std::recursive_mutex> lock(m_databaseLock);
		AssetCollection::InfoByGUID::const_iterator it = m_assetCollection.infoByGUID.find(id);
		if (it == m_assetCollection.infoByGUID.end()) return false;
		else {
			info = *dynamic_cast<const AssetInformation*>(it->second.operator->());
			return true;
		}
	}

	void FileSystemDatabase::GetAssetsOfType(const TypeId& resourceType, const Callback<const AssetInformation&>& reportAsset, bool exactType)const {
		std::unique_lock<std::recursive_mutex> lock(m_databaseLock);
		AssetCollection::IndexPerType::const_iterator typeIt = m_assetCollection.indexPerType.find(resourceType);
		if (typeIt == m_assetCollection.indexPerType.end()) return;
		const AssetCollection::TypeIndex::Set& set = typeIt->second.set;
		if (exactType) {
			for (AssetCollection::TypeIndex::Set::const_iterator setIt = set.begin(); setIt != set.end(); ++setIt)
				if (setIt->operator->()->m_asset->ResourceType() == resourceType)
					reportAsset(**setIt);
		}
		else for (AssetCollection::TypeIndex::Set::const_iterator setIt = set.begin(); setIt != set.end(); ++setIt)
			reportAsset(**setIt);
	}

	void FileSystemDatabase::GetAssetsByName(
		const std::string& name, const Callback<const AssetInformation&>& reportAsset, bool exactName, const TypeId& resourceType, bool exactType)const {
		std::unique_lock<std::recursive_mutex> lock(m_databaseLock);
		AssetCollection::IndexPerType::const_iterator typeIt = m_assetCollection.indexPerType.find(resourceType);
		if (typeIt == m_assetCollection.indexPerType.end()) return;
		const AssetCollection::TypeIndex::NameIndex& index = typeIt->second.nameIndex;
		AssetCollection::TypeIndex::NameIndex::const_iterator nameIt = index.find(name);
		if (nameIt == index.end()) return;
		const std::set<Reference<AssetCollection::Info>>& infos = nameIt->second;
		for (std::set<Reference<AssetCollection::Info>>::const_iterator infoIt = infos.begin(); infoIt != infos.end(); ++infoIt) {
			const AssetCollection::Info* info = *infoIt;
			if (exactType && info->m_asset->ResourceType() != resourceType) continue;
			else if (exactName && info->m_resourceName != name) continue;
			else reportAsset(*info);
		}
	}

	namespace {
		inline static OS::Path SafeCannonicalPathFromPath(const OS::Path& path) {
			std::error_code error;
			const OS::Path cannonical = std::filesystem::canonical(path, error);
			if (error) return path;
			else return cannonical;
		}
	}

	void FileSystemDatabase::GetAssetsFromFile(
		const OS::Path& sourceFilePath, const Callback<const AssetInformation&>& reportAsset, const TypeId& resourceType, bool exactType)const {
		const OS::Path cannonicalPath = SafeCannonicalPathFromPath(sourceFilePath);
		AssetCollection::IndexPerType::const_iterator typeIt = m_assetCollection.indexPerType.find(resourceType);
		if (typeIt == m_assetCollection.indexPerType.end()) return;
		const AssetCollection::TypeIndex::PathIndex& index = typeIt->second.pathIndex;
		AssetCollection::TypeIndex::PathIndex::const_iterator pathIt = index.find(cannonicalPath);
		if (pathIt == index.end()) return;
		const std::set<Reference<AssetCollection::Info>>& infos = pathIt->second;
		if (exactType) {
			for (std::set<Reference<AssetCollection::Info>>::const_iterator setIt = infos.begin(); setIt != infos.end(); ++setIt)
				if (setIt->operator->()->m_asset->ResourceType() == resourceType)
					reportAsset(**setIt);
		}
		else for (std::set<Reference<AssetCollection::Info>>::const_iterator setIt = infos.begin(); setIt != infos.end(); ++setIt)
			reportAsset(**setIt);
	}

	size_t FileSystemDatabase::AssetCount()const {
		std::unique_lock<std::recursive_mutex> lock(m_databaseLock);
		return m_assetCollection.infoByGUID.size();
	}

	Event<FileSystemDatabase::DatabaseChangeInfo>& FileSystemDatabase::OnDatabaseChanged()const { return m_onDatabaseChanged; }

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

			ImportFile(fileInfo);
		}
	}

	namespace {
		inline static OS::Path MetadataPath(const OS::Path& filePath, const OS::Path& metadataExtension) {
			return filePath.native() + metadataExtension.native();
		}

		inline static void StoreMetadata(const Serialization::SerializedObject& serializedObject, OS::Logger* logger, const OS::Path& metadataPath, nlohmann::json* lastMetadata) {
			bool error = false;
			nlohmann::json metadata = Serialization::SerializeToJson(serializedObject, logger, error,
				[&](const Serialization::SerializedObject&, bool& error) {
					logger->Error("FileSystemDatabase::StoreMetadata - Metadata files are not expected to contain any object pointers! <SerializeToJson>");
					error = true;
					return nlohmann::json();
				});
			if (error) {
				logger->Error("FileSystemDatabase::StoreMetadata - Failed to serialize asset importer! (Metadata Path: '", metadataPath, "')");
			}
			else if (lastMetadata != nullptr && (*lastMetadata) == metadata) return;
			else {
				std::ofstream stream(metadataPath);
				if (stream.is_open())
					stream << metadata.dump(1, '\t') << std::endl;
				else logger->Error("FileSystemDatabase::StoreMetadata - Failed to store metadata! (Path: '", metadataPath, "')");
				if (lastMetadata != nullptr)
					(*lastMetadata) = std::move(metadata);
			}
		}
	}

	void FileSystemDatabase::ImportFile(const AssetFileInfo& fileInfo) {
		// If there are no serializers, we don't care about this file...
		if (fileInfo.serializers.size() <= 0) return;
		Reference<PathLock> pathLockInstance = m_pathLockCache.LockFor(fileInfo.filePath);
		std::unique_lock<std::mutex> filePathLock(*pathLockInstance);

		// Metadata path and json
		const OS::Path metadataPath = MetadataPath(fileInfo.filePath, m_metadataExtension);
		nlohmann::json metadataJson({});
		if (std::filesystem::exists(metadataPath)) {
			const Reference<OS::MMappedFile> metadataMapping = OS::MMappedFile::Create(metadataPath);
			if (metadataMapping != nullptr) {
				const MemoryBlock block = *metadataMapping;
				try { metadataJson = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size())); }
				catch (nlohmann::json::parse_error&) { metadataJson = nlohmann::json({}); }
			}
		}

		// Creates a reader, given a serializer:
		auto createReader = [&](AssetImporter::Serializer* serializer) -> Reference<AssetImporter> {
			Reference<AssetImporter> reader = serializer->CreateReader();
			if (reader == nullptr) return nullptr;
			else {
				FileSystemDatabase* prevOwner = nullptr;
				if (!reader->m_owner.compare_exchange_strong(prevOwner, this)) {
					m_assetDirectoryObserver->Log()->Error(
						"FileSystemDatabase::ImportFile - AssetImporter::Serializer::CreateReader() returned an instance of an AssetImporter that's already in use (path:'",
						fileInfo.filePath, "')! [File:", __FILE__, "; Line:", __LINE__);
					return nullptr;
				}
			}
			reader->m_context = m_context;
			// Note: Path will be set inside import()...

			// Load from metadata if possible:
			if (!Serialization::DeserializeFromJson(serializer->Serialize(reader), metadataJson, m_assetDirectoryObserver->Log(),
				[&](const Serialization::SerializedObject&, const nlohmann::json&) {
					m_assetDirectoryObserver->Log()->Error("FileSystemDatabase::ImportFile - Metadata files are not expected to contain any object pointers! <DeserializeFromJson>");
					return false;
				})) m_assetDirectoryObserver->Log()->Warning("FileSystemDatabase::ImportFile - Metadata deserialization failed!");

			return reader;
		};

		// Tries to import asset, given a reader:
		std::vector<AssetImporter::AssetInfo> assets;
		auto importAssets = [&](AssetImporter* reader) -> bool {
			{
				std::unique_lock<std::mutex> lock(reader->m_pathLock);
				reader->m_path = fileInfo.filePath;
			}
			assets.clear();
			void (*recordAsset)(std::vector<AssetImporter::AssetInfo>*, const AssetImporter::AssetInfo&) =
				[](std::vector<AssetImporter::AssetInfo>* assets, const AssetImporter::AssetInfo& asset) { if (asset.asset != nullptr) assets->push_back(asset); };
			return reader->Import(Callback<const AssetImporter::AssetInfo&>(recordAsset, &assets));
		};

		// Retrieves stored reader information if there exists one:
		auto getStoredReaderInfo = [&]() -> Reference<AssetReaderInfo> {
			std::unique_lock<std::mutex> lock(m_pathReaderLock);
			PathReaderInfo::const_iterator it = m_pathReaders.find(fileInfo.filePath);
			if (it == m_pathReaders.end()) return nullptr;
			else {
				Reference<AssetReaderInfo> readerInfo = it->second;
				return readerInfo;
			}
		};

		// Updates reader information:
		auto updateReaderInfo = [&](AssetReaderInfo* info) -> bool {
			if (info == nullptr) return false;
			else if (info->reader == nullptr) return false;
			else if (!importAssets(info->reader)) return false;
			std::unique_lock<std::mutex> pathLock(m_pathReaderLock);
			{
				PathReaderInfo::const_iterator it = m_pathReaders.find(fileInfo.filePath);
				if (it == m_pathReaders.end()) 
					m_pathReaders[fileInfo.filePath] = info;
				else if (it->second != info) {
					m_assetDirectoryObserver->Log()->Error("FileSystemDatabase::ImportFile - More than one thread is operating on the same resource file ('",
						fileInfo.filePath, "'; AssetReaderInfo changed)! [File:", __FILE__, "; Line:", __LINE__); // filePathLock should guarantee, this can never happen, but whatever...
					return false;
				}
			}
			{
				std::unique_lock<std::recursive_mutex> dbLock(m_databaseLock);
				for (size_t i = 0; i < info->assets.size(); i++)
					m_assetCollection.RemoveAsset(info->assets[i].asset);
				for (size_t i = 0; i < assets.size(); i++)
					m_assetCollection.InsertAsset(assets[i], info->reader);
			}
			{
				std::unordered_map<GUID, AssetChangeType> changes;
				for (size_t i = 0; i < info->assets.size(); i++)
					changes[info->assets[i].asset->Guid()] = AssetChangeType::ASSET_DELETED;
				for (size_t i = 0; i < assets.size(); i++) {
					const GUID guid = assets[i].asset->Guid();
					decltype(changes)::iterator it = changes.find(guid);
					if (it == changes.end()) changes[guid] = AssetChangeType::ASSET_CREATED;
					else it->second = AssetChangeType::ASSET_MODIFIED;
				}
				for (decltype(changes)::const_iterator it = changes.begin(); it != changes.end(); ++it)
					m_onDatabaseChanged(DatabaseChangeInfo{ it->first, it->second });
			}

			info->assets = std::move(assets);
			
			// Store/Overwrite the meta file:
			StoreMetadata(info->serializer->Serialize(info->reader), m_assetDirectoryObserver->Log(), metadataPath, &metadataJson);

			return true;
		};

		Reference<AssetReaderInfo> readerInfo = getStoredReaderInfo();
		if (readerInfo == nullptr)
			readerInfo = Object::Instantiate<AssetReaderInfo>();
		if (!updateReaderInfo(readerInfo))
			for (size_t i = 0; i < fileInfo.serializers.size(); i++) {
				readerInfo->serializer = fileInfo.serializers[i];
				readerInfo->reader = createReader(readerInfo->serializer);
				if (updateReaderInfo(readerInfo)) return;
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

	void FileSystemDatabase::FileRenamed(const OS::Path& oldPath, const OS::Path& newPath) {
		if (oldPath == newPath) return; // Nothing to do here...
		Reference<PathLock> pLock_0 = m_pathLockCache.LockFor(oldPath);
		Reference<PathLock> pLock_1 = m_pathLockCache.LockFor(newPath);
		if (oldPath < newPath) std::swap(pLock_0, pLock_1);
		std::unique_lock<std::mutex> pathLock_0(*pLock_0);
		std::unique_lock<std::mutex> pathLock_1(*pLock_1);
		std::unique_lock<std::mutex> pathLock(m_pathReaderLock);
		PathReaderInfo::const_iterator it = m_pathReaders.find(oldPath);
		if (it == m_pathReaders.end()) return;
		Reference<AssetReaderInfo> info = it->second;
		m_pathReaders.erase(it);
		std::unique_lock<std::mutex> readerLock(info->reader->m_pathLock);
		info->reader->m_path = newPath;
		m_pathReaders[newPath] = info;
		{
			std::unique_lock<std::recursive_mutex> dbLock(m_databaseLock);
			for (size_t i = 0; i < info->assets.size(); i++)
				m_assetCollection.AssetSourceFileRenamed(info->assets[i].asset);
		}
		// Rename metadata:
		{
			std::error_code error;
			std::filesystem::remove(MetadataPath(oldPath, m_metadataExtension), error);
			StoreMetadata(info->serializer->Serialize(info->reader), m_assetDirectoryObserver->Log(), MetadataPath(newPath, m_metadataExtension), nullptr);
		}
	}

	void FileSystemDatabase::FileErased(const OS::Path& path) {
		Reference<PathLock> pLock = m_pathLockCache.LockFor(path);
		std::unique_lock<std::mutex> lockForPath(*pLock);
		std::unique_lock<std::mutex> pathLock(m_pathReaderLock);
		PathReaderInfo::const_iterator it = m_pathReaders.find(path);
		if (it == m_pathReaders.end()) return;
		Reference<AssetReaderInfo> info = it->second;
		m_pathReaders.erase(it);
		{
			std::unique_lock<std::recursive_mutex> dbLock(m_databaseLock);
			for (size_t i = 0; i < info->assets.size(); i++)
				m_assetCollection.RemoveAsset(info->assets[i].asset);
		}
		for (size_t i = 0; i < info->assets.size(); i++)
			m_onDatabaseChanged(DatabaseChangeInfo{ info->assets[i].asset->Guid(), AssetChangeType::ASSET_DELETED });

		// Remove metadata:
		{
			std::error_code error;
			std::filesystem::remove(MetadataPath(path, m_metadataExtension), error);
		}
	}

	void FileSystemDatabase::OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info) {
		std::unique_lock<std::mutex> lock(m_observerLock);
		if ((info.changeType == OS::DirectoryChangeObserver::FileChangeType::CREATED) ||
			(info.changeType == OS::DirectoryChangeObserver::FileChangeType::MODIFIED))
			QueueFile(AssetFileInfo{ info.filePath, {} });
		else if (info.changeType == OS::DirectoryChangeObserver::FileChangeType::DELETED)
			FileErased(info.filePath);
		else if (info.changeType == OS::DirectoryChangeObserver::FileChangeType::RENAMED) {
			if (!info.oldPath.has_value()) 
				m_assetDirectoryObserver->Log()->Error("FileSystemDatabase::OnFileSystemChanged - changeType is RENAMED, but old path is missing! [File:", __FILE__, "; Line:", __LINE__);
			else FileRenamed(info.oldPath.value(), info.filePath);
		}
	}





	void FileSystemDatabase::AssetCollection::ClearTypeIndexFor(Info* info) {
		for (std::set<TypeId>::iterator typeIt = info->parentTypes.begin(); typeIt != info->parentTypes.end(); ++typeIt) {
			IndexPerType::iterator typeIndexIt = indexPerType.find(*typeIt);
			if (typeIndexIt == indexPerType.end()) continue;
			{
				TypeIndex& typeIndex = typeIndexIt->second;
				typeIndex.set.erase(info);
				auto removeIndexReference = [&](auto* index, const auto& name) {
					auto indexIt = index->find(name);
					if (indexIt == index->end()) return;
					indexIt->second.erase(info);
					if (indexIt->second.empty())
						index->erase(indexIt);
				};
				auto removeAllSubstrings = [&](auto* index, const auto& name) {
					removeIndexReference(index, name);
					const std::string nameString = name;
					{
						std::string substr = "";
						for (size_t i = 0; i < nameString.size(); i++) {
							removeIndexReference(index, substr);
							substr += nameString[i];
						}
					}
					for (size_t i = 0; i < nameString.size(); i++)
						removeIndexReference(index, nameString.substr(i));
				};
				removeAllSubstrings(&typeIndex.nameIndex, info->m_resourceName);
				removeIndexReference(&typeIndex.pathIndex, info->cannonicalSourceFilePath);
			}
			if (typeIndexIt->second.set.empty()) {
				assert(typeIndexIt->second.nameIndex.empty());
				assert(typeIndexIt->second.pathIndex.empty());
				indexPerType.erase(typeIndexIt);
			}
		}
	}
	void FileSystemDatabase::AssetCollection::FillTypeIndexFor(Info* info) {
		for (std::set<TypeId>::iterator typeIt = info->parentTypes.begin(); typeIt != info->parentTypes.end(); ++typeIt) {
			TypeIndex& typeIndex = indexPerType[*typeIt];
			typeIndex.set.insert(info);
			auto addIndexReferences = [&](auto* index, const auto& name) {
				(*index)[name].insert(info);
				const std::string nameString = name;
				{
					std::string substr = "";
					for (size_t i = 0; i < nameString.size(); i++) {
						(*index)[substr].insert(info);
						substr += nameString[i];
					}
				}
				for (size_t i = 0; i < nameString.size(); i++)
					(*index)[nameString.substr(i)].insert(info);
			};
			addIndexReferences(&typeIndex.nameIndex, info->m_resourceName);
			typeIndex.pathIndex[info->cannonicalSourceFilePath].insert(info);
		}
	}

	void FileSystemDatabase::AssetCollection::RemoveAsset(Asset* asset) {
		InfoByGUID::iterator it = infoByGUID.find(asset->Guid());
		if (it == infoByGUID.end()) return;
		ClearTypeIndexFor(it->second);
		infoByGUID.erase(it);
	}

	void FileSystemDatabase::AssetCollection::InsertAsset(const AssetImporter::AssetInfo& assetInfo, const AssetImporter* importer) {
		{
			const Reference<Info> oldInfo = [&]() -> Reference<Info> {
				InfoByGUID::const_iterator it = infoByGUID.find(assetInfo.asset->Guid());
				if (it != infoByGUID.end()) {
					importer->Log()->Error("FileSystemDatabase::AssetCollection::InsertAsset - Found duplicate GUID! [File:", __FILE__, "; Line:", __LINE__);
					return it->second;
				}
				else return nullptr;
			}();
			if (oldInfo != nullptr)
				RemoveAsset(oldInfo->m_asset);
		}
		const Reference<Info> info = Object::Instantiate<Info>();
		{
			info->m_asset = assetInfo.asset;
			info->m_sourceFilePath = importer->AssetFilePath();
			info->cannonicalSourceFilePath = SafeCannonicalPathFromPath(info->m_sourceFilePath);
			if (assetInfo.resourceName.has_value()) {
				info->m_resourceName = assetInfo.resourceName.value();
				info->nameIsFromSourceFile = false;
			}
			else {
				info->m_resourceName = OS::Path(info->m_sourceFilePath.filename());
				info->nameIsFromSourceFile = true;
			}
			info->importer = importer;

			typedef Callback<TypeId> RecordParentTypeCall;
			typedef void(*RecordParentTypeFn)(std::pair<std::set<TypeId>*, RecordParentTypeCall*>*, TypeId);
			static const RecordParentTypeFn recordFn = [](std::pair<std::set<TypeId>*, RecordParentTypeCall*>* data, TypeId typeId) {
				if (data->first->find(typeId) != data->first->end()) return;
				data->first->insert(typeId);
				typeId.GetParentTypes(*data->second);
			};
			RecordParentTypeCall callback(Unused<TypeId>);
			std::pair<std::set<TypeId>*, RecordParentTypeCall*> callData(&info->parentTypes, &callback);
			callback = RecordParentTypeCall(recordFn, &callData);
			callback(TypeId::Of<Resource>());
			callback(info->m_asset->ResourceType());
		}

		infoByGUID[info->m_asset->Guid()] = info;
		FillTypeIndexFor(info);
	}

	void FileSystemDatabase::AssetCollection::AssetSourceFileRenamed(Asset* asset) {
		InfoByGUID::iterator it = infoByGUID.find(asset->Guid());
		if (it == infoByGUID.end()) return;
		Info* info = it->second;
		ClearTypeIndexFor(info);
		{
			info->m_sourceFilePath = info->importer->AssetFilePath();
			info->cannonicalSourceFilePath = SafeCannonicalPathFromPath(info->m_sourceFilePath);
			if (info->nameIsFromSourceFile)
				info->m_resourceName = OS::Path(info->m_sourceFilePath.filename());
		}
		FillTypeIndexFor(info);
	}
}
