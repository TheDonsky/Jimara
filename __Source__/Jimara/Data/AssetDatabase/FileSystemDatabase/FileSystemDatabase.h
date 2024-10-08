#pragma once
#include "../AssetDatabase.h"
#include "../../ShaderLibrary.h"
#include "../../Serialization/ItemSerializers.h"
#include "../../../Core/Helpers.h"
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Physics/PhysicsInstance.h"
#include "../../../Audio/AudioDevice.h"
#include "../../../Core/Helpers.h"
#include "../../../OS/IO/MMappedFile.h"
#include "../../../OS/IO/DirectoryChangeObserver.h"
#include <thread>
#include <queue>
#include <set>
#include <condition_variable>


namespace Jimara {
	/// <summary>
	/// AssetDatabase based on some working directory
	/// </summary>
	class JIMARA_API FileSystemDatabase : public virtual AssetDatabase {
	private:
		struct Context;

	public:
		/// <summary> 
		/// Object, responsible for importing assets from files
		/// Note: AssetImporter implements AssetDatabase, but the interface presence 
		///		only serves to expose other assets from the same owner file system database. 
		///		It has nothing to do with the sub-assets contained within the importer or something like that
		/// </summary>
		class JIMARA_API AssetImporter : public virtual AssetDatabase {
		public:
			/// <summary> Asset information </summary>
			struct JIMARA_API AssetInfo {
				/// <summary> Asset </summary>
				Reference<Asset> asset;

				/// <summary> Name of the resource/asset [if not initialized, this one will be replaced by the source file name] </summary>
				std::optional<std::string> resourceName;
			};

			/// <summary>
			/// Imports assets from the file
			/// </summary>
			/// <param name="reportAsset"> Whenever the FileReader detects an asset within the file, it should report it's presence through this callback </param>
			/// <returns> True, if the entire file got parsed successfully, false otherwise </returns>
			virtual bool Import(Callback<const AssetInfo&> reportAsset) = 0;

			/// <summary> Graphics device </summary>
			Graphics::GraphicsDevice* GraphicsDevice()const;

			/// <summary> Bindless set of array buffers </summary>
			Graphics::BindlessSet<Graphics::ArrayBuffer>* BindlessBuffers()const;

			/// <summary> Bindless set of combined image samplers </summary>
			Graphics::BindlessSet<Graphics::TextureSampler>* BindlessSamplers()const;

			/// <summary> Shader library </summary>
			Jimara::ShaderLibrary* ShaderLibrary()const;

			/// <summary> Physics API instance </summary>
			Physics::PhysicsInstance* PhysicsInstance()const;

			/// <summary> Audio device </summary>
			Audio::AudioDevice* AudioDevice()const;

			/// <summary> Current path (may change if file gets moved; therefore, accessing it requires a lock and a deep copy) </summary>
			OS::Path AssetFilePath()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary>
			/// Finds an asset within the owner database
			/// </summary>
			/// <param name="id"> Asset identifier </param>
			/// <returns> Asset reference, if found </returns>
			virtual Reference<Asset> FindAsset(const GUID& id)override;

			/// <summary> 
			/// Serializer for AssetImporter objects, responsible for their instantiation, serialization and extension registration
			/// </summary>
			class Serializer : public virtual Serialization::SerializerList::From<AssetImporter> {
			public:
				/// <summary> Creates a new instance of an AssetImporter </summary>
				virtual Reference<AssetImporter> CreateReader() = 0;

				/// <summary>
				/// Ties AssetImporter::Serializer to the file extension
				/// </summary>
				/// <param name="extension"> File extension to look out for </param>
				void Register(const OS::Path& extension);

				/// <summary>
				/// Unties AssetImporter::Serializer from the file extension
				/// </summary>
				/// <param name="extension"> File extension to ignore </param>
				void Unregister(const OS::Path& extension);
			};

		protected:
			// Arbitrary data from the previous Import() call (may be randomly cleared; always cleared when last modified date is changed; only safe to use inside Import() method)
			inline std::string& PreviousImportData() { return m_previousImportData; }

		private:
			// File system database is allowed to set the internals
			friend class FileSystemDatabase;

			// Owner; inaccessible, but used for internal error checking
			std::atomic<FileSystemDatabase*> m_owner = nullptr;

			// Context
			Reference<const Context> m_context = nullptr;

			// Current path (may change if file gets moved; therefore, accessing it requires a lock and a deep copy)
			OS::Path m_path;
			mutable std::mutex m_pathLock;

			// Arbitrary data from the previous Import() call
			std::string m_previousImportData;
		};

		/// <summary>
		/// Default metadata extension
		/// Note: '.jado' stands for "Jimara Asset Data Object"
		/// </summary>
		inline static constexpr std::string_view DefaultMetadataExtension() { return ".jado"; }

		/// <summary>
		/// Arguments for creating a FileSystemDatabase instance
		/// <para/> All fields are REQUIRED, unless stated otherwise; some of them have explicit default values which are valid.
		/// </summary>
		struct JIMARA_API CreateArgs {
			/// <summary> Logger for error reporting (if not provided, the logger from graphics device will be used instead) </summary>
			Reference<OS::Logger> logger;

			/// <summary> Graphics device to use </summary>
			Reference<Graphics::GraphicsDevice> graphicsDevice;

			/// <summary> Bindless set of array buffers </summary>
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> bindlessBuffers;

			/// <summary> Bindless set of combined image samplers </summary>
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>> bindlessSamplers;

			/// <summary> Shader library/loader </summary>
			Reference<Jimara::ShaderLibrary> shaderLibrary;

			/// <summary> Physics API instance to use </summary>
			Reference<Physics::PhysicsInstance> physicsInstance;

			/// <summary> Audio device to use </summary>
			Reference<Audio::AudioDevice> audioDevice;

			/// <summary> Asset directory to listen to and find resources within </summary>
			OS::Path assetDirectory;

			/// <summary> Path to the cache for PreviousImportData entries (Optional; loaded on strartup; updated during destruction) </summary>
			std::optional<OS::Path> previousImportDataCache;

			/// <summary> Limit on the import thead count (at least one will be created) </summary>
			size_t importThreadCount = std::thread::hardware_concurrency();

			/// <summary> Extension of generated asset metadata files </summary>
			OS::Path metadataExtension = DefaultMetadataExtension();

			/// <summary> 
			/// Reports status of initial scan progress through this callback 
			/// <para/> First argument is 'num processed', second one is 'total file count'.
			/// <para/> This function can be local to the stack, since it's neither stored nor used after DB creation
			/// </summary>
			Callback<size_t, size_t> reportImportProgress = Callback<size_t, size_t>(Unused<size_t, size_t>);
		};

		/// <summary>
		/// Creates a FileSystemDatabase instance
		/// </summary>
		/// <param name="configuration"> Creation arguments </param>
		/// <returns> FileSystemDatabase if successful; nullptr otherwise </returns>
		static Reference<FileSystemDatabase> Create(const CreateArgs& configuration);

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemDatabase();

		/// <summary> Asset directory </summary>
		const OS::Path& AssetDirectory()const;

		/// <summary>
		/// Finds an asset within the database
		/// Note: The asset may or may not be loaded once returned;
		/// </summary>
		/// <param name="id"> Asset identifier </param>
		/// <returns> Asset reference, if found </returns>
		virtual Reference<Asset> FindAsset(const GUID& id) override;

		/// <summary> Information about an arbitrary asset </summary>
		class AssetInformation {
		public:
			/// <summary> Asset </summary>
			inline Asset* AssetRecord()const { return m_asset; }

			/// <summary> Name of the asset/resource </summary>
			inline const std::string& ResourceName()const { return m_resourceName; }

			/// <summary> File, this asset originates from </summary>
			inline const OS::Path& SourceFilePath()const { return m_sourceFilePath; }

		private:
			// AssetRecord
			Reference<Asset> m_asset;

			// ResourceName
			std::string m_resourceName;

			// SourceFilePath
			OS::Path m_sourceFilePath;

			// FileSystemDatabase can modify the content
			friend class FileSystemDatabase;
		};

		/// <summary>
		/// Gets information about the asset
		/// </summary>
		/// <param name="asset"> Asset of interest </param>
		/// <param name="info"> Result will be stored here </param>
		/// <returns> True, if the asset is found inside the DB and info is filled; false otherwise </returns>
		bool TryGetAssetInfo(const Asset* asset, AssetInformation& info)const;

		/// <summary>
		/// Gets information about the asset
		/// </summary>
		/// <param name="id"> GUID of interest </param>
		/// <param name="info"> Result will be stored here </param>
		/// <returns> True, if the asset is found inside the DB and info is filled; false otherwise </returns>
		bool TryGetAssetInfo(const GUID& id, AssetInformation& info)const;

		/// <summary>
		/// Retrieves assets by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <param name="resourceType"> Type of the resource assets are expected to load </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		void GetAssetsOfType(const TypeId& resourceType, const Callback<const AssetInformation&>& reportAsset, bool exactType = false)const;

		/// <summary>
		/// Retrieves assets by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="resourceType"> Type of the resource assets are expected to load  </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename CallbackType>
		inline void GetAssetsOfType(const TypeId& resourceType, const CallbackType& reportAsset, bool exactType = false)const {
			void(*report)(const CallbackType*, const AssetInformation&) = [](const CallbackType* call, const AssetInformation& info) { (*call)(info); };
			GetAssetsOfType(resourceType, Callback<const AssetInformation&>(report, &reportAsset), exactType);
		}

		/// <summary>
		/// Retrieves assets by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="ResourceType"> Type of the resource assets are expected to load </typeparam>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename ResourceType, typename CallbackType>
		inline void GetAssetsOfType(const CallbackType& reportAsset, bool exactType = false)const {
			GetAssetsOfType(TypeId::Of<ResourceType>(), reportAsset, exactType);
		}

		/// <summary>
		/// Retrieves assets, filtered by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <param name="name"> Resource name or a substring of it </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactName"> If true, 'name' will be treated as an exact name and substrings will be ignored </param>
		/// <param name="resourceType"> Type of the resource assets are expected to load </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		void GetAssetsByName(
			const std::string& name, const Callback<const AssetInformation&>& reportAsset, bool exactName = false, 
			const TypeId& resourceType = TypeId::Of<Resource>(), bool exactType = false)const;

		/// <summary>
		/// Retrieves assets, filtered by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="name"> Resource name or a substring of it </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactName"> If true, 'name' will be treated as an exact name and substrings will be ignored </param>
		/// <param name="resourceType"> Type of the resource assets are expected to load </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename CallbackType>
		inline void GetAssetsByName(
			const std::string& name, const CallbackType& reportAsset, bool exactName = false, const TypeId& resourceType = TypeId::Of<Resource>(), bool exactType = false)const {
			void(*report)(const CallbackType*, const AssetInformation&) = [](const CallbackType* call, const AssetInformation& info) { (*call)(info); };
			GetAssetsByName(name, Callback<const AssetInformation&>(report, &reportAsset), exactName, resourceType, exactType);
		}

		/// <summary>
		/// Retrieves assets, filtered by type
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="ResourceType"> Type of the resource assets are expected to load </typeparam>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="name"> Resource name or a substring of it </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactName"> If true, 'name' will be treated as an exact name and substrings will be ignored </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename ResourceType, typename CallbackType>
		inline void GetAssetsByName(const std::string& name, const CallbackType& reportAsset, bool exactName = false, bool exactType = false)const {
			GetAssetsByName(name, reportAsset, exactName, TypeId::Of<ResourceType>(), exactType);
		}

		/// <summary>
		/// Retrieves assets, stored inside given file (will do nothing, if there are no records for the given file inside the database)
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <param name="sourceFilePath"> Path to the file of interest inside the target directory </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="resourceType"> Type of the resource assets are expected to load </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		void GetAssetsFromFile(
			const OS::Path& sourceFilePath, const Callback<const AssetInformation&>& reportAsset,
			const TypeId& resourceType = TypeId::Of<Resource>(), bool exactType = false)const;

		/// <summary>
		/// Retrieves assets, stored inside given file (will do nothing, if there are no records for the given file inside the database)
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="sourceFilePath"> Path to the file of interest inside the target directory </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="resourceType"> Type of the resource assets are expected to load </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename CallbackType>
		inline void GetAssetsFromFile(
			const OS::Path& sourceFilePath, const CallbackType& reportAsset, 
			const TypeId& resourceType = TypeId::Of<Resource>(), bool exactType = false)const {
			void(*report)(const CallbackType*, const AssetInformation&) = [](const CallbackType* call, const AssetInformation& info) { (*call)(info); };
			GetAssetsFromFile(sourceFilePath, Callback<const AssetInformation&>(report, &reportAsset), resourceType, exactType);
		}

		/// <summary>
		/// Retrieves assets, stored inside given file (will do nothing, if there are no records for the given file inside the database)
		/// <para/> Note: Resource loads inside reportAsset may result in deadlocks for the assets that may load asynchronously, so do not do that!
		/// </summary>
		/// <typeparam name="ResourceType"> Type of the resource assets are expected to load </typeparam>
		/// <typeparam name="CallbackType"> Any callback, that can receive constant AssetInformation reference as argument </typeparam>
		/// <param name="sourceFilePath"> Path to the file of interest inside the target directory </param>
		/// <param name="reportAsset"> Each asset stored in the database will be reported by invoking this callback </param>
		/// <param name="exactType"> If true, only the assets that have the exact resource type will be reported (ei parent types (if known) will be ignored) </param>
		template<typename ResourceType, typename CallbackType>
		inline void GetAssetsFromFile(const OS::Path& sourceFilePath, const CallbackType& reportAsset, bool exactType = false)const {
			GetAssetsFromFile(sourceFilePath, reportAsset, TypeId::Of<ResourceType>(), exactType);
		}

		/// <summary> Number of assets currently stored inside the database </summary>
		size_t AssetCount()const;

		/// <summary>
		/// Asset change type
		/// </summary>
		enum class JIMARA_API AssetChangeType : uint8_t {
			/// <summary> Nothing happened (never used) </summary>
			NO_CHANGE = 0,

			/// <summary> Asset created/discovered </summary>
			ASSET_CREATED = 1,

			/// <summary> Asset deleted/lost </summary>
			ASSET_DELETED = 2,

			/// <summary> Asset modified </summary>
			ASSET_MODIFIED = 3,

			/// <summary> Not a valid change type; just the number of valid types </summary>
			AssetChangeType_COUNT = 4
		};

		/// <summary>
		/// Information about an asset change within the database
		/// </summary>
		struct JIMARA_API DatabaseChangeInfo {
			/// <summary> GUID of the asset in question </summary>
			GUID assetGUID = {};

			/// <summary> Information, about "what happened" to the asset record </summary>
			AssetChangeType changeType;
		};

		/// <summary> Invoked each time the asset database internals change </summary>
		Event<DatabaseChangeInfo>& OnDatabaseChanged()const;


	private:
		// Basic app context
		struct Context : public virtual Object {
			/// Logger
			Reference<OS::Logger> logger;

			/// Graphics device
			Reference<Graphics::GraphicsDevice> graphicsDevice = nullptr;

			/// Bindless set of array buffers
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> bindlessBuffers;

			/// Bindless set of combined image samplers
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>> bindlessSamplers;

			/// Shader loader
			Reference<Jimara::ShaderLibrary> shaderLibrary = nullptr;

			/// Physics instance
			Reference<Physics::PhysicsInstance> physicsInstance = nullptr;

			// Audio device
			Reference<Audio::AudioDevice> audioDevice = nullptr;

			// Lock for owner
			mutable SpinLock ownerLock;

			// Owner
			FileSystemDatabase* owner = nullptr;
		};

		// Basic application context
		const Reference<Context> m_context;

		// Asset directory change observer
		const Reference<OS::DirectoryChangeObserver> m_assetDirectoryObserver;

		// Asset metadata extension
		const OS::Path m_metadataExtension;

		// Lock for directory observer notifications
		std::mutex m_observerLock;

		// Asset collection
		struct AssetCollection {
			struct Info : public virtual AssetInformation, public virtual Object {
				OS::Path cannonicalSourceFilePath;
				std::atomic<bool> nameIsFromSourceFile = false;
				Reference<const AssetImporter> importer;
				size_t importerAssetIndex = 0u;
				std::set<std::string> parentTypes; // TypeId-s as strings, to avoid issues with dynamic reloads
			};

			typedef std::unordered_map<GUID, Reference<Info>> InfoByGUID;
			InfoByGUID infoByGUID;

			// Index per resource type (entry for Resource<> stores full set)
			struct TypeIndex {
				typedef std::unordered_set<Reference<Info>> Set;
				Set set;

				typedef std::unordered_map<std::string, std::set<Reference<Info>>> NameIndex;
				// Info mapped to each substring of the resource name
				NameIndex nameIndex;

				typedef std::unordered_map<OS::Path, std::set<Reference<Info>>> PathIndex;
				// Info mapped to resource file path
				PathIndex pathIndex;
			};
			typedef std::unordered_map<std::string, TypeIndex> IndexPerType; // TypeId-s as strings, to avoid issues with dynamic reloads
			IndexPerType indexPerType;

			void ClearTypeIndexFor(Info* info);
			void FillTypeIndexFor(Info* info);

			void RemoveAsset(Asset* asset);
			void InsertAsset(const AssetImporter::AssetInfo& assetInfo, const AssetImporter* importer, size_t assetIndex);
			void AssetSourceFileRenamed(Asset* asset);
		} m_assetCollection;

		// Lock for asset collection
		mutable std::recursive_mutex m_databaseLock;


		// Lock for any given path
		class PathLock : public virtual ObjectCache<OS::Path>::StoredObject, public virtual std::mutex {
		public:
			class Cache : public virtual ObjectCache<OS::Path> {
			public:
				inline Reference<PathLock> LockFor(const OS::Path& path) {
					return GetCachedOrCreate(path, [&]()-> Reference<PathLock> {
						return Object::Instantiate<PathLock>();
						});
				}
			};
		};
		PathLock::Cache m_pathLockCache;

		// Basic Asset file information
		struct AssetFileInfo {
			OS::Path filePath;
			std::vector<Reference<AssetImporter::Serializer>> serializers;
		};

		// Information about the asset's content
		struct AssetReaderInfo : public virtual Object {
			Reference<AssetImporter::Serializer> serializer;
			Reference<AssetImporter> reader;
			std::vector<AssetImporter::AssetInfo> assets;
		};

		// Path to current AssetReaderInfo mapping
		typedef std::unordered_map<OS::Path, Reference<AssetReaderInfo>> PathReaderInfo;
		PathReaderInfo m_pathReaders;
		std::mutex m_pathReaderLock;
		
		// Asset (re)import queue
		typedef std::queue<AssetFileInfo> AssetImportQueue;
		AssetImportQueue m_importQueue;

		// Collection of paths already inside the import queue
		typedef std::unordered_set<OS::Path> QueuedPaths;
		QueuedPaths m_queuedPaths;
		
		// Lock and condition for m_importQueue and m_queuedPaths
		std::mutex m_importQueueLock;
		std::condition_variable m_importAvaliable;
		std::atomic<bool> m_dead = false;

		// 'PreviousImportData' storage
		struct PreviousFileImportData {
			uint64_t lastModifiedDate = 0u;
			std::string previousImportData;
		};
		std::map<std::string, PreviousFileImportData> m_previousImportData;
		const std::optional<OS::Path> m_previousImportDataCache;
		std::mutex m_previousImportDataLock;

		// Invoked each time the asset database internals change
		mutable EventInstance<DatabaseChangeInfo> m_onDatabaseChanged;

		// Private Constructor
		FileSystemDatabase(const CreateArgs& configuration, OS::DirectoryChangeObserver* observer);

		// Import threads
		std::vector<std::thread> m_importThreads;
		void ImportThread();

		// (Re)Imports the file
		void ImportFile(const AssetFileInfo& fileInfo);

		// Queues file parsing
		void QueueFile(AssetFileInfo&& fileInfo);

		// Invoked, when a file gets renamed; should act accordingly
		void FileRenamed(const OS::Path& oldPath, const OS::Path& newPath);

		// Invoked, when a file gets erased; should act accordingly
		void FileErased(const OS::Path& path);

		// Invoked, whenever something happens within the observed directory
		void OnFileSystemChanged(const OS::DirectoryChangeObserver::FileChangeInfo& info);
	};
}
